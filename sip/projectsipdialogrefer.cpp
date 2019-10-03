

#include <boost/lexical_cast.hpp>

#include <stdexcept> // out of range

#include "projectsipdialog.h"
#include "projectsipconfig.h"
#include "projectsipregistrar.h"
#include "projectsipdirectory.h"
#include "projectsipsdp.h"
#include "globals.h"


/*!md
# handlerefer
*/
void projectsipdialog::handlerefer( projectsippacket::pointer pk )
{
  if( projectsippacket::REFER != pk->getmethod() )
  {
    return;
  }

  /* We always auth a refer? */
  substring user = pk->getheaderparam( projectsippacket::Authorization, "username" );
  if( 0 != user.end() )
  {
    // Auth has been included
    this->referauth( pk );
    return;
  }

  this->retries = 3;
  this->send401();

  this->nextstate = std::bind(
      &projectsipdialog::waitforack,
      this,
      std::placeholders::_1 );

  this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
      std::bind( &projectsipdialog::resend401, this, std::placeholders::_1 ) );
  return;
}

/*!md
# referauth
*/
void projectsipdialog::referauth( projectsippacket::pointer pk )
{
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
  this->domain = pk->geturihost().str();
  this->authenticated = true;
  this->refer( pk );
}

/*!md
# refer
We have gone through all other stages, now just do it.
*/
void projectsipdialog::refer( projectsippacket::pointer pk )
{
  this->referpk = pk;
  this->updatecontrol();
  this->retries = 3;
  this->send202();
}

/*!md
# notify
This may go elsewhere for being more generic, but for now this can go in here as this is where we use it.
*/
void projectsipdialog::notify( void )
{
  this->cseq++;
  this->sendnotify();
  this->nextstate = std::bind( &projectsipdialog::waitfor200, this, std::placeholders::_1 );
}

void projectsipdialog::sendnotify( void )
{
  if( !this->lastreceivedpk ) return;

  projectsippacket::pointer notify = projectsippacket::create();
  // Generate SIP URI i.e. sip:realm
  notify->setrequestline( projectsippacket::NOTIFY, this->invitepk->getrequesturi().str()  );
  
  notify->addcommonheaders();
  notify->addviaheader( projectsipconfig::gethostipsipport(), this->lastreceivedpk );

  notify->addheader( projectsippacket::To,
                this->lastreceivedpk->getheader( projectsippacket::From ) );

  notify->addheader( projectsippacket::From,
              this->lastreceivedpk->getheader( projectsippacket::To ) );

  notify->addheader( projectsippacket::Contact,
              projectsippacket::contact( this->lastreceivedpk->getuser().strptr(),
              stringptr( new std::string( projectsipconfig::gethostipsipport() ) ) ) );

  notify->addheader( projectsippacket::Call_ID, this->callid );

  /* TODO - need to correct the number - i.e. re-invite may be different. */
  notify->addheader( projectsippacket::CSeq, std::to_string( this->cseq ) + " NOTIFY" );
  
  notify->addheader( projectsippacket::Content_Type, "message/sipfrag;version=2.0" );
  notify->addheader( projectsippacket::Subscription_State, "active;expires=500" );
  notify->addheader( projectsippacket::Content_Length, "14" );
  notify->setbody( "SIP/2.0 200 OK" );

  this->sendpk( notify );

  this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
      std::bind( &projectsipdialog::ontimeoutenddialog, this, std::placeholders::_1 ) );
}

/*!md
# notify
This may go elsewhere for being more generic, but for now this can go in here as this is where we use it.
*/
void projectsipdialog::resendnotify( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    if( 0 == this->retries )
    {
      this->untrack();
      return;
    }

    this->sendnotify();
    this->retries--;

    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
        std::bind( &projectsipdialog::resendnotify, this, std::placeholders::_1 ) );
  }
}

