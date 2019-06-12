

#ifndef PROJECTRTPCHANNEL_H
#define PROJECTRTPCHANNEL_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>

#include <boost/shared_ptr.hpp>

#include <stdint.h>
#include <arpa/inet.h>

#define RTPMAXLENGTH 1500
#define RTCPMAXLENGTH 1500

/* The number of bytes in a packet */
#define G711PAYLOADBYTES 80
#define G722PAYLOADBYTES 80
#define ILBC20PAYLOADBYTES 38
#define ILBC30PAYLOADBYTES 50

/* The number of packets we will keep in a buffer */
#define BUFFERPACKETCOUNT 20

/*******************************************************************************
Class: projectrtpchannel
Purpose: RTP Channel - which represents RTP and RTCP. This is here we include
our jitter buffer. We create a cyclic window to write data into and then read
out of.

RTP Payload size (bytes) for the following CODECs
G711: 80 (10mS)
G722: 80 (10mS)
ilbc (mode 20): 38 (20mS)
ilbc (mode 30): 50 (30mS)
Question, What do we write this data to? We need to limit transcoding
Updated: 01.03.2019
*******************************************************************************/
class projectrtpchannel :
  public boost::enable_shared_from_this< projectrtpchannel >
{

public:
  projectrtpchannel( boost::asio::io_service &io_service, unsigned short port );
  ~projectrtpchannel( void );

  typedef boost::shared_ptr< projectrtpchannel > pointer;
  static pointer create( boost::asio::io_service &io_service, unsigned short port );

  enum{ PCMA, PCMU, ILBC20, ILBC30, G722 };
  void open( int codec );
  void close( void );

  unsigned short getport( void );

  void bridgeto( pointer other );
  void target( std::string &address, unsigned short port );

  void writepacket( uint8_t *pk, size_t length );
  void handlesend(
        const boost::system::error_code& error,
        std::size_t bytes_transferred);

private:
  projectrtpchannel::pointer bridgedto;
  unsigned short port;

  boost::asio::ip::udp::resolver resolver;

  boost::asio::ip::udp::socket rtpsocket;
  boost::asio::ip::udp::socket rtcpsocket;

  boost::asio::ip::udp::endpoint rtpsenderendpoint;
  boost::asio::ip::udp::endpoint confirmedrtpsenderendpoint;
  boost::asio::ip::udp::endpoint rtcpsenderendpoint;

  bool receivedrtp;
  bool targetconfirmed;

  unsigned char *rtpdata;
  unsigned char *rtcpdata;

  int rtpindex;

  void readsomertp( void );
  void readsomertcp( void );

  void handlertpdata( std::size_t );
  void handlertcpdata( void );
  void handletargetresolve (
              boost::system::error_code e,
              boost::asio::ip::udp::resolver::iterator it );

  uint8_t getpacketversion( uint8_t *pk );
  uint8_t getpacketpadding( uint8_t *pk );
  uint8_t getpacketextension( uint8_t *pk );
  uint8_t getpacketcsrccount( uint8_t *pk );
  uint8_t getpacketmarker( uint8_t *pk );
  uint8_t getpayloadtype( uint8_t *pk );
  uint16_t getsequencenumber( uint8_t *pk );
  uint32_t gettimestamp( uint8_t *pk );
  uint32_t getssrc( uint8_t *pk );
  uint32_t getcsrc( uint8_t *pk, uint8_t index );

};

#endif
