

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

/* The number of bytes in a packet */
#define G711PAYLOADBYTES 80
#define G722PAYLOADBYTES 80
#define ILBC20PAYLOADBYTES 38
#define ILBC30PAYLOADBYTES 50

/* Need to double check max RTP length with variable length header - there could be a larger length withour CODECs */
#define RTPMAXLENGTH 80
#define RTCPMAXLENGTH 1500

/* The number of packets we will keep in a buffer */
#define BUFFERPACKETCOUNT 20

class rtppacket
{
public:
  size_t length;
  unsigned char pk[ RTPMAXLENGTH ];

  uint8_t getpacketversion();
  uint8_t getpacketpadding();
  uint8_t getpacketextension();
  uint8_t getpacketcsrccount();
  uint8_t getpacketmarker();
  uint8_t getpayloadtype();
  uint16_t getsequencenumber();
  uint32_t gettimestamp();
  uint32_t getssrc();
  uint32_t getcsrc( uint8_t index );
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
  rtppacket *pop( void );
  rtppacket *popordered( uint32_t ts );

private:
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

  rtppacket rtpdata[ BUFFERPACKETCOUNT ];
  unsigned char rtcpdata[ RTCPMAXLENGTH ];

  std::atomic_int rtpindexoldest;
  std::atomic_int rtpindexin;

  void readsomertp( void );
  void readsomertcp( void );

  void handlertcpdata( void );
  void handletargetresolve (
              boost::system::error_code e,
              boost::asio::ip::udp::resolver::iterator it );

  std::atomic_bool active;
  uint32_t timestampdiff;

  codeclist codecs;
  int selectedcodec;
};

/*typedef std::list< projectrtpchannel::pointer > channellist;
typedef boost::shared_ptr< channellist > channellistptr;*/

#endif
