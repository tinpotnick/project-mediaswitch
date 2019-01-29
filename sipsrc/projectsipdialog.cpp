

#include "projectsipdialog.h"
#include "projectsipconfig.h"
#include "projectsipdirectory.h"

#include "json.hpp"


static projectsipdialogs dialogs;

extern boost::asio::io_service io_service;

/*******************************************************************************
Function: projectsipdialog
Purpose: Constructor
Updated: 23.01.2019
*******************************************************************************/
projectsipdialog::projectsipdialog() :
  laststate( std::bind( &projectsipdialog::invitestart, this, std::placeholders::_1 ) ),
  nextstate( std::bind( &projectsipdialog::invitestart, this, std::placeholders::_1 ) ),
  timer( io_service )
{
}

/*******************************************************************************
Function: projectsipdialog
Purpose: Destructor
Updated: 23.01.2019
*******************************************************************************/
projectsipdialog::~projectsipdialog()
{
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
    if( projectsippacket::RESPONSE == pk->getmethod() )
    {
      return false;
    }
    // New dialog
    ptr = projectsipdialog::create();
    ptr->callid = callid;
    dialogs.insert( ptr );
  }
  else
  {
    ptr = *it;
  }

  ptr->nextstate( pk );
  return true;
}

/*******************************************************************************
Function: timerhandler
Purpose: Timer handler
Updated: 29.01.2019
*******************************************************************************/
void projectsipdialog::timerhandler( const boost::system::error_code& error )
{
  this->untrack();
}

/*******************************************************************************
Function: invitestart
Purpose: The start of a dialog state machine. We check we have a valid INVITE
and if we need to authenticate the user.
Updated: 23.01.2019
*******************************************************************************/
void projectsipdialog::invitestart( projectsippacketptr pk )
{
  this->lastpacket = pk;

  /* TODO - handle time outs more effectivly */
  this->timer.expires_after( std::chrono::seconds( 3600 ) );
  this->timer.async_wait( std::bind( &projectsipdialog::timerhandler, this, std::placeholders::_1 ) );

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

  this->passtocontrol( pk );
  this->nextstate =  std::bind( &projectsipdialog::donetrying, this, std::placeholders::_1 );
}

/*******************************************************************************
Function: passtocontrol
Purpose: Send a 100 Trying and tell control.
Updated: 25.01.2019
*******************************************************************************/
void projectsipdialog::passtocontrol( projectsippacketptr pk )
{
  projectsippacket trying;
  trying.setstatusline( 100, "Trying" );
  trying.addviaheader( projectsipconfig::gethostname(), pk.get() );

  trying.addheader( projectsippacket::To,
              pk->getheader( projectsippacket::To ) );
  trying.addheader( projectsippacket::From,
              pk->getheader( projectsippacket::From ) );
  trying.addheader( projectsippacket::Call_ID,
              pk->getheader( projectsippacket::Call_ID ) );
  trying.addheader( projectsippacket::CSeq,
              pk->getheader( projectsippacket::CSeq ) );
  trying.addheader( projectsippacket::Contact,
              projectsippacket::contact( pk->getuser().strptr(), 
              stringptr( new std::string( projectsipconfig::gethostip() ) ), 
              0, 
              projectsipconfig::getsipport() ) );
  trying.addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  trying.addheader( projectsippacket::Content_Length,
                      "0" );

  pk->respond( trying.strptr() );

  this->controlrequest = projecthttpclient::create( io_service );
  projectwebdocumentptr d = projectwebdocumentptr( new projectwebdocument );

  d->setrequestline( projectwebdocument::POST, "http://127.0.0.1/invite" );

  JSON::Object v;
  JSON::Object c;
  v[ "callid" ] = pk->getheader( projectsippacket::Call_ID ).str();
  c[ "number" ] = pk->getuser( projectsippacket::From ).str();
  c[ "name" ] = pk->getdisplayname( projectsippacket::From ).str();
  v[ "callerid" ] = c;
  v[ "to" ] = pk->getuserhost();
  v[ "from" ] = pk->getuserhost( projectsippacket::From );

  std::string t = JSON::to_string( v );
  d->addheader( projectwebdocument::Content_Length, t.length() );
  d->addheader( projectwebdocument::Content_Type, "text/json" );
  d->setbody( t.c_str() );

  this->controlrequest->asyncrequest( d, std::bind( &projectsipdialog::httpcallback, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: inviteauth
Purpose: We have requested authenticaation.
Updated: 23.01.2019
*******************************************************************************/
void projectsipdialog::inviteauth( projectsippacketptr pk )
{
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

  this->passtocontrol( pk );
}

/*******************************************************************************
Function: httpcallback
Purpose: We have made a request to our control server.
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
}

/*******************************************************************************
Function: donetrying
Purpose: We have sent a trying - so what happens next. We should send a proceeding
or a 200 ok.
Updated: 25.01.2019
*******************************************************************************/
void projectsipdialog::donetrying( projectsippacketptr pk )
{
  switch( pk->getmethod() )
  {
    case projectsippacket::INVITE:
    {
      // Send a 100 again?
      break;
    }
    case projectsippacket::BYE:
    {
      projectsippacket p;

      p.setstatusline( 200, "Ok" );

      p.addviaheader( projectsipconfig::gethostname(), pk.get() );
      p.addheader( projectsippacket::To,
                  pk->getheader( projectsippacket::To ) );
      p.addheader( projectsippacket::From,
                  pk->getheader( projectsippacket::From ) );
      p.addheader( projectsippacket::Call_ID,
                  pk->getheader( projectsippacket::Call_ID ) );
      p.addheader( projectsippacket::CSeq,
                  pk->getheader( projectsippacket::CSeq ) );

      p.addheader( projectsippacket::Contact,
                  projectsippacket::contact( pk->getuser().strptr(), 
                    stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                    0, 
                    projectsipconfig::getsipport() ) );
      p.addheader( projectsippacket::Allow,
                  "INVITE, ACK, CANCEL, OPTIONS, BYE" );
      p.addheader( projectsippacket::Content_Length,
                  "0" );

      pk->respond( p.strptr() );

      this->untrack();
      
      break;
    }
  }
}

/*******************************************************************************
Function: untrack
Purpose: Remove from our container so we can clean up.
Updated: 29.01.2019
*******************************************************************************/
void projectsipdialog::untrack( void )
{
  projectsipdialogs::index< projectsipdialogcallid >::type::iterator it;
  it = dialogs.get< projectsipdialogcallid >().find( this->callid );
  if( dialogs.get< projectsipdialogcallid >().end() != it )
  {
    dialogs.erase( it );
  }
}


