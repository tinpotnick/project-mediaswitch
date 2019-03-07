

#ifndef PROJECTRTPCHANNEL_H
#define PROJECTRTPCHANNEL_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>

#include <boost/shared_ptr.hpp>

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
  projectrtpchannel( boost::asio::io_service &io_service, short port );
  ~projectrtpchannel( void );

  typedef boost::shared_ptr< projectrtpchannel > pointer;
  static pointer create( boost::asio::io_service &io_service, short port );

  enum{ PCMA, PCMU, ILBC20, ILBC30, G722 };
  void open( int codec );
  void close( void );

private:
  short port;
  boost::asio::ip::udp::socket rtpsocket;
  boost::asio::ip::udp::socket rtcpsocket;

  boost::asio::ip::udp::endpoint rtpsenderendpoint;
  boost::asio::ip::udp::endpoint rtcpsenderendpoint;

  char *rtpdata;
  char *rtcpdata;

  int rtpindex;

  void readsomertp( void );
  void readsomertcp( void );

  void handlertpdata( void );
  void handlertcpdata( void );

  /*
    For the following to works:
    char = 8 bits
    short = 16 bits
    int= 32 bits
  */
  inline unsigned char getpacketversion( unsigned char *pk );
  inline unsigned char getpacketpadding( unsigned char *pk );
  inline unsigned char getpacketextension( unsigned char *pk );
  inline unsigned char getpacketcsrccount( unsigned char *pk );
  inline unsigned char getpacketmarker( unsigned char *pk );
  inline unsigned char getpayloadtype( unsigned char *pk );
  inline unsigned short getsequencenumber( unsigned char *pk );
  inline unsigned int gettimestamp( unsigned char *pk );
  inline unsigned int getssrc( unsigned char *pk );
  inline unsigned int getcsrc( unsigned char *pk, unsigned char index );

};

#endif

