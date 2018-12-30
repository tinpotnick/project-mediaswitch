


#include "projectsipregistrar.h"
#include "test.h"

static projectsipregistrations regs;

/*******************************************************************************
Function: projectsipregistration
Purpose: Constructor
Updated: 18.12.2018
*******************************************************************************/
projectsipregistration::projectsipregistration( std::string u, int expires /*seconds */ ) :
  user( u ),
  expires( boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( expires ) ),
  nextping( boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( OPTIONSPINGFREQUENCY ) ),
  outstandingping( 0 ),
  lastreg( &projectsipregistration::regstart ),
  nextreg( &projectsipregistration::regstart )
{
}


void projectsipregistration::regstart( void )
{
  this->nextreg = &projectsipregistration::regwaitauth;
}

void projectsipregistration::regwaitauth( void )
{

}


void processsippacket( projectsippacket &pk )
{
    switch( pk.getmethod() )
  {
    case processsippacket::REGISTER:
    {
      break;
    }
    default:
    {
      return; // ?
      break;
    }
  }

  if( !pk.hasheader( projectsippacket::Via ) )
  {
    /* Bad = ignore */
    return;
  }
}


#ifdef TESTCODE
/*******************************************************************************
Function: testregs
Purpose: Test our registrar code.
Updated: 18.12.2018
*******************************************************************************/
void testregs( void )
{

  projectsipregistrationptr r1( new projectsipregistration( "1000@bling.babblevoice.com", 3600 ) );
  projectsipregistrationptr r2( new projectsipregistration( "1001@bling.babblevoice.com", 3600 ) );

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
  if( expiresit != regs.get< regindexexpires >().end() )
  {
    projectsipregistrationptr ptr = *expiresit;
    ptr->expires = boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( 10 );

    regs.get< 1 >().replace(  expiresit, ptr  );
  }


  std::cout << "All SIP registration tests passed" << std::endl;
}



#endif /* TESTCODE */


