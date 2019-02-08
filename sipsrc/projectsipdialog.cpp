

#include <boost/lexical_cast.hpp>

#include "projectsipdialog.h"
#include "projectsipconfig.h"
#include "projectsipdirectory.h"
#include "projectsipsdp.h"


static projectsipdialogs dialogs;
static unsigned int projectsipdialogcounter = 0;
extern boost::asio::io_service io_service;

/*******************************************************************************
Function: projectsipdialog
Purpose: Constructor
Updated: 23.01.2019
*******************************************************************************/
projectsipdialog::projectsipdialog() :
  laststate( std::bind( &projectsipdialog::invitestart, this, std::placeholders::_1 ) ),
  nextstate( std::bind( &projectsipdialog::invitestart, this, std::placeholders::_1 ) ),
  controlrequest( projecthttpclient::create( io_service ) ),
  timer( io_service ),
  retries( 3 ),
  authenticated( false ),
  callringing( false ),
  callanswered( false ),
  callhungup( false ),
  startat( std::time(nullptr) ),
  ringingat( 0 ),
  answerat( 0 ),
  endat( 0 )
{
  projectsipdialogcounter++;
}

/*******************************************************************************
Function: projectsipdialog
Purpose: Destructor
Updated: 23.01.2019
*******************************************************************************/
projectsipdialog::~projectsipdialog()
{
  this->controlrequest->asynccancel();
}

/*******************************************************************************
Function: create
Purpose: 
Updated: 23.01.2019
*******************************************************************************/
projectsipdialog::pointer projectsipdialog::create()
{
  return pointer( new projectsipdialog() );
}

/*******************************************************************************
Function: invitesippacket
Purpose: Handle a SIP invite packet. Return false if the packet was not
intended for us (i.e. not part of a dialog) or true if it was.
Updated: 23.01.2019
*******************************************************************************/
bool projectsipdialog::invitesippacket( projectsippacketptr pk )
{
  std::string callid = pk->getheader( projectsippacket::Call_ID ).str();

  if( 5 > callid.size() )
  {
    // Ignore small call ids.
    return true;
  }

  projectsipdialogs::index< projectsipdialogcallid >::type::iterator it;
  it = dialogs.get< projectsipdialogcallid >().find( callid );

  projectsipdialog::pointer ptr;

  if( dialogs.get< projectsipdialogcallid >().end() == it )
  {
    switch( pk->getmethod() )
    {
      case projectsippacket::RESPONSE:
      {
        return false;
        break;
      }
      case projectsippacket::INVITE:
      {
        // New dialog
        ptr = projectsipdialog::create();
        ptr->callid = callid;
        dialogs.insert( ptr );
        break;
      }
      default:
      {
        projectsippacketptr response = projectsippacketptr( new projectsippacket() );

        response->setstatusline( 481, "Call Leg/Transaction Does Not Exist" );
        response->addviaheader( projectsipconfig::gethostname(), pk.get() );

        response->addwwwauthenticateheader( pk.get() );

        response->addheader( projectsippacket::To,
                            pk->getheader( projectsippacket::To ) );
        response->addheader( projectsippacket::From,
                            pk->getheader( projectsippacket::From ) );
        response->addheader( projectsippacket::Call_ID,
                            pk->getheader( projectsippacket::Call_ID ) );
        response->addheader( projectsippacket::CSeq,
                            pk->getheader( projectsippacket::CSeq ) );
        response->addheader( projectsippacket::Contact,
                            projectsippacket::contact( pk->getuser().strptr(), 
                            stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                            0, 
                            projectsipconfig::getsipport() ) );
        response->addheader( projectsippacket::Allow,
                            "INVITE, ACK, CANCEL, OPTIONS, BYE" );
        response->addheader( projectsippacket::Content_Type,
                            "application/sdp" );
        response->addheader( projectsippacket::Content_Length,
                            "0" );

        pk->respond( response->strptr() );
        return false;
      }
    }
  }
  else
  {
    ptr = *it;
  }

  switch( pk->getmethod() )
  {
    case projectsippacket::BYE:
    {
      /* we always have to handle a BYE. */
      ptr->handlebye( pk );
      break;
    }
    default:
    {
      ptr->nextstate( pk );
      break;
    }
  }
  
  return true;
}

