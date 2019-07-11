

#include <iostream>

#include <boost/bind.hpp>
#include <iomanip>


#include "projectrtpchannel.h"


/*!md
Pre generate all 711 data.
We can speed up 711 conversion by way of pre calculating values. 128K(ish) of data is not too much to worry about!
*/

static uint8_t l16topcmu[ 65536 ];
static uint8_t l16topcma[ 65536 ];
static int16_t pcmatol16[ 256 ];
static int16_t pcmutol16[ 256 ];

void gen711convertdata( void )
{
  std::cout << "Pre generating G711 tables";
  for( int32_t i = 0; i != 65535; i++ )
  {
    int16_t l16val = i - 32768;
    l16topcmu[ i ] = linear_to_ulaw( l16val );
    l16topcma[ i ] = linear_to_alaw( l16val );
  }

  for( uint8_t i = 0; i != 255; i++ )
  {
    pcmatol16[ i ] = alaw_to_linear( i );
    pcmutol16[ i ] = ulaw_to_linear( i );
  }

  std::cout << " - completed." << std::endl;
}


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
  : rtpindexoldest( 0 ),
  rtpindexin( 0 ),
  rtpoutindex( 0 ),
  active( false ),
  port( port ),
  resolver( io_service ),
  rtpsocket( io_service ),
  rtcpsocket( io_service ),
  receivedrtp( false ),
  targetconfirmed( false ),
  reader( true ),
  writer( true ),
  receivedpkcount( 0 ),
  g722encoder( nullptr ),
  g722decoder( nullptr ),
  lpfilter(),
  resamplelastsample( 0 )
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
  this->receivedpkcount = 0;

  this->rtpoutindex = 0;

  this->rtpsocket.open( boost::asio::ip::udp::v4() );
  this->rtpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port ) );

  this->rtcpsocket.open( boost::asio::ip::udp::v4() );
  this->rtcpsocket.bind( boost::asio::ip::udp::endpoint(
      boost::asio::ip::udp::v4(), this->port + 1 ) );

  this->receivedrtp = false;
  this->active = true;

  this->codecs.clear();
  this->selectedcodec = 0;

  this->readsomertp();
  this->readsomertcp();

  this->lpfilter.reset();
  this->resamplelastsample = 0;
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
    if( nullptr != this->g722decoder )
    {
      g722_decode_free( this->g722decoder );
      this->g722decoder = nullptr;
    }
    if( nullptr != this->g722encoder )
    {
      g722_encode_free( this->g722encoder );
      this->g722encoder = nullptr;
    }

    /* remove oursevelse from our list of mixers */
    if( this->others )
    {
      projectrtpchannellist::iterator it;
      for( it = this->others->begin(); it != this->others->end(); it++ )
      {
        if( it->get() == this )
        {
          this->others->erase( it );
          break;
        }
      }
      /* release the shred pointer */
      this->others = nullptr;
    }

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
Wait for RTP data. We have to re-order when required. Look after all of the round robin memory here. We should have enough time to deal with the in data before it gets overwritten.
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
          this->receivedpkcount++;
          if( !this->receivedrtp )
          {
            this->confirmedrtpsenderendpoint = this->rtpsenderendpoint;
            this->receivedrtp = true;
          }

          this->rtpdata[ this->rtpindexin ].length = bytes_recvd;
          this->rtpdata[ this->rtpindexin ].havel168k = false;
          this->rtpdata[ this->rtpindexin ].havel1616k = false;

          this->rtpindexin = ( this->rtpindexin + 1 ) % BUFFERPACKETCOUNT;

          /* if we catch up */
          if( this->rtpindexin == this->rtpindexoldest )
          {
            this->rtpindexoldest = ( this->rtpindexoldest + 1 ) % BUFFERPACKETCOUNT;
          }
          this->handlertpdata();
        }

        if( !ec && bytes_recvd && this->active )
        {
           this->readsomertp();
        }
      } );
}

