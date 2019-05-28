

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

/*!md
# projectsipconfig
Constructor
*/
projectsipconfig::projectsipconfig()
{
  char b[ 200 ];

  if( -1 == ::gethostname( b, sizeof( b ) ) )
  {
    return;
  }

  this->sipport = 5060;

  /* Work out a default IP. */
  char *IPbuffer;
  struct hostent *host_entry;

  // To retrieve host information
  host_entry = ::gethostbyname( b );

  // To convert an Internet network
  // address into ASCII string
  IPbuffer = inet_ntoa(*((struct in_addr*)
                          host_entry->h_addr_list[0]));
  this->hostip = IPbuffer;

  this->hostipsipport = this->hostip;
}

/*!md
# gethostip
Getour host ip - used in SIP comms especially in the contact field
*/
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

/*!md
# getsipport
Getour sip port - used in SIP comms.
*/
const short projectsipconfig::getsipport( void )
{
  return cnf.sipport;
}

/*!md
# setsipport
Set our sip port - used in SIP comms.
*/
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

/*!md
# setsiphost
Set our sip host - used in SIP comms.
*/
const void projectsipconfig::setsiphostip( std::string a )
{
  cnf.hostip = a;

  cnf.hostipsipport = cnf.hostip;
  if( 5060 != cnf.sipport )
  {
    cnf.hostipsipport += ":";
    cnf.hostipsipport += std::to_string( cnf.sipport );
  }
}
