

#include <boost/lexical_cast.hpp>

#include <stdexcept> // out of range

#include "projectsipdialog.h"
#include "projectsipconfig.h"
#include "projectsipregistrar.h"
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
  endat( 0 ),
  finished( false ),
  totag( projectsippacket::tag() )
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
    /* There may be a reason I should seperate this out - but for now */
    case projectsippacket::BYE:
    case projectsippacket::CANCEL:
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
  this->send200();
  this->finished = true;
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

  this->domain = pk->geturihost().str();

  this->invitepacket = pk;
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

  stringptr password = projectsipdirdomain::lookuppassword(
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

  this->domain = pk->geturihost().str();

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

  sipuri u( this->lastpacket->getheader( projectsippacket::To ) );
  substring uri = u.uri;
  if( 0 != uri.end() )
  {
    uri.start( uri.start() - 1 );
    uri.end( uri.end() + 1 );
  }

  trying.addheader( projectsippacket::Via,
              this->lastpacket->getheader( projectsippacket::Via ) );
  trying.addheader( projectsippacket::To,
              uri.str() + ";" + *this->totag );
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
  if( projectsippacket::ACK == pk->getmethod() )
  {
    this->canceltimer();
    this->updatecontrol( pk );
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
    this->updatecontrol( pk );
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

    ringing.addheader( projectsippacket::Via,
              this->lastpacket->getheader( projectsippacket::Via ) );

    sipuri u( this->lastpacket->getheader( projectsippacket::To ) );
    substring uri = u.uri;
    if( 0 != uri.end() )
    {
      /* include the <> */
      uri.start( uri.start() - 1 );
      uri.end( uri.end() + 1 );
    }

    ringing.addheader( projectsippacket::To,
                uri.str() + ";" + *this->totag );
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
void projectsipdialog::answer( std::string body )
{
  if( !this->callanswered )
  {
    this->retries = 3;
    this->send200( body );
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
void projectsipdialog::send200( std::string body, bool final )
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
                      std::to_string( body.length() ) );

  if( body.length() > 0 )
  {
    ok.addheader( projectsippacket::Content_Type, "application/sdp" );
    ok.setbody( body.c_str() );
  }

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
Function: hangup
Purpose: Hangup if ansered (BYE) or send reason for error code.
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::hangup( void )
{
  this->callhungup = true;
  this->endat = std::time( nullptr );

  if( this->callanswered )
  {
    this->sendbye();
  }
  else
  {
    this->retries = 3;

    this->senderror();
    this->nextstate = std::bind( &projectsipdialog::waitforackanddie, this, std::placeholders::_1 );

      /* Wait up to DIALOGACKTIMEOUT seconds before we close this dialog */
    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
      std::bind( &projectsipdialog::ontimeoutenddialog, this, std::placeholders::_1 ) );
  }
}

/*******************************************************************************
Function: senderror
Purpose: Send an error
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::senderror( void )
{
  projectsippacket ok;

  ok.setstatusline( this->errorcode, errorreason );
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

  this->nextstate = std::bind( &projectsipdialog::waitforackanddie, this, std::placeholders::_1 );

  this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
      std::bind( &projectsipdialog::resenderror, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: resenderror
Purpose: Send a 200
Updated: 06.02.2019
*******************************************************************************/
void projectsipdialog::resenderror( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    if( 0 == this->retries )
    {
      this->untrack();
      return;
    }

    this->senderror();
    this->retries--;

    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
        std::bind( &projectsipdialog::resenderror, this, std::placeholders::_1 ) );
  }
}

