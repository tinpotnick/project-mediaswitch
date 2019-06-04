
#ifndef PROJECTSIPENDPOINT_H
#define PROJECTSIPENDPOINT_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>


#include "projectsipstring.h"
#include "projectsippacket.h"
#include "projectsipserver.h"

#include "projectsipdnssrvresolver.h"
#include "globals.h"



class projectsipendpoint :
  public boost::enable_shared_from_this< projectsipendpoint >
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

  /* Resolve host to ip */
  boost::asio::ip::udp::resolver resolver;
  void handleresolve(
            boost::system::error_code e,
            boost::asio::ip::udp::resolver::iterator it );

  boost::asio::ip::udp::endpoint receiverendpoint;
  dnssrvrecord::pointer srvrecord;

  /* We have been asked to send a packet */
  projectsippacket::pointer tosendpk;

  void handlesipendpointsend( const boost::system::error_code& error,
                      std::size_t bytes_transferred);
};


#endif  /* PROJECTSIPENDPOINT_H */
