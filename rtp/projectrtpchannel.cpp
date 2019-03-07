

#include <iostream>


#include "projectrtpchannel.h"

/*******************************************************************************
Function: projectrtpchannel constructor
Purpose: Create the socket then wait for data
Updated: 01.03.2019
echo "This is my data" > /dev/udp/127.0.0.1/10000
*******************************************************************************/
projectrtpchannel::projectrtpchannel( boost::asio::io_service &io_service, short port )
  : port( port ),
  rtpsocket( io_service ),
  rtcpsocket( io_service ),
  rtpdata( NULL ),
  rtcpdata( NULL ),
  rtpindex( 0 )
{
}

/*******************************************************************************
Function: projectrtpchannel destructor
Purpose: Clean up
Updated: 05.03.2019
*******************************************************************************/
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

/*******************************************************************************
Function: create
Purpose: 
Updated: 01.03.2019
*******************************************************************************/
projectrtpchannel::pointer projectrtpchannel::create( boost::asio::io_service &io_service, short port )
{
  return pointer( new projectrtpchannel( io_service, port ) );
}

/*******************************************************************************
Function: open
Purpose: Open the channel to read network data. Setup memory and pointers.
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::open( int codec )
{
  switch( codec )
  {
    case PCMA:
    case PCMU:
    {
      this->rtpdata = new char[ G711PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
    case G722:
    {
      this->rtpdata = new char[ G722PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
    case ILBC20:
    {
      this->rtpdata = new char[ ILBC20PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
    case ILBC30:
    {
      this->rtpdata = new char[ ILBC30PAYLOADBYTES * BUFFERPACKETCOUNT ];
      break;
    }
  }

  this->rtcpdata = new char[ RTCPMAXLENGTH ];
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

/*******************************************************************************
Function: close
Purpose: Closes the channel.
Updated: 05.03.2019
*******************************************************************************/
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

/*******************************************************************************
Function: handlereadsomertp
Purpose: Wait for RTP data
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::readsomertp( void )
{
  this->rtpsocket.async_receive_from(
    boost::asio::buffer( &this->rtpdata[ this->rtpindex ], RTPMAXLENGTH ), this->rtpsenderendpoint,
      [ this ]( boost::system::error_code ec, std::size_t bytes_recvd )
      {
        if ( !ec && bytes_recvd > 0 && bytes_recvd <= RTPMAXLENGTH )
        {
          this->handlertpdata();
        }

        if( !ec )
        {
           this->readsomertp();
        }
      } );
}

/*******************************************************************************
Function: handlereadsomertcp
Purpose: Wait for RTP data
Updated: 01.03.2019
*******************************************************************************/
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


/*******************************************************************************
Function: handlertpdata
Purpose: We have received some RTP data - now do something with it.
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::handlertpdata( void )
{
  /* first check we have received the correct one and move if necessary */
}

/*******************************************************************************
Function: handlertcpdata
Purpose: We have received some RTCP data - now do something with it.
Updated: 01.03.2019
*******************************************************************************/
void projectrtpchannel::handlertcpdata( void )
{
  
}

/*******************************************************************************
Function: getpacketversion
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned char projectrtpchannel::getpacketversion( unsigned char *pk )
{
  return ( pk[ 0 ] & 0xb0 ) >> 6;
}

/*******************************************************************************
Function: getpacketpadding
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned char projectrtpchannel::getpacketpadding( unsigned char *pk )
{
  return ( pk[ 0 ] & 0x20 ) >> 5;
}

/*******************************************************************************
Function: getpacketextension
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned char projectrtpchannel::getpacketextension( unsigned char *pk )
{
  return ( pk[ 0 ] & 0x10 ) >> 4;
}

/*******************************************************************************
Function: getpacketcsrccount
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned char projectrtpchannel::getpacketcsrccount( unsigned char *pk )
{
  return ( pk[ 0 ] & 0x0f );
}

/*******************************************************************************
Function: getpacketmarker
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned char projectrtpchannel::getpacketmarker( unsigned char *pk )
{
  return ( pk[ 1 ] & 0x80 ) >> 7;
}

/*******************************************************************************
Function: getpayloadtype
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned char projectrtpchannel::getpayloadtype( unsigned char *pk )
{
  return ( pk[ 1 ] & 0x7f );
}

/*******************************************************************************
Function: getsequencenumber
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned short projectrtpchannel::getsequencenumber( unsigned char *pk )
{
  unsigned short *tmp = ( unsigned short * )pk;
  tmp++;
  return *tmp;
}

/*******************************************************************************
Function: gettimestamp
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned int projectrtpchannel::gettimestamp( unsigned char *pk )
{
  unsigned int *tmp = ( unsigned int * )pk;
  tmp++;
  return *tmp;
}

/*******************************************************************************
Function: getssrc
Purpose: As it says.
Updated: 06.03.2019
*******************************************************************************/
unsigned int projectrtpchannel::getssrc( unsigned char *pk )
{
  unsigned int *tmp = ( unsigned int * )pk;
  tmp += 2;
  return *tmp;
}

/*******************************************************************************
Function: getcsrc
Purpose: As it says. Use getpacketcsrccount to return the number of available
0-15. This function doesn't check bounds.
Updated: 06.03.2019
*******************************************************************************/
unsigned int projectrtpchannel::getcsrc( unsigned char *pk, unsigned char index )
{
  unsigned int *tmp = ( unsigned int * )pk;
  tmp += 3 + index;
  return *tmp;
}