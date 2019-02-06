

#ifndef PROJECTHTTPCLIENT_H
#define PROJECTHTTPCLIENT_H

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <functional>

#include <boost/asio/placeholders.hpp>

#include "projectwebdocument.h"
#include "projectsipstring.h"

#define HTTPCLIENTDEFAULTTIMEOUT 10

/*
TODO 

1. Get the io context within this file. Anyone file using
a webclient doens't need to know about io contexts.

2. The callback shoud supply the doument.

*/

/*******************************************************************************
Class: projecthttpclient
Purpose: Simple async HTTP Client.
Updated: 18.01.2019
*******************************************************************************/
class projecthttpclient :
  public boost::enable_shared_from_this< projecthttpclient >
{
public:
  projecthttpclient();
  typedef boost::shared_ptr< projecthttpclient > pointer;
  static pointer create( boost::asio::io_context& iocontext );

  projecthttpclient( boost::asio::io_service &ioservice );
  ~projecthttpclient();

  enum{ FAIL_RESOLVE = 1, FAIL_CONNECT, FAIL_READ, FAIL_WRITE, FAIL_TIMEOUT };

  void asynccancel( void );
  void asyncrequest( projectwebdocumentptr request, 
      std::function< void ( int errorcode ) > );

  projectwebdocumentptr getresponse( void );

  void handleresolve( 
            boost::system::error_code e, 
            boost::asio::ip::tcp::resolver::iterator it );
  void handleconnect( boost::system::error_code errorcode );
  void handleread( boost::system::error_code errorcode, std::size_t bytestransferred );
  void handlewrite( boost::system::error_code errorcode, std::size_t bytestransferred );
  void handletimeout( const boost::system::error_code& error );

  void nullfunction( int ){};

private:
  boost::asio::io_service &ioservice;
  boost::asio::ip::tcp::socket socket;
  std::function< void ( int errorcode ) > callback;
  stringptr requestdoc;
  projectwebdocumentptr response;

  chararray inbounddata;

  boost::asio::ip::tcp::resolver resolver;
  boost::asio::steady_timer timer;
};

#endif /* PROJECTHTTPCLIENT_H */

