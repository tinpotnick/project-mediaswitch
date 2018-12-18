

#include <iostream>
#include <string>

#include "projectsipserver.h"

/*******************************************************************************
Function: projectsipserver constructor
Purpose: Create the socket then wait for data
Updated: 12.12.2018
echo "This is my data" > /dev/udp/127.0.0.1/5060
*******************************************************************************/
projectsipserver::projectsipserver( boost::asio::io_service &io_service, short port )
  : socket( io_service, boost::asio::ip::udp::endpoint( boost::asio::ip::udp::v4(), port ) )
{
  this->handlereadsome();
}

/*******************************************************************************
Function: handlereadsome
Purpose: Wait for data
Updated: 12.12.2018
*******************************************************************************/
void projectsipserver::handlereadsome( void )
{
  socket.async_receive_from(
  boost::asio::buffer( data, max_length ), sender_endpoint,
  [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
  {
    if ( !ec && bytes_recvd > 0 && bytes_recvd <= 1500 )
    {
      this->bytesreceived = bytes_recvd;
      this->handledata();
      this->handlereadsome();
    }
  } );
}


/*******************************************************************************
Function: handledata
Purpose: Do something with the data.
Updated: 12.12.2018
*******************************************************************************/
void projectsipserver::handledata( void )
{
  std::cout << "Peer IP: " << this->sender_endpoint.address().to_string() << std::endl;
  std::cout << "Peer Port: " << this->sender_endpoint.port() << std::endl;
  std::string sdata( this->data, this->bytesreceived );
  std::cout << sdata << std::endl;
}




