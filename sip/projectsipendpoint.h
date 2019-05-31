
#ifndef PROJECTSIPENDPOINT_H
#define PROJECTSIPENDPOINT_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>


#include "projectsipstring.h"
#include "projectsippacket.h"

#include "projectsipdnssrvresolver.h"



class projectsipendpoint : public boost::enable_shared_from_this< projectsipendpoint >
{
public:
  projectsipendpoint();
  virtual ~projectsipendpoint();

  bool sendpk( projectsippacket::pointer pk );

  /* Set this to the last received packet from this connection */
  projectsippacket::pointer lastreceivedpk;

  /* Set this to the last authenticated packet from this connection */
  projectsippacket::pointer lastauthenticatedpk;

protected:
  int errorcode;
  std::string errorreason;

private:
  projectsipdnssrvresolver::pointer srvresolver;

  void handlesrvresolve( dnssrvrealm::pointer answer );
};


#endif  /* PROJECTSIPENDPOINT_H */