/*!md
## isl16required
Check if we need it converting to L16

If we send it to somewhere, we need to decide if we have to convert to l16 or not
If we send to PCM (U or A) to other then we do
If we send from ilbc or 722 then we do
PCM(a or u) to PCM(a or u) does not
same to same does not

Further work: recording etc - we probably do need conversion.
Further work: this is where can re-order (probaby very simple - are we the current latest and work back to any left over)
*/
bool projectrtpchannel::isl16required( rtppacket *src )
{
  auto uspayloadtype = src->getpayloadtype();

  for( auto it = this->others->begin(); it != this->others->end(); it++ )
  {
    if( uspayloadtype != (* it )->selectedcodec )
    {
      /* Some conversion required */
      switch( uspayloadtype )
      {
        case PCMAPAYLOADTYPE:
        case PCMUPAYLOADTYPE:
        {
          switch( (* it )->selectedcodec )
          {
            case PCMAPAYLOADTYPE:
            case PCMUPAYLOADTYPE:
            {
              break;
            }
            default:
            {
              return true;
            }
          }
          break;
        }
        case G722PAYLOADTYPE:
        {
          return true;
        }
        case ILBCPAYLOADTYPE:
        {
          return true;
        }
      }
    }
  }

  return false;
}

#warning TODO - finish these checks and move them to only when something has changed and replace with simple bools.
bool projectrtpchannel::isl16widebandrequired( rtppacket *src )
{
  return true;
}

bool projectrtpchannel::isl16narrowbandrequired( rtppacket *src )
{
  return true;
}

