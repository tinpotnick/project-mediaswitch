

#include <iostream>
#include <string>

#include <boost/bind.hpp>

#include "projectsipserver.h"
#include "projectsipsm.h"
#include "projectsipdirectory.h"

/*!md
## projectsipserver constructor
Create the socket then wait for data

echo "This is my data" > /dev/udp/127.0.0.1/5060
*/
projectsipserver::projectsipserver( boost::asio::io_service &io_service, short port )
  : socket( io_service, boost::asio::ip::udp::endpoint( boost::asio::ip::udp::v4(), port ) )
{
  this->handlereadsome();
}

/*!md
## create
Create a new server.
*/
projectsipserver::pointer projectsipserver::create(  boost::asio::io_service &io_service, short port  )
{
  return pointer( new projectsipserver( io_service, port ) );
}

/*!md
## handlereadsome
Wait for data

*/
void projectsipserver::handlereadsome( void )
{
  socket.async_receive_from(
    boost::asio::buffer( this->data, max_length ), sender_endpoint,
      [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
      {
        if ( !ec && bytes_recvd > 0 && bytes_recvd <= 1500 )
        {
          this->bytesreceived = bytes_recvd;
          this->handledata();
        }

        if( !ec )
        {
          this->handlereadsome();
        }
      } );
}


/*!md
## handledata
Do something with the data.

*/
void projectsipserver::handledata( void )
{
  //std::cout << "Peer IP: " << this->sender_endpoint.address().to_string() << std::endl;
  //std::cout << "Peer Port: " << this->sender_endpoint.port() << std::endl;

  try
  {
    stringptr pk( new std::string( this->data, this->bytesreceived ) );
    projectsippacket::pointer packet( new projectsipserverpacket( this, this->sender_endpoint, pk ) );
    projectsipsm::handlesippacket( packet );
  }
  catch(...)
  {
    std::cerr << "Unhandled exception in SIP server." << std::endl;
  }

}

/*!md
## respond
Called by our state machine in response to timers/data etc.

*/
void projectsipserverpacket::respond( stringptr doc )
{
  sipserver->getsocket()->async_send_to( boost::asio::buffer( *doc ), this->sender_endpoint,
          boost::bind( &projectsipserverpacket::handlesend,
            shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred) );
}

/*!md
## handlesend
Just placeholder for now - we should perhaps track errors.

*/
void projectsipserverpacket::handlesend(
      const boost::system::error_code& error,
      std::size_t bytes_transferred)
{

}
