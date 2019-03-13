
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
  registered( std::time( nullptr ) ),
  isregistered( false ),
  expires( boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( DEFAULTSIPEXPIRES ) ),
  nextping( boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( OPTIONSPINGFREQUENCY ) ),
  outstandingping( 0 ),
  timer( io_service ),
  optionscseq( 0 ),
  controlrequest( projecthttpclient::create( io_service ) )
{
}


/*******************************************************************************
Function: regstart
Purpose: Our start state. We wait to receive a REGISTER packet.
Updated: 10.01.2019
*******************************************************************************/
void projectsipregistration::regstart( projectsippacketptr pk )
{  
  /*
    What is the expires the client is requesting.
  */
  int expires = pk->getexpires();

  if( -1 == expires )
  {
    expires = DEFAULTSIPEXPIRES;
  }

  if( 0 == expires || DEFAULTSIPEXPIRES > expires )
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

    std::string via = projectsipconfig::gethostip() + 
                    std::string( ":" ) + 
                    std::to_string( projectsipconfig::getsipport() );

    toobrief.addviaheader( via.c_str(), pk.get() );

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

  std::string via = projectsipconfig::gethostip() + 
                    std::string( ":" ) + 
                    std::to_string( projectsipconfig::getsipport() );

  this->authrequest->addviaheader( via.c_str(), pk.get() );

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
}