/*!md
## handlertpdata
We are not supporting endless support for different CODECs which reduces flexability but simplifies this work. As this work is simplified it should also be more efficient.

*/
void projectrtpchannel::handlertpdata( void )
{
  rtppacket *src = &this->rtpdata[ this->rtpindexoldest ];
  this->rtpindexoldest = ( this->rtpindexoldest + 1 ) % BUFFERPACKETCOUNT;

  /* review: if nothing do nothing */
  if( !this->others || 0 == this->others->size() )
  { 
    return;
  }

  /* any decoding required */
  if( this->isl16required( src ) )
  {
    switch( src->getpayloadtype() )
    {
      case PCMAPAYLOADTYPE:
      case PCMUPAYLOADTYPE:
      {
        src->g711tol16();
        if( this->isl16widebandrequired( src ) )
        {
          src->l16lowtowideband( &this->resamplelastsample );
        }
        break;
      }
      case ILBCPAYLOADTYPE:
      {
        break;
      }
      case G722PAYLOADTYPE:
      {
        if( nullptr == this->g722decoder )
        {
          this->g722decoder = g722_decode_init( NULL, 64000, G722_PACKED );
        }
        src->g722tol16( this->g722decoder );

        if( this->isl16narrowbandrequired( src ) )
        {
          src->l16widetonarrowband( this->lpfilter );
        }
        break;
      }
    }
  }

  /* The next section is sending to our recipient(s) */
  if( 2 == this->others->size() )
  {
    /* one should be us */
    projectrtpchannellist::iterator it = this->others->begin();
    projectrtpchannel::pointer chan = *it;
    if( it->get() == this )
    {
      chan = *( ++it );
    }

    auto uspayloadtype = src->getpayloadtype();

    /* Conversion from PCMU to PCMA and vice versa can be done here as it is a simple lookup, anything else should be passed off to another thread. */
    if( uspayloadtype == chan->selectedcodec )
    {
      rtppacket *dst = chan->gettempoutbuf();
      dst->copy( src );
      chan->writepacket( dst );
    }
    else
    {
      /* we have to transcode */
      switch( chan->selectedcodec ) /* other channel - where we are sending data (encoding) */
      {
        case PCMAPAYLOADTYPE:
        case PCMUPAYLOADTYPE:
        {
          switch( uspayloadtype )
          {
            /* special case */
            case PCMAPAYLOADTYPE:
            case PCMUPAYLOADTYPE:
            {
              rtppacket *dst = chan->gettempoutbuf();
              dst->copyheader( src );
              dst->xlaw2ylaw( src );
              chan->writepacket( dst );
              return;
            }
            default:
            {
              rtppacket *dst = chan->gettempoutbuf();
              dst->copyheader( src );
              dst->l16tog711( chan->selectedcodec, src );
              chan->writepacket( dst );
              return;
            }
          }
        }
        case ILBCPAYLOADTYPE:
        {
          return;
        }
        case G722PAYLOADTYPE:
        {
          if( nullptr == this->g722encoder )
          {
            this->g722encoder = g722_encode_init( NULL, 64000, G722_PACKED );
          }

          rtppacket *dst = chan->gettempoutbuf();
          dst->copyheader( src );
          dst->l16tog722( this->g722encoder, src );
          chan->writepacket( dst );
        }
      }
    }
  }
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
## gettempoutbuf
When we need a buffer to send data out (because we cannot guarantee our own buffer will be available) we can use the circular out buffer on this channel. This will return the next one available.
*/
rtppacket *projectrtpchannel::gettempoutbuf( void )
{
  rtppacket *buf = &this->outrtpdata[ this->rtpoutindex ];
  this->rtpoutindex = ( this->rtpoutindex + 1 ) % BUFFERPACKETCOUNT;
  return buf;
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
  boost::asio::ip::udp::resolver::query query( boost::asio::ip::udp::v4(), address, std::to_string( port ) );

  /* Resolve the address */
  this->resolver.async_resolve( query,
      boost::bind( &projectrtpchannel::handletargetresolve,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::iterator ) );
}

/*!md
## mix
Add the other to our list of others. n way relationship.
*/
bool projectrtpchannel::mix( projectrtpchannel::pointer other )
{
  /* We only mix us with others who are currently friendless */
  if( other->others )
  {
    return false;
  }

  if( !this->others )
  {
    this->others = projectrtpchannellistptr( new projectrtpchannellist  );
  }

  /* ensure no duplicates */
  bool usfound = false;
  bool themfound = false;
  projectrtpchannellist::iterator it;
  for( it = this->others->begin(); it != this->others->end(); it++ )
  {
    if( it->get() == this )
    {
      usfound = true;
    }

    if( *it == other )
    {
      themfound = true;
    }
  }

  if( !usfound )
  {
    this->others->push_back( other );
  }

  if( !themfound )
  {
    this->others->push_back( shared_from_this() );
  }

  other->others = this->others;

  return true;
}

/*!md
## audio
The CODECs on the other end which are acceptable. The first one should be the preferred. For now we keep hold of the list of codecs as we may be using them in the future. Filter out non-RTP streams (such as DTMF).
*/
void projectrtpchannel::audio( codeclist codecs )
{
  if( codecs.size() > 0 )
  {
    this->codecs = codecs;
    codeclist::iterator it;
    for( it = codecs.begin(); it != codecs.end(); it++ )
    {
      switch( *it )
      {
        case PCMAPAYLOADTYPE:
        case PCMUPAYLOADTYPE:
        case G722PAYLOADTYPE:
        case ILBCPAYLOADTYPE:
        {
          this->selectedcodec = *it;
          return;
        }
      }
    }
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
  /* Don't override the symmetric port we send back to */
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
## Constructor
*/
rtppacket::rtppacket() :
  length( 0 ),
  havel168k( false ),
  havel1616k( false )
{

}

/*!md
## Copy contructor

We only need to copy the required bytes.
*/
rtppacket::rtppacket( rtppacket &p ) :
  havel168k( false ),
  havel1616k( false )
{
  this->length = p.length;
  memcpy( this->pk, p.pk, p.length );
}

/*!md
## Copy
*/
void rtppacket::copy( rtppacket *src )
{
  if( !src ) return;
  this->length = src->length;
  memcpy( this->pk, src->pk, src->length );
}
/*!md
## Copy header
*/
void rtppacket::copyheader( rtppacket *src )
{
  if( !src ) return;
  this->length = src->length;
  memcpy( this->pk, src->pk, 12 + src->getpacketcsrccount() );
}

/*!md
## xlaw2ylaw

From whichever PCM encoding (u or a) encode to the other without having to do intermediate l16.
*/
void rtppacket::xlaw2ylaw( rtppacket *in )
{
  if( !in ) return;
  unsigned char ( *convert )( unsigned char );

  if( PCMAPAYLOADTYPE == in->getpayloadtype() )
  {
    this->setpayloadtype( PCMUPAYLOADTYPE );
    convert = alaw_to_ulaw;
  }
  else
  {
    this->setpayloadtype( PCMAPAYLOADTYPE );
    convert = ulaw_to_alaw;
  }

  uint8_t *inbufptr, *outbufptr;
  inbufptr = in->getpayload();
  outbufptr = this->getpayload();

  for( int i = 0; i < G711PAYLOADBYTES; i++ )
  {
    *outbufptr = convert( *inbufptr );

    outbufptr++;
    inbufptr++;
  }
}


/*!md
## getpacketversion
As it says.
*/
uint8_t rtppacket::getpacketversion( void )
{
  return ( this->pk[ 0 ] & 0xc0 ) >> 6;
}

/*!md
## getpacketpadding
As it says.
*/
uint8_t rtppacket::getpacketpadding( void )
{
  return ( this->pk[ 0 ] & 0x20 ) >> 5;
}

/*!md
## getpacketextension
As it says.
*/
uint8_t rtppacket::getpacketextension( void )
{
  return ( this->pk[ 0 ] & 0x10 ) >> 4;
}

/*!md
## getpacketcsrccount
As it says.
*/
uint8_t rtppacket::getpacketcsrccount( void )
{
  return ( this->pk[ 0 ] & 0x0f );
}

/*!md
## getpacketmarker
As it says.
*/
uint8_t rtppacket::getpacketmarker( void )
{
  return ( this->pk[ 1 ] & 0x80 ) >> 7;
}

/*!md
## getpayloadtype
As it says.
*/
uint8_t rtppacket::getpayloadtype( void )
{
  return ( this->pk[ 1 ] & 0x7f );
}

/*!md
## setpayloadtype
As it says.
*/
void rtppacket::setpayloadtype( uint8_t pt )
{
  this->pk[ 1 ] = ( this->pk[ 1 ] & 0x80 ) | ( pt & 0x7f );
}

/*!md
## getsequencenumber
As it says.
*/
uint16_t rtppacket::getsequencenumber( void )
{
  uint16_t *tmp = ( uint16_t * )this->pk;
  tmp++;
  return ntohs( *tmp );
}

/*!md
## getsequencenumber
As it says.
*/
void rtppacket::setsequencenumber( uint16_t sq )
{
  uint16_t *tmp = ( uint16_t * )this->pk;
  *tmp = htons( sq );
}

/*!md
## gettimestamp
As it says.
*/
uint32_t rtppacket::gettimestamp( void )
{
  uint32_t *tmp = ( uint32_t * )this->pk;
  tmp++;
  return ntohl( *tmp );
}

/*!md
## settimestamp
As it says.
*/
void rtppacket::settimestamp( uint32_t tmstp )
{
  uint32_t *tmp = ( uint32_t * )this->pk;
  *tmp = htonl( tmstp );
}

/*!md
## getssrc
As it says.
*/
uint32_t rtppacket::getssrc( void )
{
  uint32_t *tmp = ( uint32_t * )this->pk;
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
  uint32_t *tmp = ( uint32_t * )this->pk;
  tmp += 3 + index;
  return ntohl( *tmp );
}

/*!md
## getpayload
Returns a pointer to the start of the payload.
*/
uint8_t *rtppacket::getpayload( void )
{
  uint8_t *ptr = this->pk;
  ptr += 12;
  ptr += this->getpacketcsrccount();
  return ptr;
}

/*!md
## g722tol16
As it says.
*/
void rtppacket::g722tol16( g722_decode_state_t *g722decoder )
{
  g722_decode( g722decoder, this->l1616k, this->getpayload(), G722PAYLOADBYTES );
  this->havel1616k = true;
}

/*!md
## l16tog722
As it says.
*/
void rtppacket::l16tog722( g722_encode_state_t *g722encoder, rtppacket *l16src )
{
  this->setpayloadtype( G722PAYLOADTYPE );
  this->length = 12 + this->getpacketcsrccount() + G722PAYLOADBYTES;
  g722_encode( g722encoder, this->getpayload(), l16src->l1616k, G722PAYLOADBYTES * 2 );
}

/*!md
## g711tol16
As it says.
*/
void rtppacket::g711tol16( void )
{
  int16_t *convert;
  uint8_t pt = this->getpayloadtype();

  if( PCMAPAYLOADTYPE == pt )
  {
    convert = pcmatol16;
  }
  else if( PCMUPAYLOADTYPE == pt )
  {
    convert = pcmutol16;
  }
  else
  {
    return;
  }

  uint8_t *in;
  int16_t *out;
  in = this->getpayload();
  out = this->l168k;

  for( int i = 0; i < G711PAYLOADBYTES; i++ )
  {
    *out = convert[ *in ];
    in++;
    out++;
  }

  this->havel168k = true;
}

/*!md
## l16tog711
payloadtype: PCMAPAYLOADTYPE or PCMUPAYLOADTYPE
*/
void rtppacket::l16tog711( uint8_t payloadtype, rtppacket *l16src )
{
  this->setpayloadtype( payloadtype );
  this->length = 12 + this->getpacketcsrccount() + G711PAYLOADBYTES;
  uint8_t *convert;
  switch( payloadtype )
  {
    case PCMAPAYLOADTYPE:
    {
      convert = l16topcma;
      break;
    }
    case PCMUPAYLOADTYPE:
    {
      convert = l16topcmu;
      break;
    }
    default:
    {
      return;
    }
  }

  uint8_t *out;
  int16_t *in;
  out = this->getpayload();
  in = l16src->l168k;

  for( int i = 0; i < G711PAYLOADBYTES; i++ )
  {
    *out = convert[ ( *in ) + 32768 ];

    in++;
    out++;
  }
}


/*!md
## l16lowtowideband
Upsample from narrow to wideband. Take each point and interpolate between them. We require the final sample from the last packet to continue the interpolating.
*/
void rtppacket::l16lowtowideband( int16_t *lastsample )
{
  int16_t *in = this->l168k;
  int16_t *out = this->l1616k;

  for( int i = 0; i < G711PAYLOADBYTES * 2; i++ )
  {
    *out = ( ( *in - *lastsample ) / 2 ) + *lastsample;
    *lastsample = *in;
    out++;

    *out = *in;

    out++;
    in++;
  }
  this->havel1616k = true;
}

/*!md
##  l16widetolowband
Downsample our L16 wideband samples to 8K. Pass through filter then grab every other sample.
*/
void rtppacket::l16widetonarrowband( lowpass3_4k16k &filter )
{
  int16_t *out = this->l168k;
  int16_t *in = this->l1616k;

  int16_t filteredval;
  for( int i = 0; i < G711PAYLOADBYTES * 2; i++ )
  {
    filteredval = filter.execute( *in );
    in++;

    if( 0x01 == ( i & 0x01 ) )
    {
      *out = filteredval;
      out++;
    }
  }
  this->havel168k = true;
}

