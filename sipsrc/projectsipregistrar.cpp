
#include <boost/property_tree/ptree.hpp>
#include "json.hpp"

#include "projectsipregistrar.h"
#include "projectsipconfig.h"
#include "projectsipdirectory.h"
#include "test.h"

/* Debuggng */
#include <iostream>

static projectsipregistrations regs;

extern boost::asio::io_service io_service;

/*******************************************************************************
Function: projectsipregistration
Purpose: Constructor
Updated: 18.12.2018
*******************************************************************************/
projectsipregistration::projectsipregistration( std::string u ) :
  user( u ),
  expires( boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( DEFAULTSIPEXPIRES ) ),
  nextping( boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( OPTIONSPINGFREQUENCY ) ),
  outstandingping( 0 ),
  nextstate( std::bind( &projectsipregistration::regstart, this, std::placeholders::_1 ) ),
  laststate( std::bind( &projectsipregistration::regstart, this, std::placeholders::_1 ) ),
  timer( io_service ),
  optionscseq( 0 )
{
}


/*******************************************************************************
Function: regstart
Purpose: Our start state. We wait to receive a REGISTER packet.
Updated: 10.01.2019
*******************************************************************************/
void projectsipregistration::regstart( projectsippacketptr pk )
{
  if( !pk->isrequest() )
  {
    this->handleresponse( pk );
    return;
  }
  
  /*
    What is the expires the client is requesting.
  */
  int expires = pk->getexpires();
  if( -1 == expires )
  {
    expires = DEFAULTSIPEXPIRES;
  }

  if( 0 == expires && DEFAULTSIPEXPIRES > expires )
  {
    /*
     If a UA receives a 423 (Interval Too Brief) response, it MAY retry
     the registration after making the expiration interval of all contact
     addresses in the REGISTER request equal to or greater than the
     expiration interval within the Min-Expires header field of the 423
     (Interval Too Brief) response.
    */
    projectsippacket toobrief;

    toobrief.setstatusline( 423, "Interval Too Brief" );
    toobrief.addviaheader( projectsipconfig::gethostname(), pk.get() );

    toobrief.addheader( projectsippacket::Min_Expires, 
                DEFAULTSIPEXPIRES );

    toobrief.addheader( projectsippacket::To,
                pk->getheader( projectsippacket::To ) );
    toobrief.addheader( projectsippacket::From,
                pk->getheader( projectsippacket::From ) );
    toobrief.addheader( projectsippacket::Call_ID,
                pk->getheader( projectsippacket::Call_ID ) );
    toobrief.addheader( projectsippacket::CSeq,
                pk->getheader( projectsippacket::CSeq ) );
    toobrief.addheader( projectsippacket::Contact,
                projectsippacket::contact( pk->getuser().strptr(), 
                stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                0, 
                projectsipconfig::getsipport() ) );
    toobrief.addheader( projectsippacket::Allow,
                "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    toobrief.addheader( projectsippacket::Content_Type,
                "application/sdp" );
    toobrief.addheader( projectsippacket::Content_Length,
                        "0" );

    pk->respond( toobrief.strptr() );

    return;
  }

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

  this->laststate = std::bind( &projectsipregistration::regstart, this, std::placeholders::_1 );
  this->nextstate = std::bind( &projectsipregistration::regwaitauth, this, std::placeholders::_1 );
}


