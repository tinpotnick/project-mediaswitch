

/* gethostname */
#include <unistd.h>

#define PROJECTSIPCONFIGEXTERN

#include "projectsipconfig.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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

  /* Work out a default IP. */
  char *IPbuffer;
  struct hostent *host_entry;

  // To retrieve host information
  host_entry = ::gethostbyname( this->hostname.c_str() );

  // To convert an Internet network
  // address into ASCII string
  IPbuffer = inet_ntoa(*((struct in_addr*)
                          host_entry->h_addr_list[0]));
  this->hostip = IPbuffer;

  this->hostipsipport = this->hostip;
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
Purpose: Getour host ip - used in SIP comms especially in the contact field
Updated: 10.01.2019
*******************************************************************************/
const char* projectsipconfig::gethostip( void )
{
  return cnf.hostip.c_str();
}

/*!md
# gethostipsipport
*/
const char* projectsipconfig::gethostipsipport( void )
{
  return cnf.hostipsipport.c_str();
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

  cnf.hostipsipport = cnf.hostip;
  if( 5060 != port )
  {
    cnf.hostipsipport += ":";
    cnf.hostipsipport += std::to_string( cnf.sipport );
  }
}
