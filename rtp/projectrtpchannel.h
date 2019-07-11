

#ifndef PROJECTRTPCHANNEL_H
#define PROJECTRTPCHANNEL_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <arpa/inet.h>

#include <list>
#include <vector>

/* CODECs */
#include <ilbc.h>
#include <spandsp.h>

#include "firfilter.h"

/* The number of bytes in a packet ( these figure are less some overhead G711 = 172*/
#define G711PAYLOADBYTES 160
#define G722PAYLOADBYTES 160
#define ILBC20PAYLOADBYTES 38
#define ILBC30PAYLOADBYTES 50

#define PCMUPAYLOADTYPE 0
#define PCMAPAYLOADTYPE 8
#define G722PAYLOADTYPE 9
#define ILBCPAYLOADTYPE 97

/* Need to double check max RTP length with variable length header - there could be a larger length withour CODECs */
/* this maybe breached if a stupid number of csrc count is high */
#define RTPMAXLENGTH 200
#define L16MAXLENGTH ( RTPMAXLENGTH * 2 )
#define RTCPMAXLENGTH 1500

/* The number of packets we will keep in a buffer */
#define BUFFERPACKETCOUNT 20

class rtppacket
{
public:
  rtppacket();
  rtppacket( rtppacket & );
  size_t length;
  uint8_t pk[ RTPMAXLENGTH ];
  int16_t l168k[ RTPMAXLENGTH ]; /* narrow band */
  int16_t l1616k[ L16MAXLENGTH ]; /* wideband */
  bool havel168k;
  bool havel1616k;

  uint8_t getpacketversion( void );
  uint8_t getpacketpadding( void );
  uint8_t getpacketextension( void );
  uint8_t getpacketcsrccount( void );
  uint8_t getpacketmarker( void );
  uint8_t getpayloadtype( void );
  uint16_t getsequencenumber( void );
  uint32_t gettimestamp( void );
  uint32_t getssrc( void );
  uint32_t getcsrc( uint8_t index );
  uint8_t *getpayload( void );

  void setpayloadtype( uint8_t payload );
  void setsequencenumber( uint16_t sq );
  void settimestamp( uint32_t tmstp );

  void copy( rtppacket *src );
  void copyheader( rtppacket *src );

  /* conversion functions */
  void xlaw2ylaw( rtppacket *in );
  void g722tol16( g722_decode_state_t *g722decoder );
  void l16tog722( g722_encode_state_t *g722encoder, rtppacket *l16src );
  void g711tol16( void );
  void l16tog711( uint8_t payloadtype, rtppacket *l16src );

  void l16lowtowideband(  int16_t *lastsample  );
  void l16widetonarrowband( lowpass3_4k16k &filter );
};

/*!md
# projectrtpchannel
Purpose: RTP Channel - which represents RTP and RTCP. This is here we include our jitter buffer. We create a cyclic window to write data into and then read out of.

RTP on SIP channels should be able to switch between CODECS during a session so we have to make sure we have space for that.

RTP Payload size (bytes) for the following CODECs
G711: 80 (10mS)
G722: 80 (10mS)
ilbc (mode 20): 38 (20mS)
ilbc (mode 30): 50 (30mS)
Question, What do we write this data to? We need to limit transcoding

Notes about transcoding.

We receive RTP data into rtpdata round robin buffer. When we transcode (and we will support PCMA PCMU iLBC and G722) we can do things like

PCMA->PCMU in one go (very efficiently)
PCMU->PCMA (also very efficient)

To do
PCMA-G722 we actually have to PCMA-L16-G722 which is also true of iLBC. If we also do things like conference (mixing to multiple channels) and call recording then the complexity also increases - especially if we want to become low cpu.

if PCMA to PCMU and no recording and only 2 channels then direct conversion

Buffer?

In our RTP channel, we can have a fixed buffer for inbound L16 (i.e. transcoded).

In our mixed channel would contain the buffer to store the 2nd transcoded leg (i.e. PCMA->L16->G722) channel a has the PCMA in the inbuffer, the L16 in the transcoded buffer and the G722 would be stored in an out buffer of the other channel which requires it.
*/
class projectrtpchannel :
  public boost::enable_shared_from_this< projectrtpchannel >
{

public:
  projectrtpchannel( boost::asio::io_service &io_service, unsigned short port );
  ~projectrtpchannel( void );

  typedef boost::shared_ptr< projectrtpchannel > pointer;
  static pointer create( boost::asio::io_service &io_service, unsigned short port );

  void open();
  void close( void );

  unsigned short getport( void );

  void target( std::string &address, unsigned short port );

  typedef std::vector< int > codeclist;
  void audio( codeclist codecs );

  void writepacket( rtppacket * );
  void handlesend(
        const boost::system::error_code& error,
        std::size_t bytes_transferred);

  bool canread( void ) { return this->reader; };
  bool canwrite( void ) { return this->writer; };

  bool isactive( void );

  bool mix( projectrtpchannel::pointer other );
  rtppacket *gettempoutbuf( void );

  codeclist codecs;
  int selectedcodec;

  rtppacket rtpdata[ BUFFERPACKETCOUNT ];
  unsigned char rtcpdata[ RTCPMAXLENGTH ];
  int rtpindexoldest;
  int rtpindexin;

  /* The out data is intended to be written by other channels (or functions), they can then be sent to other channels as well as our own end point  */
  rtppacket outrtpdata[ BUFFERPACKETCOUNT ];
  int rtpoutindex;

private:
  bool active;
  unsigned short port;

  boost::asio::ip::udp::resolver resolver;

  boost::asio::ip::udp::socket rtpsocket;
  boost::asio::ip::udp::socket rtcpsocket;

  boost::asio::ip::udp::endpoint rtpsenderendpoint;
  boost::asio::ip::udp::endpoint confirmedrtpsenderendpoint;
  boost::asio::ip::udp::endpoint rtcpsenderendpoint;

  /* confirmation of where the other end of the RTP stream is */
  bool receivedrtp;
  bool targetconfirmed;

  bool reader;
  bool writer;
  void readsomertp( void );
  void readsomertcp( void );

  void handlertpdata( void );
  void handlertcpdata( void );
  void handletargetresolve (
              boost::system::error_code e,
              boost::asio::ip::udp::resolver::iterator it );

  bool isl16required( rtppacket *src );
  bool isl16widebandrequired( rtppacket *src );
  bool isl16narrowbandrequired( rtppacket *src );

  uint32_t timestampdiff;
  uint64_t receivedpkcount;

  typedef std::list< projectrtpchannel::pointer > projectrtpchannellist;
  typedef boost::shared_ptr< projectrtpchannellist > projectrtpchannellistptr;
  projectrtpchannellistptr others;

  /* CODECs  */
  g722_encode_state_t *g722encoder;
  g722_decode_state_t *g722decoder;

  /* If we require downsampling */
  lowpass3_4k16k lpfilter;
  int16_t resamplelastsample; /* When we upsample we need to interpolate so need last sample */
};


/* Functions */
void gen711convertdata( void );

#endif
