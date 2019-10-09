


#include <boost/lexical_cast.hpp>

#include <stdexcept> // out of range

#include "projectsipdialog.h"
#include "projectsipconfig.h"
#include "projectsipregistrar.h"
#include "projectsipdirectory.h"
#include "projectsipsdp.h"
#include "globals.h"


/*!md
# invitestart
The start of a dialog state machine. We check we have a valid INVITE
and if we need to authenticate the user.
*/
void projectsipdialog::invitestart( projectsippacket::pointer pk )
{
  this->lastreceivedpk = pk;

  /* Do we need authenticating? Currently force authentication. But we will need also friendly IP to allow inbound calls. */
#warning finish off auth
  if ( 1 /* do we need authenticating */ )
  {
    substring user = pk->getheaderparam( projectsippacket::Authorization, "username" );
    if( 0 != user.end() )
    {
      // Auth has been included
      this->inviteauth( pk );
      return;
    }

    this->retries = 3;
    this->send401();

    this->nextstate = std::bind(
        &projectsipdialog::waitforacktheninviteauth,
        this,
        std::placeholders::_1 );

    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
        std::bind( &projectsipdialog::resend401, this, std::placeholders::_1 ) );
    return;
  }

  this->domain = pk->geturihost().str();

  if( pk->getheader( projectsippacket::Content_Length ).toint() > 0 &&
      0 != pk->getheader( projectsippacket::Content_Type ).find( "sdp" ).end() )
  {
    sdptojson( pk->getbody(), this->remotesdp );
  }

  this->invitepk = pk;
  this->updatecontrol();
  this->nextstate = std::bind(
        &projectsipdialog::waitfornextinstruction,
        this,
        std::placeholders::_1 );

  this->trying();
}

/*!md
# waitforacktheninviteauth
We have (probably) sent a auth request via an invite responce. We now have to get an Ack then the client will proceed to invite with auth.
*/
void projectsipdialog::waitforacktheninviteauth( projectsippacket::pointer pk )
{
  this->lastackpacket = pk;
  if( projectsippacket::ACK == pk->getmethod() )
  {
    this->ackpk = pk;
    this->canceltimer();
    this->nextstate = std::bind( &projectsipdialog::inviteauth, this, std::placeholders::_1 );
  }
}

/*!md
# inviteauth
We have requested authentication so this packet should contain auth information.
*/
void projectsipdialog::inviteauth( projectsippacket::pointer pk )
{

  if( projectsippacket::INVITE != pk->getmethod() )
  {
    return;
  }

  substring user = pk->getheaderparam( projectsippacket::Authorization, "username" );
  substring realm = pk->getheaderparam( projectsippacket::Authorization, "realm" );

  stringptr password = projectsipdirdomain::lookuppassword(
                realm,
                user );

  if( !password ||
        !pk->checkauth( this->authrequest, password ) )
  {
    this->send403();
    this->nextstate = std::bind( &projectsipdialog::waitforackanddie, this, std::placeholders::_1 );

    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
        std::bind( &projectsipdialog::ontimeoutenddialog, this, std::placeholders::_1 ) );

    return;
  }
  this->theirtag = pk->gettag( projectsippacket::From ).strptr();
  this->touser = pk->getuser().str();
  this->fromuser = pk->getuser( projectsippacket::From ).str();
  this->todomain = pk->gethost().str();
  this->domain = pk->geturihost().str();
  this->authenticated = true;
  this->invite( pk );
}

/*!md
# invite
We have received an invite. We may have authed or not depending on our rules. It may be a re-invite.
*/
void projectsipdialog::invite( projectsippacket::pointer pk )
{
  if( pk->getheader( projectsippacket::Content_Length ).toint() > 0 &&
      0 != pk->getheader( projectsippacket::Content_Type ).find( "sdp" ).end() )
  {
    sdptojson( pk->getbody(), this->remotesdp );
  }

  if( 0 != pk->gettag().end() )
  {
    this->checkforhold();

    this->invitepk = pk;
    this->lastauthenticatedpk = pk;
    this->updatecontrol();
    return;
  }

  this->invitepk = pk;
  this->lastauthenticatedpk = pk;
  this->updatecontrol();
  this->trying();
}

/*!md
# checkforhold
Looks for hold in sdp and sets flag appropriately. I may regret doing this here - more appropriate for control server?
*/
bool projectsipdialog::checkforhold( void )
{
  JSON::Object &sdp = JSON::as_object( this->remotesdp );

  // The new way
  if( sdp.has_key( "m" ) )
  {
    if( 4 == sdp[ "m" ].which() ) // Array
    {
      JSON::Array &m = JSON::as_array( sdp[ "m" ] );
      for( auto it = m.values.begin(); it != m.values.end(); it++ )
      {
        if( 3 == it->which() ) // Object
        {
          JSON::Object &m_i = JSON::as_object( *it );
          if( m_i.has_key( "direction" ) &&
              5 == m_i[ "direction" ].which() && // String
               "inactive" == JSON::as_string( m_i[ "direction" ] ) )
          {
            this->callhold = true;
            return true;
          }
        }
      }
    }
  }

  // Old way
  if( sdp.has_key( "o" ) )
  {
    if( 3 == sdp[ "o" ].which() ) // Object
    {
      JSON::Object &o = JSON::as_object( sdp[ "o" ] );
      if( o.has_key( "address" ) &&
          5 == o[ "address" ].which() && // String
          "0.0.0.0" == JSON::as_string( o[ "address" ] ) )
      {
        this->callhold = true;
        return true;
      }
    }
  }
  this->callhold = false;
  return true;
}