/*******************************************************************************
Function: regwaitauth
Purpose: The next packet we will receive is a REGISTER with credentials.
Updated: 10.01.2019
*******************************************************************************/
void projectsipregistration::regwaitauth( projectsippacketptr pk )
{
  if( !pk->isrequest() )
  {
    this->handleresponse( pk );
    return;
  }

  this->currentpacket = pk;

  stringptr password = projectsipdirectory::lookuppassword( 
                pk->geturihost(), 
                pk->getuser() );


  if( !password ||
        !this->currentpacket->checkauth( this->authrequest.get(), password ) )
  {
    projectsippacket failedauth;

    failedauth.setstatusline( 403, "Failed auth" );
    failedauth.addviaheader( projectsipconfig::gethostname(), this->currentpacket.get() );

    failedauth.addheader( projectsippacket::To,
                this->currentpacket->getheader( projectsippacket::To ) );
    failedauth.addheader( projectsippacket::From,
                this->currentpacket->getheader( projectsippacket::From ) );
    failedauth.addheader( projectsippacket::Call_ID,
                this->currentpacket->getheader( projectsippacket::Call_ID ) );
    failedauth.addheader( projectsippacket::CSeq,
                this->currentpacket->getheader( projectsippacket::CSeq ) );
    failedauth.addheader( projectsippacket::Contact,
                projectsippacket::contact( this->currentpacket->getuser().strptr(), 
                stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                0, 
                projectsipconfig::getsipport() ) );
    failedauth.addheader( projectsippacket::Allow,
                "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    failedauth.addheader( projectsippacket::Content_Type,
                "application/sdp" );
    failedauth.addheader( projectsippacket::Content_Length,
                        "0" );

    this->currentpacket->respond( failedauth.strptr() );

    return;
  }

  /* We have authd. Get an iterator into our container so that we can modify. */
  projectsipregistrations::index< regindexuser >::type::iterator userit;
  userit = regs.get< regindexuser >().find( this->user );

  /* Modify our entry in our container. If expires == 0 then we remove bindings. */
  int expires = this->currentpacket->getexpires();
  if( 0 == expires )
  {
    if( regs.get< regindexuser >().end() != userit )
    {
      regs.get< regindexuser >().erase( userit );
    }
    return;
  }

  if( -1 == expires )
  {
    expires = DEFAULTSIPEXPIRES;
  }

  if( this->authrequest->getheader( projectsippacket::CSeq ) != 
        this->currentpacket->getheader( projectsippacket::CSeq ) )
  {
    // This should not happen. Although, if 2 different clients managed 
    // to do this at the same time it could. This will make one stall
    // (and retry).
    return;
  }

  /*
    Now send a 200 
    The registrar returns a 200 (OK) response.  The response MUST
    contain Contact header field values enumerating all current
    bindings.  Each Contact value MUST feature an "expires"
    parameter indicating its expiration interval chosen by the
    registrar.  The response SHOULD include a Date header field.
  */
  projectsippacket p;

  p.setstatusline( 200, "Ok" );

  p.addviaheader( projectsipconfig::gethostname(), this->currentpacket.get() );
  p.addheader( projectsippacket::To,
              this->currentpacket->getheader( projectsippacket::To ) );
  p.addheader( projectsippacket::From,
              this->currentpacket->getheader( projectsippacket::From ) );
  p.addheader( projectsippacket::Call_ID,
              this->currentpacket->getheader( projectsippacket::Call_ID ) );
  p.addheader( projectsippacket::CSeq,
              this->currentpacket->getheader( projectsippacket::CSeq ) );

  p.addheader( projectsippacket::Contact,
              projectsippacket::contact( this->currentpacket->getuser().strptr(), 
                stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                expires, 
                projectsipconfig::getsipport() ) );
  p.addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  p.addheader( projectsippacket::Content_Type,
              "application/sdp" );
  p.addheader( projectsippacket::Content_Length,
              "0" );

  this->currentpacket->respond( p.strptr() );
  this->authacceptpacket = this->currentpacket;

  this->expires = boost::posix_time::second_clock::local_time() + 
                      boost::posix_time::seconds( expires );

  this->laststate = std::bind( &projectsipregistration::regwaitauth, this, std::placeholders::_1 );
  this->nextstate = std::bind( &projectsipregistration::regstart, this, std::placeholders::_1 );

  this->timer.expires_after( std::chrono::seconds( OPTIONSPINGFREQUENCY ) );
  this->timer.async_wait( std::bind( &projectsipregistration::timerhandler, this, std::placeholders::_1 ) );
}

/*******************************************************************************
Function: expire
Purpose: Expire(remove) this entry.
Updated: 17.01.2019
*******************************************************************************/
void projectsipregistration::expire( void )
{
  projectsipregistrations::index< regindexuser >::type::iterator userit;
  userit = regs.get< regindexuser >().find( this->user );

  if( regs.get< regindexuser >().end() != userit )
  {
    regs.get< regindexuser >().erase( userit );
  }
}

