

#include <iostream>

#include <boost/bind.hpp>


#include "projectrtpchannel.h"

/*!md
# Project RTP Channel

This file (class) represents an RP channel. That is an RTP stream (UDP) with its pair RTCP socket. Basic functions for

1. Opening and closing channels
2. bridging 2 channels
3. Sending data to an endpoint based on us receiving data first or (to be implimented) the address and port given to us when opening in the channel.


## projectrtpchannel constructor
Create the socket then wait for data

echo "This is my data" > /dev/udp/127.0.0.1/10000
*/
projectrtpchannel::projectrtpchannel( boost::asio::io_service &io_service, unsigned short port )
  : port( port ),
  resolver( io_service ),
  rtpsocket( io_service ),
  rtcpsocket( io_service ),
  receivedrtp( false ),
  targetconfirmed( false ),
  reader( true ),
  writer( true ),
  rtpindexoldest( 0 ),
  rtpindexin( 0 ),
  active( false ),
  timestampdiff( 0 )
{
}

/*!md
## projectrtpchannel destructor
Clean up
*/
projectrtpchannel::~projectrtpchannel( void )
{
}

/*!md
# create

*/
projectrtpchannel::pointer projectrtpchannel::create( boost::asio::io_service &io_service, unsigned short port )
{
  return pointer( new projectrtpchannel( io_service, port ) );
}

/*!md
## open
Open the channel to read network data. Setup memory and pointers.
*/
void projectrtpchannel::open()
{
  this->rtpindexin = 0;
  this->rtpindexoldest = 0;

  this->rtpsocket.open( boost::asio::ip::udp::v4() );
  this->rtpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port ) );

  this->rtcpsocket.open( boost::asio::ip::udp::v4() );
  this->rtcpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port + 1 ) );

  this->receivedrtp = false;
  this->active = true;
  this->timestampdiff = 0;

  this->codecs.clear();
  this->selectedcodec = 0;

  this->readsomertp();
  this->readsomertcp();
}

unsigned short projectrtpchannel::getport( void )
{
  return this->port;
}

/*!md
## close
Closes the channel.
*/
void projectrtpchannel::close( void )
{
  try
  {
    this->active = false;
    this->rtpsocket.close();
    this->rtcpsocket.close();
  }
  catch(...)
  {

  }
}

