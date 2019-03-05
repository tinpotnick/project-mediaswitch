

#ifndef PROJECTRTPCHANNEL_H
#define PROJECTRTPCHANNEL_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>

#include <boost/shared_ptr.hpp>

#define RTPBUFFERLENGTH 50
#define RTPD


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
  enum{ PCMA, PCMU, ILBC20, ILBC30, G722 };
  projectrtpchannel( boost::asio::io_service &io_service, short port );

  typedef boost::shared_ptr< projectrtpchannel > pointer;
  static pointer create( boost::asio::io_service &io_service, short port );

  void open( void );
  void close( void );

private:
  short port;
  boost::asio::ip::udp::socket rtpsocket;
  boost::asio::ip::udp::socket rtcpsocket;

  boost::asio::ip::udp::endpoint rtpsenderendpoint;
  boost::asio::ip::udp::endpoint rtcpsenderendpoint;

  enum { max_length = 1500 };
  char data[ max_length ];
  char rtcpdata[ max_length ];
  int bytesreceived;

  void readsomertp( void );
  void readsomertcp( void );

  void handlertpdata( void );
  void handlertcpdata( void );
};

#endif