/*******************************************************************************
Function: timer
Purpose: Our timer callback.
Updated: 17.01.2019
*******************************************************************************/
void projectsipregistration::timerhandler( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    if( boost::posix_time::second_clock::local_time() > this->expires )
    {
      this->expire();
      return;
    }

    if( this->outstandingping > OPTIONSMAXFAILCOUNT )
    {
      this->expire();
      return;
    }
    this->sendoptions();

    this->timer.expires_after( std::chrono::seconds( OPTIONSPINGFREQUENCY ) );
    this->timer.async_wait( std::bind( &projectsipregistration::timerhandler, this, std::placeholders::_1 ) );
  }
}

/*******************************************************************************
Function: sendoptions
Purpose: Send an options packet. Construction of an OPTIONS packet 11.2.
Updated: 17.01.2019
*******************************************************************************/
void projectsipregistration::sendoptions( void )
{
  projectsippacket request;
  request.setrequestline( projectsippacket::OPTIONS, this->user );
  request.addviaheader( projectsipconfig::gethostname(), this->authacceptpacket.get() );

  request.addheader( projectsippacket::Max_Forwards, "70" );
  request.addheader( projectsippacket::To,
                      this->authacceptpacket->getheader( projectsippacket::To ) );
  request.addheader( projectsippacket::From,
                      this->authacceptpacket->getheader( projectsippacket::From ) );
  request.addheader( projectsippacket::Call_ID,
                      projectsippacket::callid() );
  request.addheader( projectsippacket::CSeq,
                      std::to_string( this->optionscseq ) + " OPTIONS" );
  request.addheader( projectsippacket::Contact,
                      projectsippacket::contact( this->authacceptpacket->getuser().strptr(), 
                      stringptr( new std::string( projectsipconfig::gethostip() ) ), 
                      0, 
                      projectsipconfig::getsipport() ) );
  request.addheader( projectsippacket::Allow,
                      "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  request.addheader( projectsippacket::Content_Type,
                      "application/sdp" );
  request.addheader( projectsippacket::Content_Length,
                      "0" );

  this->authacceptpacket->respond( request.strptr() );

  this->outstandingping++;
}

/*******************************************************************************
Function: handleresponse
Purpose: We received a SIP 200 OK in response to our OPTIONS request. We only 
(at the moment) send out options.
Updated: 17.01.2019
*******************************************************************************/
void projectsipregistration::handleresponse( projectsippacketptr pk )
{
  if( 0 != this->authacceptpacket->getheader( projectsippacket::CSeq )
                .find( "OPTIONS" ).length() )
  {
    this->optionscseq++;
    this->outstandingping = 0;
  }
}

/*******************************************************************************
Function: processsippacket
Purpose: Process a REGISTER packet. To get into this function
the packet must have already been checked as a REGISTER packet and has all 
of the required headers.
Updated: 10.01.2019
*******************************************************************************/
void projectsipregistration::registrarsippacket( projectsippacketptr pk )
{
  /*
    Work out who this registration is for.
  */
  substring uri = pk->getrequesturi();
  if( uri.length() < 4 )
  {
    return;
  }

  std::string s = pk->getuserhost();

  projectsipregistrationptr r;
  projectsipregistrations::index< regindexuser >::type::iterator it = regs.get< regindexuser >().find( s );
  if( it == regs.get< regindexuser >().end() )
  {
    r = projectsipregistrationptr( new projectsipregistration( s ) );
    regs.insert( r );
  }
  else
  {
    r = *it;
  }

  r->nextstate( pk );
}


/*******************************************************************************
Function: httpget
Purpose: Handle a request from our web server.
Updated: 22.01.2019
*******************************************************************************/
void projectsipregistration::httpget( stringvector &path, projectwebdocument &response )
{
  JSON::Object v;
  v[ "count" ] = ( JSON::Integer ) regs.size();
  std::string t = JSON::to_string( v );

  response.setstatusline( 200, "Ok" );
  response.addheader( projectwebdocument::Content_Length, t.length() );
  response.addheader( projectwebdocument::Content_Type, "text/json" );
  response.setbody( t.c_str() );
}





