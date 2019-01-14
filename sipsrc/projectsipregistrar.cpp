


#include "projectsipregistrar.h"
#include "projectsipconfig.h"
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
#warning TODO
  /*
    do we request an hour from the client or adjust ourselves. I think we 
    should request the client to go to an hour but this will be more work.
  */

  if( false == pk->hasheader( projectsippacket::Authorization ) )
  {
    projectsippacket response;
    response.setstatusline( 401, "Unauthorized" );
    response.addviaheader( ::cnf.gethostname(), pk.get() );

    response.addwwwauthenticateheader( pk.get() );

    response.addheader( projectsippacket::To,
                        pk->getheader( projectsippacket::To ) );
    response.addheader( projectsippacket::From,
                        pk->getheader( projectsippacket::From ) );
    response.addheader( projectsippacket::Call_ID,
                        pk->getheader( projectsippacket::Call_ID ) );
    response.addheader( projectsippacket::CSeq,
                        pk->getheader( projectsippacket::CSeq ) );
    response.addheader( projectsippacket::Contact,
                        pk->getheader( projectsippacket::Contact ) );
    response.addheader( projectsippacket::Allow,
                        "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    response.addheader( projectsippacket::Content_Type,
                        "application/sdp" );
    response.addheader( projectsippacket::Content_Length,
                        "0" );

    pk->respond( response.strptr() );
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
  /* Check auth */

  /* We have authd. */
  /* Modify our entry in our container */
  projectsipregistrations::index< regindexuser >::type::iterator userit;
  userit = regs.get< regindexuser >().find( this->user );
  if( regs.get< regindexuser >().end() != userit )
  {
    regs.get< regindexuser >().modify( 
          userit, 
          projectsipupdateexpires( 
            ( *userit )->expires + 
            boost::posix_time::seconds( DEFAULTSIPEXPIRES ) 
          ) );
  }
}


/*******************************************************************************
Function: processsippacket
Purpose: Process a REGISTER packet. To get into this function
the packet must have already been checked as a REGISTER packet and has all 
of the required headers.
Updated: 10.101.2019
*******************************************************************************/
void registrarsippacket( projectsippacketptr pk )
{
  /*
    Work out who this registration is for.
  */
  stringptr uri = pk->getrequesturi();
  if( !uri || uri->size() < 4 )
  {
    return;
  }
  sipuri suri( uri );
  std::string s;
  s.reserve( DEFAULTHEADERLINELENGTH );
  s = suri.host.str();
  s += '@';

  sipuri turi( pk->getheader( projectsippacket::To ) );
  s += turi.user.str();

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


