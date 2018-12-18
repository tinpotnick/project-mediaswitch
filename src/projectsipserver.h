
#ifndef PROJECTSIPSERVER_H
#define PROJECTSIPSERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>


/*******************************************************************************
Class: projectsipserver
Purpose: Our simple UDP SIP server
Updated: 12.12.2018
*******************************************************************************/
class projectsipserver
{

public:
  projectsipserver( boost::asio::io_service &io_service, short port );

private:
  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::endpoint sender_endpoint;
  enum { max_length = 1500 };
  char data[ max_length ];
  int bytesreceived;

  void handlereadsome( void );
  void handledata( void );
};


#endif /* PROJECTSIPSERVER_H */
