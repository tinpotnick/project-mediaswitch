


#include "projectsipregistrar.h"
#include "projectsipconfig.h"
#include "projectsipdirectory.h"
#include "test.h"

static projectsipregistrations regs;

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
  laststate( std::bind( &projectsipregistration::regstart, this, std::placeholders::_1 ) )
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
  int expires = DEFAULTSIPEXPIRES;
  substring ex;
  if( pk->hasheader( projectsippacket::Expires ) )
  {
    ex = pk->getheader( projectsippacket::Expires );
  }
  else
  {
    ex = pk->getheaderparam( projectsippacket::Contact, "expires" );
  }

  if( 0 != ex.end() )
  {
    expires = ex.toint();
    if( -1 == expires )
    {
      expires = DEFAULTSIPEXPIRES;
    }
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
    projectsippacket response;
    this->lastpacket = projectsippacketptr( new projectsippacket() );

    this->lastpacket->setstatusline( 423, "Interval Too Brief" );
    this->lastpacket->addviaheader( ::cnf.gethostname(), pk.get() );

    this->lastpacket->addheader( projectsippacket::Min_Expires, 
                DEFAULTSIPEXPIRES );

    this->lastpacket->addheader( projectsippacket::To,
                this->lastpacket->getheader( projectsippacket::To ) );
    this->lastpacket->addheader( projectsippacket::From,
                this->lastpacket->getheader( projectsippacket::From ) );
    this->lastpacket->addheader( projectsippacket::Call_ID,
                this->lastpacket->getheader( projectsippacket::Call_ID ) );
    this->lastpacket->addheader( projectsippacket::CSeq,
                this->lastpacket->getheader( projectsippacket::CSeq ) );
    this->lastpacket->addheader( projectsippacket::Contact,
                this->lastpacket->getheader( projectsippacket::Contact ) );
    this->lastpacket->addheader( projectsippacket::Allow,
                "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    this->lastpacket->addheader( projectsippacket::Content_Type,
                "application/sdp" );
    this->lastpacket->addheader( projectsippacket::Content_Length,
                        "0" );

    this->lastpacket->respond( this->lastpacket->strptr() );

    return;
  }

  if( false == pk->hasheader( projectsippacket::Authorization ) )
  {
    projectsippacket response;
    this->lastpacket = projectsippacketptr( new projectsippacket() );

    this->lastpacket->setstatusline( 401, "Unauthorized" );
    this->lastpacket->addviaheader( ::cnf.gethostname(), pk.get() );

    this->lastpacket->addwwwauthenticateheader( pk.get() );

    this->lastpacket->addheader( projectsippacket::To,
                        pk->getheader( projectsippacket::To ) );
    this->lastpacket->addheader( projectsippacket::From,
                        pk->getheader( projectsippacket::From ) );
    this->lastpacket->addheader( projectsippacket::Call_ID,
                        pk->getheader( projectsippacket::Call_ID ) );
    this->lastpacket->addheader( projectsippacket::CSeq,
                        pk->getheader( projectsippacket::CSeq ) );
    this->lastpacket->addheader( projectsippacket::Contact,
                        pk->getheader( projectsippacket::Contact ) );
    this->lastpacket->addheader( projectsippacket::Allow,
                        "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    this->lastpacket->addheader( projectsippacket::Content_Type,
                        "application/sdp" );
    this->lastpacket->addheader( projectsippacket::Content_Length,
                        "0" );

    pk->respond( this->lastpacket->strptr() );

    return;
  }

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
  this->currentpacket = pk;
  /* 
    Check auth
    1. Get the users password (if applicable)
    2. Check the hash.
  */
  projectsipdirectory::lookup( 
                pk->geturihost(), 
                pk->gettouser(), 
                std::bind( 
                  &projectsipregistration::regcompleteauth, 
                  this, 
                  std::placeholders::_1 ) );
}