/*******************************************************************************
Function: sendbye
Purpose: Send a BYE
Updated: 08.02.2019
*******************************************************************************/
void projectsipdialog::sendbye( void )
{
  if( !this->lastpacket )
  {
    /* We must have a previous packet to send a BYE */
    return;
  }

  projectsippacket bye;

  bye.setrequestline( projectsippacket::BYE, "" );
  bye.addviaheader( projectsipconfig::gethostname(), this->lastpacket.get() );

  bye.addheader( projectsippacket::To,
              this->lastpacket->getheader( projectsippacket::To ) );
  bye.addheader( projectsippacket::From,
              this->lastpacket->getheader( projectsippacket::From ) );
  bye.addheader( projectsippacket::Call_ID,
              this->lastpacket->getheader( projectsippacket::Call_ID ) );
  bye.addheader( projectsippacket::CSeq,
              this->lastpacket->getheader( projectsippacket::CSeq ) );
  bye.addheader( projectsippacket::Contact,
              projectsippacket::contact( this->lastpacket->getuser().strptr(),
              stringptr( new std::string( projectsipconfig::gethostip() ) ),
              0,
              projectsipconfig::getsipport() ) );
  bye.addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  bye.addheader( projectsippacket::Content_Length,
                      "0" );

  this->lastpacket->respond( bye.strptr() );

  this->nextstate = std::bind( &projectsipdialog::waitforackanddie, this, std::placeholders::_1 );

  this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
      std::bind( &projectsipdialog::resendbye, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: sendinvite
Purpose: Send an INVITE. We will need to make a couple of decisions on how
and where to send the INVITE. For a registered INVITE this will be simpler.
Updated: 08.02.2019
*******************************************************************************/
void projectsipdialog::sendinvite( JSON::Object &request, projectwebdocument &response )
{
  std::string realm, to, from, callerid, calleridname;
  JSON::Integer maxforwards;
  bool hide = false;

  try{
    realm = JSON::as_string( request[ "domain" ] );
    to = JSON::as_string( request[ "to" ] );
    from = JSON::as_string( request[ "from" ] );

    maxforwards = JSON::as_int64( request[ "maxforwards" ] );

    JSON::Object &cid =  JSON::as_object( request[ "callerid" ] );
    callerid = JSON::as_string( cid[ "number" ] );
    calleridname = JSON::as_string( cid[ "name" ] );

    JSON::Object control = JSON::as_object( request[ "control" ] );

    hide = ( bool )JSON::as_boolean( cid[ "private" ] );
  }
  catch( const std::out_of_range& oor )
  {
    // We have not got enough params.
    this->untrack();
    response.setstatusline( 500, "Error not enough params" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    response.addheader( projectwebdocument::Content_Type, "text/json" );
    return;
  }

  this->callid = *projectsippacket::callid();
  projectsippacketptr invite = projectsippacketptr( new projectsippacket() );

  // Generate SIP URI i.e. sip:realm
  invite->setrequestline( projectsippacket::INVITE, std::string( "sip:" ) + realm );
  invite->addviaheader( projectsipconfig::gethostip() );

  invite->addremotepartyid( realm.c_str(), calleridname.c_str(), callerid.c_str(), hide );

  invite->addheader( projectsippacket::To, to );
  invite->addheader( projectsippacket::From, from );
  invite->addheader( projectsippacket::Call_ID, this->callid );
  invite->addheader( projectsippacket::CSeq, "1 INVITE" );
  invite->addheader( projectsippacket::Max_Forwards, maxforwards );

  invite->addheader( projectsippacket::Contact,
              projectsippacket::contact( stringptr( new std::string( from ) ),
              stringptr( new std::string( projectsipconfig::gethostip() ) ),
              0,
              projectsipconfig::getsipport() ) );

  invite->addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  invite->addheader( projectsippacket::Content_Length,
                      "0" );

  // Now, where do we send this request?
  // 1) are we a locally registered user
  // 2) are we a local domain - but user doesn't exist or isn't registered
  // 3) are we an external destination, i.e. we have to dns lookup.

  std::string useratdom = to;
  useratdom += "@";
  useratdom += realm;
  if( !projectsipregistration::sendtoregisteredclient( useratdom, invite ) )
  {
    projectsipdirdomain::pointer dom = projectsipdirdomain::lookupdomain( realm );
    if ( dom )
    {
      if( projectsipdirdomain::userexist( realm, to ) )
      {
        this->untrack();
        response.setstatusline( 480, "Temporarily Unavailable" );
        response.addheader( projectwebdocument::Content_Length, 0 );
        response.addheader( projectwebdocument::Content_Type, "text/json" );
        return;
      }

      this->untrack();
      response.setstatusline( 404, "User not found" );
      response.addheader( projectwebdocument::Content_Length, 0 );
      response.addheader( projectwebdocument::Content_Type, "text/json" );
      return;
    }

    // TODO - external call - now need to dns lookup etc.
  }

  // Respond to the web request.
  JSON::Object v;
  v[ "callid" ] = this->callid;
  std::string t = JSON::to_string( v );

  response.setstatusline( 200, "Ok" );
  response.addheader( projectwebdocument::Content_Length, t.size() );
  response.addheader( projectwebdocument::Content_Type, "text/json" );
  response.setbody( t.c_str() );

  this->invitepacket = invite;

  this->laststate = std::bind( &projectsipdialog::waitforinviteprogress, this, std::placeholders::_1 );
  this->nextstate = std::bind( &projectsipdialog::waitforinviteprogress, this, std::placeholders::_1 );

}


/*******************************************************************************
Function: waitforinviteprogress
Purpose: We have sent an invite - wait for something to come back.
Updated: 27.02.2019
*******************************************************************************/
void projectsipdialog::waitforinviteprogress( projectsippacketptr pk )
{
}

/*******************************************************************************
Function: resendbye
Purpose: Send a 200
Updated: 08.02.2019
*******************************************************************************/
void projectsipdialog::resendbye( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    if( 0 == this->retries )
    {
      this->untrack();
      return;
    }

    this->sendbye();
    this->retries--;

    this->waitfortimer( std::chrono::milliseconds( DIALOGACKTIMEOUT ),
        std::bind( &projectsipdialog::resendbye, this, std::placeholders::_1 ) );
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
bool projectsipdialog::updatecontrol( projectsippacketptr pk )
{
  projectsipdirdomain::pointer ptr = projectsipdirdomain::lookupdomain( this->domain );

  if( !ptr )
  {
    return false;
  }

  std::string controluri = *ptr->controlhost;
  controluri += "/dialog/" + pk->getheader( projectsippacket::Call_ID ).str();

  projectwebdocumentptr d = projectwebdocumentptr( new projectwebdocument() );
  d->setrequestline( projectwebdocument::PUT, controluri );

  JSON::Object v;

  v[ "callid" ] = pk->getheader( projectsippacket::Call_ID ).str();
  v[ "domain" ] = pk->geturihost().str();
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

  return true;
}

/*******************************************************************************
Function: httpcallback
Purpose: We have made a request to our control server. This is a do nothing if
no error occurred.
Updated: 23.01.2019
*******************************************************************************/
void projectsipdialog::httpcallback( int errorcode )
{
  if( finished )
  {
    this->untrack();
    return;
  }

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
Function: httpput
Purpose: Handle a request from our web server.
Updated: 30.01.2019
*******************************************************************************/
void projectsipdialog::httpput( stringvector &path, JSON::Value &body, projectwebdocument &response )
{
  JSON::Object b = JSON::as_object( body );

  if( path.size() < 1 )
  {
    response.setstatusline( 404, "Not found" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }

  std::string action = path[ 1 ];

  if( action == "invite" )
  {
    projectsipdialog::pointer ptr;
    ptr = projectsipdialog::create();
    dialogs.insert( ptr );
    ptr->sendinvite( b, response );
    return;
  }

  if( !b.has_key( "callid" ) )
  {
    response.setstatusline( 404, "Not found" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }

  projectsipdialogs::index< projectsipdialogcallid >::type::iterator it;
  it = dialogs.get< projectsipdialogcallid >().find( JSON::as_string( b[ "callid" ] ) );
  if( dialogs.get< projectsipdialogcallid >().end() == it )
  {
    response.setstatusline( 404, "Not found" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }

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

    std::string sdp = "";
    if ( b.has_key( "sdp" ) )
    {
      sdp = *( jsontosdp( JSON::as_object( b[ "sdp" ] ) ) );
    }

    (*it)->answer( sdp );
  }
  else if( action == "hangup" )
  {
    response.setstatusline( 200, "Ok" );

    (*it)->retries = 3;

    if ( b.has_key( "reason" ) )
    {
      (*it)->errorreason = JSON::as_string( b[ "reason" ] );
    }
    else
    {
      (*it)->errorreason = "";
    }

    if ( b.has_key( "code" ) )
    {
      (*it)->errorcode = JSON::as_int64( b[ "code" ] );
    }
    else
    {
      /* Default - not found */
      (*it)->errorcode = 404;
    }

    (*it)->hangup();
  }
  else
  {
    response.setstatusline( 404, "Not found" );
  }

  response.addheader( projectwebdocument::Content_Length, 0 );
}