/*******************************************************************************
Timer functions.
*******************************************************************************/

/*******************************************************************************
Function: ontimeoutenddialog
Purpose: Timer handler to end this dialog on timeout.
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::ontimeoutenddialog( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    this->untrack();
  }
}

/*******************************************************************************
Function: ontimeout486andenddialog
Purpose: Timer handler to end this dialog on timeout.
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::ontimeout486andenddialog( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    this->temporaryunavailable();
  }
}

/*******************************************************************************
Function: waitfortimer
Purpose: Async wait for timer.
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::waitfortimer( std::chrono::milliseconds s, 
      std::function<void ( const boost::system::error_code& error ) > f )
{
  this->timer.cancel();
  this->timer.expires_after( s );
  this->timer.async_wait( f );
}

/*******************************************************************************
Function: canceltimer
Purpose: Set the timer back to the default.
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::canceltimer( void )
{
  this->timer.cancel();
}

/*******************************************************************************
Function: handlebye
Purpose: We have received a BYE - clean up.
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::handlebye( projectsippacketptr pk )
{
  this->callhungup = true;
  this->endat = std::time( nullptr );
  this->lastpacket = pk;
  this->retries = 3;
  this->send200();
  this->updatecontrol( pk );
}

/*******************************************************************************
State functions.
*******************************************************************************/

/*******************************************************************************
Function: invitestart
Purpose: The start of a dialog state machine. We check we have a valid INVITE
and if we need to authenticate the user.
Updated: 23.01.2019
*******************************************************************************/
void projectsipdialog::invitestart( projectsippacketptr pk )
{
  this->lastpacket = pk;

  // Do we need authenticating?
  if ( 0 /* do we need authenticating */ )
  {
    this->authrequest = projectsippacketptr( new projectsippacket() );

    this->authrequest->setstatusline( 401, "Unauthorized" );
    this->authrequest->addviaheader( projectsipconfig::gethostname(), pk.get() );

    this->authrequest->addwwwauthenticateheader( pk.get() );

    this->authrequest->addheader( projectsippacket::To,
                        pk->getheader( projectsippacket::To ) );
    this->authrequest->addheader( projectsippacket::From,
                        pk->getheader( projectsippacket::From ) );
    this->authrequest->addheader( projectsippacket::Call_ID,
                        pk->getheader( projectsippacket::Call_ID ) );
    this->authrequest->addheader( projectsippacket::CSeq,
                        pk->getheader( projectsippacket::CSeq ) );
    this->authrequest->addheader( projectsippacket::Contact,
                        projectsippacket::contact( pk->getuser().strptr(), 
                        stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                        0, 
                        projectsipconfig::getsipport() ) );
    this->authrequest->addheader( projectsippacket::Allow,
                        "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    this->authrequest->addheader( projectsippacket::Content_Type,
                        "application/sdp" );
    this->authrequest->addheader( projectsippacket::Content_Length,
                        "0" );

    pk->respond( this->authrequest->strptr() );

    this->nextstate = std::bind( &projectsipdialog::inviteauth, this, std::placeholders::_1 );
    return;
  }

  this->updatecontrol( pk );
  this->nextstate = std::bind( &projectsipdialog::waitfornextinstruction, this, std::placeholders::_1 );
  this->trying();
}

/*******************************************************************************
Function: inviteauth
Purpose: We have requested authentication.
Updated: 23.01.2019
*******************************************************************************/
void projectsipdialog::inviteauth( projectsippacketptr pk )
{
  this->lastpacket = pk;

  stringptr password = projectsipdirectory::lookuppassword( 
                pk->geturihost(), 
                pk->getuser() );


  if( !password ||
        !pk->checkauth( this->authrequest.get(), password ) )
  {
    projectsippacket failedauth;

    failedauth.setstatusline( 403, "Failed auth" );
    failedauth.addviaheader( projectsipconfig::gethostname(), pk.get() );

    failedauth.addheader( projectsippacket::To,
                pk->getheader( projectsippacket::To ) );
    failedauth.addheader( projectsippacket::From,
                pk->getheader( projectsippacket::From ) );
    failedauth.addheader( projectsippacket::Call_ID,
                pk->getheader( projectsippacket::Call_ID ) );
    failedauth.addheader( projectsippacket::CSeq,
                pk->getheader( projectsippacket::CSeq ) );
    failedauth.addheader( projectsippacket::Contact,
                projectsippacket::contact( pk->getuser().strptr(), 
                stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                0,
                projectsipconfig::getsipport() ) );
    failedauth.addheader( projectsippacket::Allow,
                "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    failedauth.addheader( projectsippacket::Content_Type,
                "application/sdp" );
    failedauth.addheader( projectsippacket::Content_Length,
                        "0" );

    pk->respond( failedauth.strptr() );

    return;
  }

  this->authenticated = true;
  this->updatecontrol( pk );
  this->trying();
}