/*******************************************************************************
Function: regcompleteauth
Purpose: This should be called after we have looked up our password for this
realm/user combo.
Updated: 15.01.2019
*******************************************************************************/
void projectsipregistration::regcompleteauth( stringptr password )
{
  if( !this->currentpacket->checkauth( this->currentpacket.get(), password ) )
  {
    // Failed auth
    return;
  }

  /* We have authd. Get an iterator into our container so that we can modify. */
  projectsipregistrations::index< regindexuser >::type::iterator userit;
  userit = regs.get< regindexuser >().find( this->user );

  /* Modify our entry in our container. If expires == 0 then we remove bindings. */
  if( 0 == this->currentpacket->getheaderparam( projectsippacket::Contact, "expires" ).toint() )
  {
    if( regs.get< regindexuser >().end() != userit )
    {
      regs.get< regindexuser >().erase( userit );
    }
  }
  else
  {
    /* we either nee to create or modify our entry */
    if( regs.get< regindexuser >().end() == userit )
    {
    }
    else
    {
      regs.get< regindexuser >().modify( 
            userit, 
            projectsipupdateexpires( 
              ( *userit )->expires + 
              boost::posix_time::seconds( DEFAULTSIPEXPIRES ) 
            ) );
    }
  }

  /*
    Now send a 200 
    The registrar returns a 200 (OK) response.  The response MUST
    contain Contact header field values enumerating all current
    bindings.  Each Contact value MUST feature an "expires"
    parameter indicating its expiration interval chosen by the
    registrar.  The response SHOULD include a Date header field.
  */
  projectsippacket response;
  this->lastpacket = projectsippacketptr( new projectsippacket() );

  this->lastpacket->setstatusline( 200, "Ok" );
  this->lastpacket->addviaheader( ::cnf.gethostname(), this->lastpacket.get() );

  this->lastpacket->addheader( projectsippacket::To,
              this->currentpacket->getheader( projectsippacket::To ) );
  this->lastpacket->addheader( projectsippacket::From,
              this->currentpacket->getheader( projectsippacket::From ) );
  this->lastpacket->addheader( projectsippacket::Call_ID,
              this->currentpacket->getheader( projectsippacket::Call_ID ) );
  this->lastpacket->addheader( projectsippacket::CSeq,
              this->currentpacket->getheader( projectsippacket::CSeq ) );
  this->lastpacket->addheader( projectsippacket::Contact,
              this->currentpacket->getheader( projectsippacket::Contact ) );
  this->lastpacket->addheader( projectsippacket::Allow,
              "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  this->lastpacket->addheader( projectsippacket::Content_Type,
              "application/sdp" );
  this->lastpacket->addheader( projectsippacket::Content_Length,
              "0" );

  this->currentpacket->respond( this->lastpacket->strptr() );

}


/*******************************************************************************
Function: processsippacket
Purpose: Process a REGISTER packet. To get into this function
the packet must have already been checked as a REGISTER packet and has all 
of the required headers.
Updated: 10.101.2019
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

  std::string s;
  s.reserve( DEFAULTHEADERLINELENGTH );
  s = pk->gettouser().str();
  s += '@';
  s += pk->geturihost().str();

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


#ifdef TESTCODE
/*******************************************************************************
Function: testregs
Purpose: Test our registrar code.
Updated: 18.12.2018
*******************************************************************************/
void testregs( void )
{
  projectsipregistrationptr r1( new projectsipregistration( "1000@bling.babblevoice.com" ) );
  projectsipregistrationptr r2( new projectsipregistration( "1001@bling.babblevoice.com" ) );

  regs.clear();
  regs.insert( r1 );
  regs.insert( r2 );

  projecttest( regs.size(), 2, "We should have 2 registrations." );

  projectsipregistrations::index< regindexuser >::type::iterator it = regs.get< regindexuser >().find( r1->user );
  projecttest( ( *it )->user, "1000@bling.babblevoice.com", "Bad user." );

  it = regs.get< regindexuser >().find( "1005@bling.babblevoice.com" );
  projecttestp( it, regs.get< regindexuser >().end(), "Should not be there." );

  /* Our first entry should be the first item */
  it = regs.begin();
  projecttestp( ( *it )->user, "1000@bling.babblevoice.com", "Should be 1000@bling.babblevoice.com." );

  projectsipregistrations::index< regindexexpires >::type::iterator expiresit;
  expiresit = regs.get< regindexexpires >().begin();

  std::cout << "Reorder test" << std::endl;
  std::cout << "User: " << (*expiresit)->user << ": " << (*expiresit)->expires << std::endl;

  //( *expiresit )->expires += boost::posix_time::seconds( DEFAULTSIPEXPIRES );

  regs.get< regindexexpires >().modify( expiresit, 
          projectsipupdateexpires( 
            ( *expiresit )->expires + 
            boost::posix_time::seconds( DEFAULTSIPEXPIRES ) 
          ) );

  projectsipregistrationptr r3;
  r3 = *expiresit;
  
  //regs.get< regindexexpires >().replace(  expiresit, r3  );

  expiresit = regs.get< regindexexpires >().begin();
  std::cout << "New: " << (*expiresit)->user << ": " << (*expiresit)->expires <<  std::endl;
  expiresit++;
  std::cout << "New back: " << (*expiresit)->user << ": " << (*expiresit)->expires <<  std::endl;


  if( expiresit != regs.get< regindexexpires >().end() )
  {
    projectsipregistrationptr ptr = *expiresit;
    ptr->expires = boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( 10 );

    regs.get< regindexexpires >().replace(  expiresit, ptr  );
  }


  std::cout << "All SIP registration tests passed" << std::endl;
}



#endif /* TESTCODE */


