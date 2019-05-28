

#include <iostream>


#include "projectrtpchannel.h"

/*!md
# projectrtpchannel constructor
Create the socket then wait for data

echo "This is my data" > /dev/udp/127.0.0.1/10000
*/
projectrtpchannel::projectrtpchannel( boost::asio::io_service &io_service, short port )
  : port( port ),
  rtpsocket( io_service ),
  rtcpsocket( io_service ),
  rtpdata( NULL ),
  rtcpdata( NULL ),
  rtpindex( 0 )
{
}

/*!md
# projectrtpchannel destructor
Clean up
*/
projectrtpchannel::~projectrtpchannel( void )
{
  if( NULL != this->rtpdata )
  {
    delete this->rtpdata;
  }

  if( NULL != this->rtcpdata )
  {
    delete this->rtcpdata;
  }
}

/*!md
# create

*/
projectrtpchannel::pointer projectrtpchannel::create( boost::asio::io_service &io_service, short port )
{
  return pointer( new projectrtpchannel( io_service, port ) );
}

/*!md
# open
Open the channel to read network data. Setup memory and pointers.
*/
void projectrtpchannel::open( int codec )
{
  switch( codec )
  {
    case PCMA:
    case PCMU:
    {
      this->rtpdata = new unsigned char[ G711PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
    case G722:
    {
      this->rtpdata = new unsigned char[ G722PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
    case ILBC20:
    {
      this->rtpdata = new unsigned char[ ILBC20PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
    case ILBC30:
    {
      this->rtpdata = new unsigned char[ ILBC30PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
  }

  this->rtcpdata = new unsigned char[ RTCPMAXLENGTH ];
  this->rtpindex = 0;

  this->rtpsocket.open( boost::asio::ip::udp::v4() );
  this->rtpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port ) );

  this->rtcpsocket.open( boost::asio::ip::udp::v4() );
  this->rtcpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port + 1 ) );

  this->readsomertp();
  this->readsomertcp();
}

short projectrtpchannel::getport( void )
{
  return this->port;
}

/*!md
# close
Closes the channel.
*/
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

  if( NULL != this->rtpdata )
  {
    delete this->rtpdata;
    this->rtpdata = NULL;
  }

  if( NULL != this->rtcpdata )
  {
    delete this->rtcpdata;
    this->rtcpdata = NULL;
  }
}

/*!md
# handlereadsomertp
Wait for RTP data. We have to re-order when required.
*/
void projectrtpchannel::readsomertp( void )
{
  this->rtpsocket.async_receive_from(
    boost::asio::buffer( &this->rtpdata[ this->rtpindex ], RTPMAXLENGTH ), this->rtpsenderendpoint,
      [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
      {
        if ( !ec && bytes_recvd > 0 && bytes_recvd <= RTPMAXLENGTH )
        {
          this->handlertpdata();
          this->rtpindex = ( this->rtpindex + 1 ) % BUFFERPACKETCOUNT;
        }

        if( !ec )
        {
           this->readsomertp();
        }
      } );
}

/*!md
# handlereadsomertcp
Wait for RTP data
*/
void projectrtpchannel::readsomertcp( void )
{
  this->rtcpsocket.async_receive_from(
  boost::asio::buffer( this->rtcpdata, RTCPMAXLENGTH ), this->rtcpsenderendpoint,
    [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
    {
      if ( !ec && bytes_recvd > 0 && bytes_recvd <= RTCPMAXLENGTH )
      {
        this->handlertcpdata();
      }

      if( !ec )
      {
        this->readsomertcp();
      }
    } );
}


/*!md
# handlertpdata
We have received some RTP data - now do something with it.
*/
void projectrtpchannel::handlertpdata( void )
{
  /* first check we have received the correct one and move if necessary */
  uint32_t ts = this->gettimestamp( &this->rtpdata[ this->rtpindex ] );
  uint8_t cccount = this->getpacketcsrccount( &this->rtpdata[ this->rtpindex ] );
  std::cout << "rtp:" << static_cast<unsigned>( cccount ) << ":" << ts << std::endl;
}

/*!md
# handlertcpdata
We have received some RTCP data - now do something with it.
*/
void projectrtpchannel::handlertcpdata( void )
{

}

/*!md
# bridgeto
Another channel we write data to.
*/
void projectrtpchannel::bridgeto( pointer other )
{

}

/*!md
# getpacketversion
As it says.
*/
uint8_t projectrtpchannel::getpacketversion( uint8_t *pk )
{
  return ( pk[ 0 ] & 0xb0 ) >> 6;
}

/*!md
# getpacketpadding
As it says.
*/
uint8_t projectrtpchannel::getpacketpadding( uint8_t *pk )
{
  return ( pk[ 0 ] & 0x20 ) >> 5;
}

/*!md
# getpacketextension
As it says.
*/
uint8_t projectrtpchannel::getpacketextension( uint8_t *pk )
{
  return ( pk[ 0 ] & 0x10 ) >> 4;
}

/*!md
# getpacketcsrccount
As it says.
*/
uint8_t projectrtpchannel::getpacketcsrccount( uint8_t *pk )
{
  return ( pk[ 0 ] & 0x0f );
}

/*!md
# getpacketmarker
As it says.
*/
uint8_t projectrtpchannel::getpacketmarker( uint8_t *pk )
{
  return ( pk[ 1 ] & 0x80 ) >> 7;
}

/*!md
# getpayloadtype
As it says.
*/
uint8_t projectrtpchannel::getpayloadtype( uint8_t *pk )
{
  return ( pk[ 1 ] & 0x7f );
}

/*!md
# getsequencenumber
As it says.
*/
uint16_t projectrtpchannel::getsequencenumber( uint8_t *pk )
{
  uint16_t *tmp = ( uint16_t * )pk;
  tmp++;
  return ntohs( *tmp );
}

/*!md
# gettimestamp
As it says.
*/
uint32_t projectrtpchannel::gettimestamp( uint8_t *pk )
{
  uint32_t *tmp = ( uint32_t * )pk;
  tmp++;
  return ntohl( *tmp );
}

/*!md
# getssrc
As it says.
*/
uint32_t projectrtpchannel::getssrc( uint8_t *pk )
{
  uint32_t *tmp = ( uint32_t * )pk;
  tmp += 2;
  return ntohl( *tmp );
}

/*!md
# getcsrc
As it says. Use getpacketcsrccount to return the number of available
0-15. This function doesn't check bounds.

*/
uint32_t projectrtpchannel::getcsrc( uint8_t *pk, uint8_t index )
{
  uint32_t *tmp = ( uint32_t * )pk;
  tmp += 3 + index;
  return ntohl( *tmp );
}
