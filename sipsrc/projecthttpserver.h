
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "projectsipstring.h"
#include "projectwebdocument.h"


// test
#include <array>

#ifndef PROJECTHTTPSERVER_H
#define PROJECTHTTPSERVER_H


/*******************************************************************************
Class: projecthttpconnection
Purpose: Our per connection object.
Updated: 22.01.2019
*******************************************************************************/
class projecthttpconnection
	: public boost::enable_shared_from_this< projecthttpconnection >
{
public:
  projecthttpconnection( boost::asio::io_service& io_service );
	typedef boost::shared_ptr< projecthttpconnection > pointer;

	static pointer create( boost::asio::io_service& io_service );
	boost::asio::ip::tcp::socket& socket();

  void start( void );
  void handleread( const boost::system::error_code& error, std::size_t bytes_transferred );
  void handlewrite( const boost::system::error_code& error, std::size_t bytes_transferred );
  void handletimeout( const boost::system::error_code& error );

private:
  boost::asio::io_service &ioservice;
  boost::asio::ip::tcp::socket tcpsocket;

  chararray inbuffer;
  stringptr outbuffer;

  projectwebdocument request;

  boost::asio::steady_timer timer;
};


/*******************************************************************************
Class: projecthttpserver
Purpose: Creates per connection object.
Updated: 22.01.2019
*******************************************************************************/
class projecthttpserver
{
public:
	projecthttpserver( boost::asio::io_service& io_service, short port );

private:
  void waitaccept( void );
	void handleaccept( projecthttpconnection::pointer new_connection,
		      const boost::system::error_code& error);

	boost::asio::ip::tcp::acceptor httpacceptor;
};

#endif /* PROJECTHTTPSERVER_H */