/*!md
## handlereadsomertp
Wait for RTP data. We have to re-order when required.
*/
void projectrtpchannel::readsomertp( void )
{
  this->rtpsocket.async_receive_from(
    boost::asio::buffer( &this->rtpdata[ this->rtpindexin ].pk, RTPMAXLENGTH ),
                          this->rtpsenderendpoint,
      [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
      {
        if ( !ec && bytes_recvd > 0 && bytes_recvd <= RTPMAXLENGTH )
        {
          if( !this->receivedrtp )
          {
            this->confirmedrtpsenderendpoint = this->rtpsenderendpoint;
            this->receivedrtp = true;
          }

          this->rtpdata[ this->rtpindexin ].length = bytes_recvd;
          this->rtpindexin = ( this->rtpindexin + 1 ) % BUFFERPACKETCOUNT;

          /* if we catch up */
          if( this->rtpindexin == this->rtpindexoldest )
          {
            this->rtpindexoldest = ( this->rtpindexoldest + 1 ) % BUFFERPACKETCOUNT;
          }
        }

        if( !ec && bytes_recvd && this->active )
        {
           this->readsomertp();
        }
      } );
}

/*!md
## handlereadsomertcp
Wait for RTP data
*/
void projectrtpchannel::readsomertcp( void )
{
  this->rtcpsocket.async_receive_from(
  boost::asio::buffer( &this->rtcpdata[ 0 ], RTCPMAXLENGTH ), this->rtcpsenderendpoint,
    [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
    {
      if ( !ec && bytes_recvd > 0 && bytes_recvd <= RTCPMAXLENGTH )
      {
        this->handlertcpdata();
      }

      if( !ec && bytes_recvd && this->active )
      {
        this->readsomertcp();
      }
    } );
}

/*!md
## isactive
As it says.
*/
bool projectrtpchannel::isactive( void )
{
  return this->active;
}

/*!md
## pop
If data has been received pop it and use it. Remains unordered.
*/
rtppacket * projectrtpchannel::pop( void )
{
  if( this->rtpindexoldest != this->rtpindexin )
  {
    rtppacket *buf;
    buf = &this->rtpdata[ this->rtpindexoldest ];

    this->rtpindexoldest = ( this->rtpindexoldest + 1 ) % BUFFERPACKETCOUNT;
    return buf;
  }

  return NULL;
}

/*!md
## popordered
It will search for the relevent ts if available. It will return NULL if not found. This is our ordering function. It will first look at the base of the buffer.
*/
rtppacket * projectrtpchannel::popordered( uint32_t ts )
{
  if( 0 == this->timestampdiff )
  {
    this->timestampdiff = ts - this->rtpdata[ this->rtpindexoldest ].gettimestamp();
  }

  /* we try and get the one from the expected position */
  for( int i = 0; i < ( BUFFERPACKETCOUNT / 2 ); i ++ )
  {
    /* fan out in both directions. the closer the more likely */
    uint32_t expectedts = ts + this->timestampdiff;
    int index = ( this->rtpindexoldest + i ) % BUFFERPACKETCOUNT;
    if( this->rtpdata[ index ].gettimestamp() == expectedts )
    {
      return &this->rtpdata[ index ];
    }

    index = ( this->rtpindexoldest - i ) % BUFFERPACKETCOUNT;
    if( this->rtpdata[ index ].gettimestamp() == expectedts )
    {
      return &this->rtpdata[ index ];
    }
  }

  return NULL;
}



/*!md
## writepacket
Send a [RTP] packet to our endpoint.
*/
void projectrtpchannel::writepacket( rtppacket *pk )
{
  if( this->receivedrtp || this->targetconfirmed )
  {
    this->rtpsocket.async_send_to(
                      boost::asio::buffer( pk->pk, pk->length ),
                      this->confirmedrtpsenderendpoint,
                      boost::bind( &projectrtpchannel::handlesend,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred ) );
  }
}

/*!md
## target
Our control can set the target of the RTP stream. This can be important in order to open holes in firewall for our reverse traffic.
*/
void projectrtpchannel::target( std::string &address, unsigned short port )
{
std::cout << "address: " << address << ", port: " << port << std::endl;
  boost::asio::ip::udp::resolver::query query( boost::asio::ip::udp::v4(), address, std::to_string( port ) );

  /* Resolve the address */
  this->resolver.async_resolve( query,
      boost::bind( &projectrtpchannel::handletargetresolve,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::iterator ) );
}

/*!md
## audio
The CODECs on the other end which are acceptable. The first one should be the prefered. For now we keep hold of the list of codecs as we may be using them in the future.
*/
void projectrtpchannel::audio( codeclist codecs )
{
  if( codecs.size() > 0 )
  {
    this->codecs = codecs;
    this->selectedcodec = codecs[ 0 ];
std::cout << "selected codec " << this->selectedcodec << std::endl;
  }
}

/*!md
## handletargetresolve
We have resolved the target address and port now use it. Further work could be to inform control there is an issue.
*/
void projectrtpchannel::handletargetresolve (
            boost::system::error_code e,
            boost::asio::ip::udp::resolver::iterator it )
{
  /* Don't override the symetric port we send back to */
  if( this->receivedrtp )
  {
    return;
  }

  boost::asio::ip::udp::resolver::iterator end;

  if( it == end )
  {
    /* Failure - silent (the call will be as well!) */
    return;
  }

std::cout << "Confirmed target" << std::endl;
  this->confirmedrtpsenderendpoint = *it;
  this->targetconfirmed = true;
}

/*!md
## handlesend
What is called once we have sent something.
*/
void projectrtpchannel::handlesend(
      const boost::system::error_code& error,
      std::size_t bytes_transferred)
{

}

/*!md
## handlertcpdata
We have received some RTCP data - now do something with it.
*/
void projectrtpchannel::handlertcpdata( void )
{

}

/*!md
## getpacketversion
As it says.
*/
uint8_t rtppacket::getpacketversion( void )
{
  return ( pk[ 0 ] & 0xb0 ) >> 6;
}

/*!md
## getpacketpadding
As it says.
*/
uint8_t rtppacket::getpacketpadding( void )
{
  return ( pk[ 0 ] & 0x20 ) >> 5;
}

/*!md
## getpacketextension
As it says.
*/
uint8_t rtppacket::getpacketextension( void )
{
  return ( pk[ 0 ] & 0x10 ) >> 4;
}

/*!md
## getpacketcsrccount
As it says.
*/
uint8_t rtppacket::getpacketcsrccount( void )
{
  return ( pk[ 0 ] & 0x0f );
}

/*!md
## getpacketmarker
As it says.
*/
uint8_t rtppacket::getpacketmarker( void )
{
  return ( pk[ 1 ] & 0x80 ) >> 7;
}

/*!md
## getpayloadtype
As it says.
*/
uint8_t rtppacket::getpayloadtype( void )
{
  return ( pk[ 1 ] & 0x7f );
}

/*!md
## getsequencenumber
As it says.
*/
uint16_t rtppacket::getsequencenumber( void )
{
  uint16_t *tmp = ( uint16_t * )pk;
  tmp++;
  return ntohs( *tmp );
}

/*!md
## gettimestamp
As it says.
*/
uint32_t rtppacket::gettimestamp( void )
{
  uint32_t *tmp = ( uint32_t * )pk;
  tmp++;
  return ntohl( *tmp );
}

/*!md
## getssrc
As it says.
*/
uint32_t rtppacket::getssrc( void )
{
  uint32_t *tmp = ( uint32_t * )pk;
  tmp += 2;
  return ntohl( *tmp );
}

/*!md
## getcsrc
As it says. Use getpacketcsrccount to return the number of available
0-15. This function doesn't check bounds.

*/
uint32_t rtppacket::getcsrc( uint8_t index )
{
  uint32_t *tmp = ( uint32_t * )pk;
  tmp += 3 + index;
  return ntohl( *tmp );
}
