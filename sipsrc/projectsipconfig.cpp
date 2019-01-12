

/* gethostname */
#include <unistd.h>

#define PROJECTSIPCONFIGEXTERN

#include "projectsipconfig.h"

projectsipconfig cnf;

/*******************************************************************************
Function: projectsipconfig
Purpose: Constructor
Updated: 10.01.2019
*******************************************************************************/
projectsipconfig::projectsipconfig()
{
  char b[ 200 ];
  if( -1 == ::gethostname( b, sizeof( b ) ) )
  {
    this->hostname = "unknown";
    return;
  }
  this->hostname = b;
}

/*******************************************************************************
Function: gethostname
Purpose: Getour hostname.
Updated: 10.01.2019
*******************************************************************************/
const char* projectsipconfig::gethostname( void )
{
  return this->hostname.c_str();;
}