/*******************************************************************************
Function: trying
Purpose: Send 100 back to client.
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::trying( void )
{
  projectsippacket trying;
  trying.setstatusline( 100, "Trying" );
  trying.addviaheader( projectsipconfig::gethostname(), this->lastpacket.get() );

  trying.addheader( projectsippacket::To,
              this->lastpacket->getheader( projectsippacket::To ) );
  trying.addheader( projectsippacket::From,
              this->lastpacket->getheader( projectsippacket::From ) );
  trying.addheader( projectsippacket::Call_ID,
              this->lastpacket->getheader( projectsippacket::Call_ID ) );
  trying.addheader( projectsippacket::CSeq,
              this->lastpacket->getheader( projectsippacket::CSeq ) );
  trying.addheader( projectsippacket::Contact,
              projectsippacket::contact( this->lastpacket->getuser().strptr(), 
              stringptr( new std::string( projectsipconfig::gethostip() ) ), 
              0,
              projectsipconfig::getsipport() ) );
  trying.addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  trying.addheader( projectsippacket::Content_Length,
                      "0" );

  this->lastpacket->respond( trying.strptr() );

  this->waitfortimer( std::chrono::milliseconds( DIALOGSETUPTIMEOUT ), 
      std::bind( &projectsipdialog::ontimeout486andenddialog, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: temporaryunavailble
Purpose: Send 480 back to client.
Updated: 29.01.2019
*******************************************************************************/
void projectsipdialog::temporaryunavailable( void )
{
  projectsippacket unavail;

  unavail.setstatusline( 480, "Temporarily Unavailable" );
  unavail.addviaheader( projectsipconfig::gethostname(), this->lastpacket.get() );

  unavail.addheader( projectsippacket::To,
              this->lastpacket->getheader( projectsippacket::To ) );
  unavail.addheader( projectsippacket::From,
              this->lastpacket->getheader( projectsippacket::From ) );
  unavail.addheader( projectsippacket::Call_ID,
              this->lastpacket->getheader( projectsippacket::Call_ID ) );
  unavail.addheader( projectsippacket::CSeq,
              this->lastpacket->getheader( projectsippacket::CSeq ) );
  unavail.addheader( projectsippacket::Contact,
              projectsippacket::contact( this->lastpacket->getuser().strptr(), 
              stringptr( new std::string( projectsipconfig::gethostip() ) ), 
              0,
              projectsipconfig::getsipport() ) );
  unavail.addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  unavail.addheader( projectsippacket::Content_Length,
                      "0" );

  this->lastpacket->respond( unavail.strptr() );

  // We should receive a ACK - at that point the dialog can be closed.
  this->nextstate = std::bind( &projectsipdialog::waitforackanddie, this, std::placeholders::_1 );

  this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ), 
      std::bind( &projectsipdialog::ontimeoutenddialog, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: waitforack
Purpose: 
Updated: 05.02.2019
*******************************************************************************/
void projectsipdialog::waitforack( projectsippacketptr pk )
{
  this->lastpacket = pk;
  switch( pk->getmethod() )
  {
    case projectsippacket::ACK:
    {
      this->canceltimer();
      this->updatecontrol( pk );
      break;
    }
  }
}

/*******************************************************************************
Function: waitforackanddie
Purpose: 
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::waitforackanddie( projectsippacketptr pk )
{
  this->lastpacket = pk;
  // When we get an ACK to the last message we can die.
  if( projectsippacket::ACK == pk->getmethod() )
  {
    this->untrack();
  }
}

/*******************************************************************************
Function: waitfornextinstruction
Purpose: We have received an INVITE and sent a trying, if the client has 
not recevieved the trying then we will get another INVITE. So send trying again
whilst we wait for our next control instruction.
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::waitfornextinstruction( projectsippacketptr pk )
{
  this->lastpacket = pk;

  switch( pk->getmethod() )
  {
    case projectsippacket::INVITE:
    {
      this->trying();
      break;
    }
  }

  /* Wait up to DIALOGSETUPTIMEOUT seconds before we close this dialog */
  this->waitfortimer( std::chrono::milliseconds( DIALOGSETUPTIMEOUT ), 
      std::bind( &projectsipdialog::ontimeoutenddialog, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: ringing
Purpose: Send a 180 ringing
Updated: 29.01.2019
*******************************************************************************/
void projectsipdialog::ringing( void )
{
  if( !this->callringing )
  {
    projectsippacket ringing;

    ringing.setstatusline( 180, "Ringing" );
    ringing.addviaheader( projectsipconfig::gethostname(), this->lastpacket.get() );

    ringing.addheader( projectsippacket::To,
                this->lastpacket->getheader( projectsippacket::To ) );
    ringing.addheader( projectsippacket::From,
                this->lastpacket->getheader( projectsippacket::From ) );
    ringing.addheader( projectsippacket::Call_ID,
                this->lastpacket->getheader( projectsippacket::Call_ID ) );
    ringing.addheader( projectsippacket::CSeq,
                this->lastpacket->getheader( projectsippacket::CSeq ) );
    ringing.addheader( projectsippacket::Contact,
                projectsippacket::contact( this->lastpacket->getuser().strptr(), 
                stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                0,
                projectsipconfig::getsipport() ) );
    ringing.addheader( projectsippacket::Allow,
                "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    ringing.addheader( projectsippacket::Content_Length,
                        "0" );

    if( 0 != this->alertinfo.size() )
    {
      ringing.addheader( projectsippacket::Alert_Info,
                this->alertinfo );
    }

    this->lastpacket->respond( ringing.strptr() );

    this->canceltimer();
    this->callringing = true;
    this->ringingat = std::time( nullptr );

    this->waitfortimer( std::chrono::milliseconds( DIALOGSETUPTIMEOUT ), 
      std::bind( &projectsipdialog::ontimeout486andenddialog, this, std::placeholders::_1 ) );
  }
}

#warning TODO
/*******************************************************************************
Function: preanswer
Purpose: Send a 183 with SDP - early media (media before answer i.e. fake ringing)
Updated: 07.02.2019
*******************************************************************************/

/*******************************************************************************
Function: answer
Purpose: Send a 200 answer
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::answer( void )
{
  if( !this->callanswered )
  {
    this->retries = 3;
    this->send200();
    this->nextstate = std::bind( &projectsipdialog::waitforack, this, std::placeholders::_1 );
    this->callanswered = true;
    this->answerat = std::time( nullptr );

      /* Wait up to 60 seconds before we close this dialog */
    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ), 
      std::bind( &projectsipdialog::ontimeoutenddialog, this, std::placeholders::_1 ) );
  }
}

