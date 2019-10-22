
#ifndef PROJECTSIPSERVER_H
#define PROJECTSIPSERVER_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>

#include "projectsippacket.h"

#define MAXUDPPACKET 2000


/*!md
# projectsipserver
Our simple UDP SIP server
*/
class projectsipserver :
  public boost::enable_shared_from_this< projectsipserver >
{
public:
  projectsipserver( boost::asio::io_service &io_service, short port );
  boost::asio::ip::udp::socket *getsocket( void ){ return &this->socket; }

  typedef boost::shared_ptr< projectsipserver > pointer;
  static projectsipserver::pointer create(  boost::asio::io_service &io_service, short port );

private:
  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::endpoint sender_endpoint;
  enum { max_length = MAXUDPPACKET };
  char data[ max_length ];
  int bytesreceived;

  void handlereadsome( void );
  void handledata( void );
};


/*!md
# projectsipserverpacket
Used to parse SIP packets - but allows us to send the response to the correct place.
*/
class projectsipserverpacket :
  public projectsippacket,
  public boost::enable_shared_from_this< projectsipserverpacket >
{
public:
  projectsipserverpacket( projectsipserver *sipserver, boost::asio::ip::udp::endpoint sender_endpoint, stringptr pk ) :
    projectsippacket( pk )
  {
  	this->sender_endpoint = sender_endpoint;
  	this->sipserver = sipserver;
  }

  virtual void respond( stringptr doc );

  virtual std::string getremotehost( void ) { return this->sender_endpoint.address().to_string(); }
  virtual unsigned short getremoteport( void ) { return this->sender_endpoint.port(); }

  void handlesend( const boost::system::error_code&,
      std::size_t );

private:
  projectsipserver *sipserver;
  boost::asio::ip::udp::endpoint sender_endpoint;
};


#endif /* PROJECTSIPSERVER_H */
