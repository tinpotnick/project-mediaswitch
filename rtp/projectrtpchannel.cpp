

#include <iostream>


#include "projectrtpchannel.h"

/*******************************************************************************
Function: projectrtpchannel constructor
Purpose: Create the socket then wait for data
Updated: 01.03.2019
echo "This is my data" > /dev/udp/127.0.0.1/10000
*******************************************************************************/
projectrtpchannel::projectrtpchannel( boost::asio::io_service &io_service, short port )
  : port( port ),
  rtpsocket( io_service ),
  rtcpsocket( io_service )
{
}

/*******************************************************************************
Function: create
Purpose: 
Updated: 01.03.2019
*******************************************************************************/
projectrtpchannel::pointer projectrtpchannel::create( boost::asio::io_service &io_service, short port )
{
  return pointer( new projectrtpchannel( io_service, port ) );
}

/*******************************************************************************
Function: open
Purpose: Open the channel to read netwok data
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::open( void )
{
  this->rtpsocket.open( boost::asio::ip::udp::v4() );
  this->rtpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port ) );

  this->rtcpsocket.open( boost::asio::ip::udp::v4() );
  this->rtcpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port + 1 ) );

  this->readsomertp();
  this->readsomertcp();
}

/*******************************************************************************
Function: close
Purpose: Closes the channel.
Updated: 05.03.2019
*******************************************************************************/
void projectrtpchannel::close( void )
{
  try
  {
    this->rtpsocket.close();
    this->rtcpsocket.close();
  }
  catch(...)
  {

  }
}

/*******************************************************************************
Function: handlereadsomertp
Purpose: Wait for RTP data
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::readsomertp( void )
{
  this->rtpsocket.async_receive_from(
    boost::asio::buffer( this->data, max_length ), this->rtpsenderendpoint,
      [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
      {
        if ( !ec && bytes_recvd > 0 && bytes_recvd <= 1500 )
        {
          this->handlertpdata();
        }

        if( !ec )
        {
           this->readsomertp();
        }
      } );
}

/*******************************************************************************
Function: handlereadsomertcp
Purpose: Wait for RTP data
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::readsomertcp( void )
{
  this->rtcpsocket.async_receive_from(
  boost::asio::buffer( this->rtcpdata, max_length ), this->rtcpsenderendpoint,
    [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
    {
      if ( !ec && bytes_recvd > 0 && bytes_recvd <= 1500 )
      {
        this->handlertcpdata();
      }

      if( !ec )
      {
        this->readsomertcp();
      }
    } );
}


/*******************************************************************************
Function: handlertpdata
Purpose: We have received some RTP data - now do something with it.
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::handlertpdata( void )
{
  
}

/*******************************************************************************
Function: handlertcpdata
Purpose: We have received some RTCP data - now do something with it.
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::handlertcpdata( void )
{
  
}