/*******************************************************************************
Function: send200
Purpose: Send a 200
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::send200( bool final )
{
  projectsippacket ok;

  ok.setstatusline( 200, "Ok" );
  ok.addviaheader( projectsipconfig::gethostname(), this->lastpacket.get() );

  ok.addheader( projectsippacket::To,
              this->lastpacket->getheader( projectsippacket::To ) );
  ok.addheader( projectsippacket::From,
              this->lastpacket->getheader( projectsippacket::From ) );
  ok.addheader( projectsippacket::Call_ID,
              this->lastpacket->getheader( projectsippacket::Call_ID ) );
  ok.addheader( projectsippacket::CSeq,
              this->lastpacket->getheader( projectsippacket::CSeq ) );
  ok.addheader( projectsippacket::Contact,
              projectsippacket::contact( this->lastpacket->getuser().strptr(), 
              stringptr( new std::string( projectsipconfig::gethostip() ) ), 
              0,
              projectsipconfig::getsipport() ) );
  ok.addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  ok.addheader( projectsippacket::Content_Length,
                      "0" );

  this->lastpacket->respond( ok.strptr() );

  if( final )
  {
    this->nextstate = std::bind( &projectsipdialog::waitforackanddie, this, std::placeholders::_1 );
  }
  else
  {
    this->nextstate = std::bind( &projectsipdialog::waitforack, this, std::placeholders::_1 );
  }

  this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ), 
      std::bind( &projectsipdialog::resend200, this, std::placeholders::_1 ) );
}


/*******************************************************************************
Function: resend200
Purpose: Send a 200
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::resend200( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    if( 0 == this->retries )
    {
      this->untrack();
      return;
    }

    this->send200();
    this->retries--;

    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ), 
        std::bind( &projectsipdialog::resend200, this, std::placeholders::_1 ) );
  }
}

/*******************************************************************************
Function: answer
Purpose: Send a 200 answer
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::busy( void )
{
  if( !this->callanswered )
  {
    this->retries = 3;
    this->send486();
    this->nextstate = std::bind( &projectsipdialog::waitforack, this, std::placeholders::_1 );
    this->callanswered = true;
    this->answerat = std::time( nullptr );

      /* Wait up to 60 seconds before we close this dialog */
    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ), 
      std::bind( &projectsipdialog::ontimeoutenddialog, this, std::placeholders::_1 ) );
  }
}