/*******************************************************************************
Function: regwaitauth
Purpose: The next packet we will receive is a REGISTER with credentials.
Updated: 10.01.2019
*******************************************************************************/
void projectsipregistration::regwaitauth( projectsippacketptr pk )
{
  this->currentpacket = pk;

  stringptr password = projectsipdirdomain::lookuppassword( 
                pk->geturihost(), 
                pk->getuser() );

  if( !this->authrequest )
  {
    this->regstart( pk );
    return;
  }

  if( !password ||
        !this->currentpacket->checkauth( this->authrequest.get(), password ) )
  {
    projectsippacket failedauth;

    failedauth.setstatusline( 403, "Failed auth" );

    std::string via = projectsipconfig::gethostip() + 
                    std::string( ":" ) + 
                    std::to_string( projectsipconfig::getsipport() );

    failedauth.addviaheader( via.c_str(), this->currentpacket.get() );

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

  if( false == this->isregistered )
  {
    this->registered = std::time( nullptr );
    this->isregistered = true;
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

  std::string via = projectsipconfig::gethostip() + 
                    std::string( ":" ) + 
                    std::to_string( projectsipconfig::getsipport() );

  p.addviaheader( via.c_str(), this->currentpacket.get() );
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

  this->timer.expires_after( std::chrono::seconds( OPTIONSPINGFREQUENCY ) );
  this->timer.async_wait( std::bind( &projectsipregistration::timerhandler, this, std::placeholders::_1 ) );

  /* update control */
  projectsipdirdomain::pointer ptr = projectsipdirdomain::lookupdomain( pk->geturihost() );
  std::string controluri = *ptr->controlhost;
  
  projectwebdocumentptr d = projectwebdocumentptr( new projectwebdocument() );
  d->setrequestline( projectwebdocument::POST, controluri );

  JSON::Object v;
  v[ "domain" ] = pk->geturihost().str();
  v[ "user" ] = this->user;

  std::string t = JSON::to_string( v );
  d->addheader( projectwebdocument::Content_Length, t.length() );
  d->addheader( projectwebdocument::Content_Type, "text/json" );
  d->setbody( t.c_str() );

  this->controlrequest->asyncrequest( d, std::bind( &projectsipregistration::httpcallback, this, std::placeholders::_1 ) );

  this->authrequest.reset();
}

/*******************************************************************************
Function: httpcallback
Purpose: May be look into more detail failures.
Updated: 22.02.2019
*******************************************************************************/
void projectsipregistration::httpcallback( int errorcode )
{

}

void projectsipregistration::httpcallbackanddie( int errorcode )
{
  projectsipregistrations::index< regindexuser >::type::iterator userit;
  userit = regs.get< regindexuser >().find( this->user );

  if( regs.get< regindexuser >().end() != userit )
  {
    regs.get< regindexuser >().erase( userit );
  }
}

/*******************************************************************************
Function: expire
Purpose: Expire(remove) this entry.
Updated: 17.01.2019
*******************************************************************************/
void projectsipregistration::expire( void )
{
  /* update control */
  projectsipdirdomain::pointer ptr = projectsipdirdomain::lookupdomain( this->authacceptpacket->geturihost() );
  std::string controluri = *ptr->controlhost;
  
  projectwebdocumentptr d = projectwebdocumentptr( new projectwebdocument() );
  d->setrequestline( projectwebdocument::DELETE, controluri );

  JSON::Object v;
  v[ "domain" ] = this->authacceptpacket->geturihost().str();
  v[ "user" ] = this->user;

  std::string t = JSON::to_string( v );
  d->addheader( projectwebdocument::Content_Length, t.length() );
  d->addheader( projectwebdocument::Content_Type, "text/json" );
  d->setbody( t.c_str() );

  this->controlrequest->asyncrequest( d, std::bind( &projectsipregistration::httpcallback, this, std::placeholders::_1 ) );
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

  sipuri contacturi( this->authacceptpacket->getheader( projectsippacket::Contact ) );

  request.setrequestline( projectsippacket::OPTIONS, contacturi.uri.str() );

  std::string via = projectsipconfig::gethostip() + 
                    std::string( ":" ) + 
                    std::to_string( projectsipconfig::getsipport() );

  request.addviaheader( via.c_str() );

  request.addheader( projectsippacket::Max_Forwards, "70" );
  request.addheader( projectsippacket::To,
                      this->authacceptpacket->getheader( projectsippacket::To ) );
  
  std::string from = "<sip:";
  from += this->user;
  from += ">;";
  from += *projectsippacket::tag();

  request.addheader( projectsippacket::From,
                      from );
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
  request.addheader( projectsippacket::Content_Length,
                      "0" );

  this->authacceptpacket->respond( request.strptr() );

  this->outstandingping++;
}

/*******************************************************************************
Function: handleresponse
Purpose: We received a SIP 200 OK in response to our OPTIONS request. We only 
(at the moment) senauthrequestd out options.
Updated: 17.01.2019
*******************************************************************************/
void projectsipregistration::handleresponse( projectsippacketptr pk )
{
  if( !this->authacceptpacket )
  {
    return;
  }

  if( 0 != pk->getheader( projectsippacket::CSeq )
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

  if( pk->isrequest() )
  {
    if( pk->hasheader( projectsippacket::Authorization ) && r->authrequest )
    {
      r->regwaitauth( pk );
      return;
    }
    
    r->regstart( pk );
    return;
  }

  r->handleresponse( pk );
}


/*******************************************************************************
Function: httpget
Purpose: Handle a request from our web server.
Updated: 22.01.2019
*******************************************************************************/
void projectsipregistration::httpget( stringvector &path, projectwebdocument &response )
{
  if( 2 == path.size() )
  {

    std::string domain = path[ 1 ];
    // Report for a domain
    JSON::Array a;
    int registeredcount = 0;

    projectsipdirdomain::pointer d = projectsipdirdomain::lookupdomain( domain );
    if( !d )
    {
      response.setstatusline( 404, "Domain not found" );
      response.addheader( projectwebdocument::Content_Type, "text/json" );
      return;
    }
    
    projectsipdirusers::iterator uit;
    for( uit = d->users.begin(); uit != d->users.end(); uit++ )
    {
      JSON::Object v;

      v[ "username" ] = uit->first;

      std::string s = uit->first + "@" + domain;
      projectsipregistrationptr r;
      projectsipregistrations::index< regindexuser >::type::iterator it = regs.get< regindexuser >().find( s );

      if ( it == regs.get< regindexuser >().end() || !( *it )->isregistered )
      {
        v[ "registered" ] = ( JSON::Bool ) false;
      }
      else
      {
        v[ "registered" ] = ( JSON::Bool ) true;
        v[ "agent" ] = ( *it )->authacceptpacket->getheader( projectsippacket::User_Agent ).str();
        
        JSON::Object r;
        r[ "host" ] = ( *it )->authacceptpacket->getremotehost();
        r[ "port" ] = ( JSON::Integer ) ( *it )->authacceptpacket->getremoteport();
        v[ "remote" ] = r;

        JSON::Object e;
        e[ "registered" ] = boost::numeric_cast< JSON::Integer >( ( *it )->registered );
        v[ "epochs" ] = e;
        registeredcount++;
      }

      a.values.push_back( v );
    }

    JSON::Object r;
    r[ "domain" ] = domain;
    r[ "count" ] = ( JSON::Integer ) d->users.size();
    r[ "registered" ] = ( JSON::Integer ) registeredcount;
    r[ "users" ] = a;

    std::string t = JSON::to_string( r );

    response.setstatusline( 200, "Ok" );
    response.addheader( projectwebdocument::Content_Length, t.length() );
    response.addheader( projectwebdocument::Content_Type, "text/json" );
    response.setbody( t.c_str() );

  }
  else
  {
    JSON::Object v;
    v[ "count" ] = ( JSON::Integer ) regs.size();
    std::string t = JSON::to_string( v );

    response.setstatusline( 200, "Ok" );
    response.addheader( projectwebdocument::Content_Length, t.length() );
    response.addheader( projectwebdocument::Content_Type, "text/json" );
    response.setbody( t.c_str() );
  }
}

/*******************************************************************************
Function: sendtoregisteredclient
Purpose: Forward a packet onto a registered client
Updated: 20.02.2019
*******************************************************************************/
bool projectsipregistration::sendtoregisteredclient( std::string &user, projectsippacketptr pk )
{
  projectsipregistrations::index< regindexuser >::type::iterator it = regs.get< regindexuser >().find( user );
  if( it != regs.get< regindexuser >().end() )
  {
    projectsipregistrationptr r = *it;
    if( r->authacceptpacket )
    {
      r->authacceptpacket->respond( pk->strptr() );
      return true;
    }
  }

  return false;
}


