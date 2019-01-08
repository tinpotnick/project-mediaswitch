

#include <iostream>
#include <string>

#include "projectsipserver.h"
#include "projectsipsm.h"

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
    }
    this->handlereadsome();
  } );
}


/*******************************************************************************
Function: handledata
Purpose: Do something with the data.
Updated: 12.12.2018
*******************************************************************************/
void projectsipserver::handledata( void )
{
  //std::cout << "Peer IP: " << this->sender_endpoint.address().to_string() << std::endl;
  //std::cout << "Peer Port: " << this->sender_endpoint.port() << std::endl;

  stringptr pk( new std::string( this->data, this->bytesreceived ) );
  projectsippacketptr packet( new projectsipserverpacket( this, this->sender_endpoint, pk ) );
  projectsipsm::handlesippacket( packet );
}

/*******************************************************************************
Function: respond
Purpose: Called by our state machine in response to timers/data etc.
Updated: 03.01.2019
*******************************************************************************/
void projectsipserverpacket::respond( stringptr doc )
{
  sipserver->getsocket()->async_send_to( boost::asio::buffer( *doc ), this->sender_endpoint,
          boost::bind( &projectsipserverpacket::handlesend, this, doc,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred) );
}

/*******************************************************************************
Function: handlesend
Purpose: Just placeholder for now - we should perhaps track errors.
Updated: 03.01.2019
*******************************************************************************/
void projectsipserverpacket::handlesend( boost::shared_ptr<std::string> message,
      const boost::system::error_code& error,
      std::size_t bytes_transferred)
{

}



