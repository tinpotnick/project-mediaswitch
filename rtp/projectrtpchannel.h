

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
#include "projectrtpcodecx.h"
#include "projectrtppacket.h"

/* The number of packets we will keep in a buffer */
#define BUFFERPACKETCOUNT 20
#define BUFFERDELAYCOUNT 10


/*!md
# projectrtpchannel
Purpose: RTP Channel - which represents RTP and RTCP. This is here we include our jitter buffer. We create a cyclic window to write data into and then read out of.

RTP on SIP channels should be able to switch between CODECS during a session so we have to make sure we have space for that.

*/


typedef std::function<bool( rtppacket *pk )> rtppacketplayback;
typedef std::list< rtppacketplayback > rtppacketplaybacks;


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
  uint32_t ssrcout;
  uint32_t ssrcin;
  uint32_t tsout;
  uint16_t seqout;

  rtppacket rtpdata[ BUFFERPACKETCOUNT ];
  rtppacket *orderedrtpdata[ BUFFERPACKETCOUNT ];
  rtppacket *lastprocessed;
  uint32_t orderedinminsn; /* sn = sequence number, min smallest we hold which is unprocessed - when it is processed we can forget about it */
  uint32_t orderedinmaxsn;
  int orderedinbottom; /* points to our min sn packet */

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
  void processrtpdata( rtppacket *src, bool inorder );
  void handlertcpdata( void );
  void handletargetresolve (
              boost::system::error_code e,
              boost::asio::ip::udp::resolver::iterator it );


  uint32_t timestampdiff;
  uint64_t receivedpkcount;

  typedef std::list< projectrtpchannel::pointer > projectrtpchannellist;
  typedef boost::shared_ptr< projectrtpchannellist > projectrtpchannellistptr;
  projectrtpchannellistptr others;

  /* CODECs  */
  codecx codecworker;

  rtppacketplaybacks players;
};


#endif
