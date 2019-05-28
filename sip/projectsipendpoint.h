
#ifndef PROJECTSIPENDPOINT_H
#define PROJECTSIPENDPOINT_H


#include <boost/shared_ptr.hpp>


#include "projectsipstring.h"
#include "projectsippacket.h"



class projectsipendpoint
{
public:
  projectsipendpoint();

  bool sendpk( projectsippacket::pointer pk );

  /* Set this to the last received packet from this connection */
  projectsippacket::pointer lastreceivedpk;

  /* Set this to the last authenticated packet from this connection */
  projectsippacket::pointer lastauthenticatedpk;


protected:
  int errorcode;
  std::string errorreason;
};


#endif  /* PROJECTSIPENDPOINT_H */