/*******************************************************************************
Function: send486
Purpose: Send a 486 Busy here
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::send486( void )
{
  projectsippacket ok;

  ok.setstatusline( 486, "Busy here" );
  ok.addviaheader( projectsipconfig::gethostname(), this->lastpacket.get() );

  ok.addheader( projectsippacket::To,
              this->lastpacket->getheader( projectsippacket::To ) );
  ok.addheader( projectsippacket::From,
              this->lastpacket->getheader( projectsippacket::From ) );
  ok.addheader( projectsippacket::Call_ID,
              this->lastpacket->getheader( projectsippacket::Call_ID ) );
  ok.addheader( projectsippacket::CSeq,
              this->lastpacket->getheader( projectsippacket::CSeq ) );
  ok.addheader( projectsippacket::Contact,
              projectsippacket::contact( this->lastpacket->getuser().strptr(), 
              stringptr( new std::string( projectsipconfig::gethostip() ) ), 
              0,
              projectsipconfig::getsipport() ) );
  ok.addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  ok.addheader( projectsippacket::Content_Length,
                      "0" );

  this->lastpacket->respond( ok.strptr() );

  this->nextstate = std::bind( &projectsipdialog::waitforack, this, std::placeholders::_1 );

  this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ), 
      std::bind( &projectsipdialog::resend486, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: resend486
Purpose: Send a 200
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::resend486( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    if( 0 == this->retries )
    {
      this->untrack();
      return;
    }

    this->send486();
    this->retries--;

    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ), 
        std::bind( &projectsipdialog::resend486, this, std::placeholders::_1 ) );
  }
}

/*******************************************************************************
Function: untrack
Purpose: Remove from our container so we can clean up.
Updated: 29.01.2019
*******************************************************************************/
void projectsipdialog::untrack( void )
{
  this->timer.cancel();

  projectsipdialogs::index< projectsipdialogcallid >::type::iterator it;
  it = dialogs.get< projectsipdialogcallid >().find( this->callid );
  if( dialogs.get< projectsipdialogcallid >().end() != it )
  {
    dialogs.erase( it );
  }
}

/*******************************************************************************
Control/HTTP functions.
*******************************************************************************/

