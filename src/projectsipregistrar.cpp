


#include "projectsipregistrar.h"

static projectsipregistrations regs;


/*******************************************************************************
Function: projectsipregistration
Purpose: Constructor
Updated: 18.12.2018
*******************************************************************************/
projectsipregistration::projectsipregistration( std::string u, int expires /*seconds */ )
{
  this->user = u;
  this->outstandingping = 0;
  this->expires = boost::posix_time::second_clock::local_time() +  boost::posix_time::seconds( expires );
  this->nextping = boost::posix_time::second_clock::local_time() + boost::posix_time::seconds( OPTIONSPINGFREQUENCY );
}


#ifdef TESTCODE
/*******************************************************************************
Function: testregs
Purpose: Test our registrar code.
Updated: 18.12.2018
*******************************************************************************/
void testregs( void )
{
  projectsipregistration r1( "1000@bling.babblevoice.com", 3600 );
  projectsipregistration r2( "1001@bling.babblevoice.com", 3600 );

  regs.insert( r1 );
  regs.insert( r2 );
}



#endif /* TESTCODE */


