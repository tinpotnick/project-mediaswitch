

#include <iostream>

#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <iomanip>


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
  mixqueue( MIXQUEUESIZE )
{
}

/*!md
## projectrtpchannel destructor
Clean up
*/
projectrtpchannel::~projectrtpchannel( void )
{
  this->player = nullptr;
  this->others = nullptr;
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
void projectrtpchannel::open( std::string &control )
{
  /* indexes into our circular rtp array */
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

  this->ssrcout = rand();

  /* anchor our out time to when the channel is opened */
  this->tsout = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );

  this->seqout = 0;

  for( int i = 0; i < BUFFERDELAYCOUNT; i++ )
  {
    this->orderedrtpdata[ i ] = nullptr;
  }
  this->orderedinminsn = 0;
  this->orderedinmaxsn = 0;
  this->orderedinbottom = 0;
  this->lastprocessed = nullptr;

  this->control = control;
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
    this->player = nullptr;

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
      /* release the shared pointer */
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
#ifdef SIMULATEDPACKETLOSSRATE
          /* simulate packet loss */
          if( 0 == rand() % SIMULATEDPACKETLOSSRATE )
          {
            if( !ec && bytes_recvd && this->active )
            {
              this->readsomertp();
            }
            return;
          }
#endif

          this->receivedpkcount++;
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
          this->handlertpdata();
        }

        if( !ec && bytes_recvd && this->active )
        {
           this->readsomertp();
        }
      } );
}


/*!md
## handlertpdata

Buffer up RTP data to reorder and give time for packets to be received then process.
*/
void projectrtpchannel::handlertpdata( void )
{
  rtppacket *src = &this->rtpdata[ this->rtpindexoldest ];
  this->rtpindexoldest = ( this->rtpindexoldest + 1 ) % BUFFERPACKETCOUNT;

  /* first task - reorder */
  uint32_t sn = src->getsequencenumber();

  if( 1 == this->receivedpkcount )
  {
    /* empty */
    this->orderedrtpdata[ 0 ] = src;
    this->orderedinminsn = sn;
    this->orderedinmaxsn = this->orderedinminsn;
    this->orderedinbottom = 0;
    this->ssrcin = src->getssrc();
    return;
  }
  else
  {
    if( this->ssrcin != src->getssrc() )
    {
      /* I think this should stay the same */
      return;
    }

    uint32_t aheadby = sn - this->orderedinminsn;

    if( aheadby > ( 4294967296 / 2 ) )
    {
      /* Too old */
      return;
    }

    if( aheadby > BUFFERPACKETCOUNT )
    {
      /* A bit ahead - so we probably suffered with silence - restart */
      this->orderedrtpdata[ 0 ] = src;
      this->orderedinminsn = sn;
      this->orderedinmaxsn = this->orderedinminsn;
      this->orderedinbottom = 0;
      return;
    }

    if( sn > this->orderedinmaxsn ) this->orderedinmaxsn = sn;
    this->orderedrtpdata[ ( this->orderedinbottom + aheadby ) % BUFFERPACKETCOUNT ] = src;

    while( aheadby > 10 )
    {
      uint32_t workingonaheadby;
      uint32_t workingonsn;
      uint32_t lastworkedonsn;

      src = this->orderedrtpdata[ this->orderedinbottom ];
      this->orderedinbottom = ( this->orderedinbottom + 1 ) % BUFFERPACKETCOUNT;

      if( nullptr == src )
      {
        goto whilecontinue;
      }

      if( nullptr != this->lastprocessed )
      {
        lastworkedonsn = this->lastprocessed->getsequencenumber();
      }
      else
      {
        lastworkedonsn = src->getsequencenumber() - 1;
      }

      workingonsn = src->getsequencenumber();
      workingonaheadby = workingonsn - lastworkedonsn;
      
      if( this->orderedinminsn != workingonsn )
      {
        /* Not a packet we are interested in */
        goto whilecontinue;
      }

      /* At this point we should be in order - but may have breaks (missing chunks) */
      this->processrtpdata( src, workingonaheadby - 1 );
      this->lastprocessed = src;

whilecontinue:
      this->orderedinminsn++;
      aheadby--;
    }
  }
}


/*!md
## processrtpdata

Mix and send the data somewhere.
*/
void projectrtpchannel::processrtpdata( rtppacket *src, uint32_t skipcount )
{
  if( 0 != skipcount )
  {
    this->codecworker.restart();
  }

  this->checkfornewmixes();

  /* only us */
  if( !this->others || 0 == this->others->size() )
  {
    rtppacket *out = this->gettempoutbuf( skipcount );

    stringptr newplaydef = std::atomic_exchange( &this->newplaydef, stringptr( NULL ) );
    if( newplaydef )
    {
      try
      {
        if( !this->player )
        {
          this->player = soundsoup::create();
        }

        JSON::Value ob = JSON::parse( *newplaydef );
        this->player->config( JSON::as_object( ob ), out->getpayloadtype() );
      }
      catch(...)
      {
        std::cerr << "Bad sound soup: " << *newplaydef << std::endl;
      }
    }

    if( this->player )
    {
      rawsound r = player->read();
      if( 0 != r.size() )
      {
        this->codecworker << codecx::next;
        this->codecworker << r;
        *out << this->codecworker;

        this->writepacket( out );
      }
    }

    return;
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

    this->codecworker << codecx::next;
    this->codecworker << *src;
    rtppacket *dst = chan->gettempoutbuf( skipcount );
    *dst << this->codecworker;
    chan->writepacket( dst );
  }

  return;
}


/*!md
## gettempoutbuf
When we need a buffer to send data out (because we cannot guarantee our own buffer will be available) we can use the circular out buffer on this channel. This will return the next one available.

We assume this is called to send packets out in order, and at intervals required for each timestamp to be incremented in lou of it payload type.
*/
rtppacket *projectrtpchannel::gettempoutbuf( uint32_t skipcount )
{
  rtppacket *buf = &this->outrtpdata[ this->rtpoutindex ];
  this->rtpoutindex = ( this->rtpoutindex + 1 ) % BUFFERPACKETCOUNT;

  buf->init( this->ssrcout );
  buf->setpayloadtype( this->selectedcodec );

  this->seqout += skipcount;
  buf->setsequencenumber( this->seqout );

  if( skipcount > 0 )
  {
    this->tsout += ( buf->getticksperpacket() * skipcount );
  }

  buf->settimestamp( this->tsout );

  this->seqout++;

  return buf;
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
## writepacket
Send a [RTP] packet to our endpoint.
*/
void projectrtpchannel::writepacket( rtppacket *pk )
{
  if( this->receivedrtp || this->targetconfirmed )
  {
    this->tsout = pk->getnexttimestamp();

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
Add the other to our list of others. n way relationship. Adds to queue for when our main thread calls into us.
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

  this->mixqueue.push( other );

  return true;
}

/*!md
## checkfornewmixes
This is the mechanism how we can use multiple threads and not screw u our data structures - without using mutexes.
*/
void projectrtpchannel::checkfornewmixes( void )
{
  projectrtpchannel::pointer other;

  while( this->mixqueue.pop( other ) )
  {
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
  }
}

/*!md
## audio
The CODECs on the other end which are acceptable. The first one should be the preferred. For now we keep hold of the list of codecs as we may be using them in the future. Filter out non-RTP streams (such as DTMF).
*/
void projectrtpchannel::audio( codeclist codecs )
{
#warning Flip this - we just need to ignore codecs like 2833 which is not really a codec
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