/*******************************************************************************
Function: updatecontrol
Purpose: Pass information to our control server.
Updated: 25.01.2019
*******************************************************************************/
void projectsipdialog::updatecontrol( projectsippacketptr pk )
{
  projectwebdocumentptr d = projectwebdocumentptr( new projectwebdocument() );
  d->setrequestline( projectwebdocument::POST, "http://127.0.0.1/call" );

  JSON::Object v;

  v[ "callid" ] = pk->getheader( projectsippacket::Call_ID ).str();
  v[ "realm" ] = pk->geturihost().str();
  v[ "to" ] = pk->getuser( projectsippacket::To ).str();
  v[ "from" ] = pk->getuser( projectsippacket::From ).str();
  v[ "contact" ] = pk->getheader( projectsippacket::Contact ).str();
  v[ "authenticated" ] = ( JSON::Bool ) this->authenticated;
  v[ "ring" ] = ( JSON::Bool ) this->callringing;
  v[ "answered" ] = ( JSON::Bool ) this->callanswered;
  v[ "hangup" ] = ( JSON::Bool ) this->callhungup;

  JSON::Object rh;
  rh[ "host" ] = pk->getremotehost();
  rh[ "port" ] = boost::numeric_cast< JSON::Integer >( pk->getremoteport() );
  v[ "remote" ] = rh;

  JSON::Object epochs;
  epochs[ "start" ] = boost::numeric_cast< JSON::Integer >( this->startat );
  epochs[ "ring" ] = boost::numeric_cast< JSON::Integer >( this->ringingat );
  epochs[ "answer" ] = boost::numeric_cast< JSON::Integer >( this->answerat );
  epochs[ "end" ] = boost::numeric_cast< JSON::Integer >( this->endat );

  v[ "epochs" ] = epochs;

  if( pk->getheader( projectsippacket::Content_Length ).toint() > 0 &&
      0 != pk->getheader( projectsippacket::Content_Type ).find( "sdp" ).end() )
  {
    JSON::Value sdpv;
    if( sdptojson( pk->getbody(), sdpv ) )
    {
      v[ "sdp" ] = sdpv;
    }
  }

  std::string t = JSON::to_string( v );
  d->addheader( projectwebdocument::Content_Length, t.length() );
  d->addheader( projectwebdocument::Content_Type, "text/json" );
  d->setbody( t.c_str() );

  this->controlrequest->asyncrequest( d, std::bind( &projectsipdialog::httpcallback, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: httpcallback
Purpose: We have made a request to our control server. This is a do nothing if
no error occurred.
Updated: 23.01.2019
*******************************************************************************/
void projectsipdialog::httpcallback( int errorcode )
{
  if( 0 != errorcode )
  {
    this->temporaryunavailable();
    return;
  }

  projectwebdocumentptr r = this->controlrequest->getresponse();

  if( !r || 200 != r->getstatuscode() )
  {
    this->temporaryunavailable();
    return;
  }
  /* We are now idle until we receive our next instruction. */
}

/*******************************************************************************
Function: registrarhttpreq
Purpose: Handle a request from our web server.
Updated: 22.01.2019
*******************************************************************************/
void projectsipdialog::httpget( stringvector &path, projectwebdocument &response )
{
  JSON::Object v;
  v[ "count" ] = ( JSON::Integer ) dialogs.size();
  v[ "counter" ] = ( JSON::Integer ) projectsipdialogcounter;
  std::string t = JSON::to_string( v );

  response.setstatusline( 200, "Ok" );
  response.addheader( projectwebdocument::Content_Length, t.length() );
  response.addheader( projectwebdocument::Content_Type, "text/json" );
  response.setbody( t.c_str() );
}

/*******************************************************************************
Function: httppost
Purpose: Handle a request from our web server.
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::httppost( stringvector &path, JSON::Value &body, projectwebdocument &response )
{
  JSON::Object b = JSON::as_object( body );

  if( !b.has_key( "callid" ) )
  {
    response.setstatusline( 404, "Not found" );
  }

  projectsipdialogs::index< projectsipdialogcallid >::type::iterator it;
  it = dialogs.get< projectsipdialogcallid >().find( JSON::as_string( b[ "callid" ] ) );
  if( dialogs.get< projectsipdialogcallid >().end() == it )
  {
    response.setstatusline( 404, "Not found" );
  }
  else
  {
    if ( b.has_key( "action" ) )
    {
      std::string action = JSON::as_string( b[ "action" ] );
      if( action == "ring" )
      {
        response.setstatusline( 200, "Ok" );
        (*it)->alertinfo = "";
        if ( b.has_key( "alertinfo" ) )
        {
          (*it)->alertinfo = JSON::as_string( b[ "alertinfo" ] );
        }
        (*it)->ringing();
      }
      else if( action == "answer" )
      {
        response.setstatusline( 200, "Ok" );

        // need to add sdp info.
        (*it)->retries = 3;
        (*it)->answer();
      }
      else if( action == "busy" )
      {
        response.setstatusline( 200, "Ok" );

        // need to add sdp info.
        (*it)->retries = 3;
        (*it)->busy();
      }
    }
    else 
    {
      response.setstatusline( 404, "Not found" );
    }
  }

  response.addheader( projectwebdocument::Content_Length, 0 );
  response.addheader( projectwebdocument::Content_Type, "text/json" );
}


