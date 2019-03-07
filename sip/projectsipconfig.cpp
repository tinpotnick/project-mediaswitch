

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

  this->sipport = 5060;
}

/*******************************************************************************
Function: gethostname
Purpose: Getour hostname.
Updated: 10.01.2019
*******************************************************************************/
const char* projectsipconfig::gethostname( void )
{
  return cnf.hostname.c_str();
}

/*******************************************************************************
Function: gethostip
Purpose: Getour host ip - used in SIP comms.
Updated: 10.01.2019
*******************************************************************************/
const char* projectsipconfig::gethostip( void )
{
  return "10.0.0.3";
}

/*******************************************************************************
Function: getsipport
Purpose: Getour sip port - used in SIP comms.
Updated: 10.01.2019
*******************************************************************************/
const short projectsipconfig::getsipport( void )
{
  return cnf.sipport;
}

/*******************************************************************************
Function: setsipport
Purpose: Set our sip port - used in SIP comms.
Updated: 05.03.2019
*******************************************************************************/
const void projectsipconfig::setsipport( short port )
{
  cnf.sipport = port;
}