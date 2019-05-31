
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "projecthttpclient.h"

/* Debugging */
#include <iostream>


/*!md
## create
Static function to create a shared pointer.

*/
projecthttpclient::pointer projecthttpclient::create( boost::asio::io_context& iocontext )
{
  return pointer( new projecthttpclient( iocontext ) );
}

/*!md
## projecthttpclient
Constructor

*/
projecthttpclient::projecthttpclient( boost::asio::io_service &ioservice ) :
  ioservice( ioservice ),
  socket( ioservice ),
  resolver( ioservice ),
  timer( ioservice ),
  socketopen( false )
{
}

/*!md
## projecthttpclient
Destructor

*/
projecthttpclient::~projecthttpclient()
{
}


/*!md
## asynccancel
Cancel any requests

*/
void projecthttpclient::asynccancel( void )
{
  this->callback = std::bind( &projecthttpclient::nullfunction, this, std::placeholders::_1 );
  this->timer.cancel();
}

/*!md
## asyncrequest
Make a HTTP request with callback.

*/
void projecthttpclient::asyncrequest( projectwebdocumentptr request,
      std::function< void ( int errorcode ) > callback )
{
  this->callback = callback;

  this->uri =  httpuri( request->getrequesturi() );
  this->requestdoc = request->strptr();

  this->timer.expires_after( std::chrono::seconds( HTTPCLIENTDEFAULTTIMEOUT ) );
  this->timer.async_wait( boost::bind( &projecthttpclient::handletimeout,
                                        shared_from_this(),
                                        boost::asio::placeholders::error ) );

  std::string port = "80";
  if( 0 != this->uri.port.end() )
  {
    port = this->uri.port.str();
  }

  if( this->socketopen )
  {
    boost::asio::async_write(
      this->socket,
      boost::asio::buffer( this->requestdoc->c_str(), this->requestdoc->length() ),
        boost::bind( &projecthttpclient::handlewrite, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred ) );
  }
  else
  {
    boost::asio::ip::tcp::resolver::query query( this->uri.host.str(), port.c_str() );
    this->resolver.async_resolve( query,
        boost::bind( &projecthttpclient::handleresolve, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::iterator ) );
  }
}

/*!md
## handleresolve
We have resolved (or not)

*/
void projecthttpclient::handleresolve(
            boost::system::error_code e,
            boost::asio::ip::tcp::resolver::iterator it )
{
  boost::asio::ip::tcp::resolver::iterator end;

  if( it == end )
  {
    this->callback( FAIL_RESOLVE );
    this->asynccancel();
    return;
  }

  this->socket.async_connect( *it,
    boost::bind( &projecthttpclient::handleconnect, shared_from_this(),
          boost::asio::placeholders::error ) );

}

/*!md
## handleread
Our read handler.

*/
void projecthttpclient::handleconnect( boost::system::error_code errorcode )
{
  if( errorcode != boost::asio::error::already_connected && errorcode )
  {
    this->callback( FAIL_CONNECT );
    this->asynccancel();
    return;
  }

  this->socketopen = true;

  boost::asio::async_write(
    this->socket,
    boost::asio::buffer( this->requestdoc->c_str(), this->requestdoc->length() ),
    boost::bind( &projecthttpclient::handlewrite, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred ) );
}

/*!md
## handlewrite
Our read handler.

*/
void projecthttpclient::handlewrite( boost::system::error_code errorcode, std::size_t bytestransferred )
{
  if( errorcode )
  {
    this->callback( FAIL_WRITE );
    this->asynccancel();
    return;
  }

  this->socket.async_read_some(
    boost::asio::buffer( this->inbounddata, this->inbounddata.size() ),
    boost::bind( &projecthttpclient::handleread, shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred ) );

}

/*!md
## handleread
Our read handler.

*/
void projecthttpclient::handleread( boost::system::error_code errorcode, std::size_t bytestransferred )
{
  if( boost::asio::error::eof == errorcode )
  {
    /*
      This indicates the other end has closed the socket - reconnect.
      This is the only way to detect a disconnect from a client - after we attempt a read.
    */
    this->socketopen = false;
    this->socket.close();

    std::string port = "80";
    if( 0 != this->uri.port.end() )
    {
      port = this->uri.port.str();
    }

    boost::asio::ip::tcp::resolver::query query( this->uri.host.str(), port.c_str() );
    this->resolver.async_resolve( query,
        boost::bind( &projecthttpclient::handleresolve, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::iterator ) );
    return;
  }

  if( errorcode )
  {
    this->callback( FAIL_READ );
    this->asynccancel();
    return;
  }

  if( 0 == bytestransferred )
  {
    this->callback( FAIL_READ );
    this->asynccancel();
    return;
  }

  this->inbounddata[ bytestransferred ] = 0;
  stringptr p( new std::string( (const char*)this->inbounddata.data() ) );
  this->response = projectwebdocumentptr( new projectwebdocument( p ) );

  this->timer.cancel();
  this->callback( 0 );
  return;
}

/*!md
## handletimeout
Our timer callback.

*/
void projecthttpclient::handletimeout( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    this->callback( FAIL_TIMEOUT );
    this->asynccancel();
  }
}


/*!md
## getresponse
Return the response document.

*/
projectwebdocumentptr projecthttpclient::getresponse( void )
{
  return this->response;
}
