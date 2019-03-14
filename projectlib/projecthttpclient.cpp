
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "projecthttpclient.h"

/* Debugging */
#include <iostream>


/*******************************************************************************
Function: create
Purpose: Static function to create a shared pointer.
Updated: 18.01.2019
*******************************************************************************/
projecthttpclient::pointer projecthttpclient::create( boost::asio::io_context& iocontext )
{
  return pointer( new projecthttpclient( iocontext ) );
}

/*******************************************************************************
Function: projecthttpclient
Purpose: Constructor
Updated: 18.01.2019
*******************************************************************************/
projecthttpclient::projecthttpclient( boost::asio::io_service &ioservice ) :
  ioservice( ioservice ),
  socket( ioservice ),
  resolver( ioservice ),
  timer( ioservice )
{
}

/*******************************************************************************
Function: projecthttpclient
Purpose: Destructor
Updated: 29.01.2019
*******************************************************************************/
projecthttpclient::~projecthttpclient()
{
}


/*******************************************************************************
Function: asynccancel
Purpose: Cancel any requests
Updated: 06.02.2019
*******************************************************************************/
void projecthttpclient::asynccancel( void )
{
  this->callback = std::bind( &projecthttpclient::nullfunction, this, std::placeholders::_1 );
  this->timer.cancel();
}

/*******************************************************************************
Function: asyncrequest
Purpose: Make a HTTP request with callback.
Updated: 18.01.2019
*******************************************************************************/
void projecthttpclient::asyncrequest( projectwebdocumentptr request, 
      std::function< void ( int errorcode ) > callback )
{
  this->callback = callback;

  httpuri uri( request->getrequesturi() );
  this->requestdoc = request->strptr();

  this->timer.expires_after( std::chrono::seconds( HTTPCLIENTDEFAULTTIMEOUT ) );
  this->timer.async_wait( boost::bind( &projecthttpclient::handletimeout, 
                                        shared_from_this(), 
                                        boost::asio::placeholders::error ) );

  boost::asio::ip::tcp::resolver resolver( this->ioservice );

  std::string port = "80";
  if( 0 != uri.port.end() )
  {
    port = uri.port.str();
  }

  boost::asio::ip::tcp::resolver::query query( uri.host.str(), port.c_str() );
  this->resolver.async_resolve( query,
      boost::bind( &projecthttpclient::handleresolve, shared_from_this(),
        boost::asio::placeholders::error,
          boost::asio::placeholders::iterator ) );

}

/*******************************************************************************
Function: handleresolve
Purpose: We have resolved (or not)
Updated: 18.01.2019
*******************************************************************************/
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

/*******************************************************************************
Function: handleread
Purpose: Our read handler.
Updated: 29.01.2019
*******************************************************************************/
void projecthttpclient::handleconnect( boost::system::error_code errorcode )
{
  if( errorcode != boost::asio::error::already_connected && errorcode )
  {
    this->callback( FAIL_CONNECT );
    this->asynccancel();
    return;
  }

  boost::asio::async_write(
    this->socket,
    boost::asio::buffer( this->requestdoc->c_str(), this->requestdoc->length() ),
    boost::bind( &projecthttpclient::handlewrite, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred ) );
}

/*******************************************************************************
Function: handlewrite
Purpose: Our read handler.
Updated: 29.01.2019
*******************************************************************************/
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

/*******************************************************************************
Function: handleread
Purpose: Our read handler.
Updated: 29.01.2019
*******************************************************************************/
void projecthttpclient::handleread( boost::system::error_code errorcode, std::size_t bytestransferred )
{
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

/*******************************************************************************
Function: handletimeout
Purpose: Our timer callback.
Updated: 29.01.2019
*******************************************************************************/
void projecthttpclient::handletimeout( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    this->callback( FAIL_TIMEOUT );
    this->asynccancel();
  }
}


/*******************************************************************************
Function: getresponse
Purpose: Return the response document.
Updated: 18.01.2019
*******************************************************************************/
projectwebdocumentptr projecthttpclient::getresponse( void )
{
  return this->response;
}

