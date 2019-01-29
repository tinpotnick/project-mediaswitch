
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "projecthttpclient.h"


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
  socket( ioservice )
{
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

  boost::asio::ip::tcp::resolver resolver( this->ioservice );
  #warning
  // TODO - add a port number to the host.
  boost::asio::ip::tcp::resolver::query query( uri.host.str(), "3000" );
  boost::asio::ip::tcp::resolver::iterator it = resolver.resolve( query );
  boost::asio::ip::tcp::resolver::iterator end;

  if( it == end )
  {
    this->callback( FAIL_RESOLVE );
    return;
  }
  this->socket.async_connect( *it, 
  [ this ]( const boost::system::error_code &errorcode )
  {
    if( errorcode )
    {
      this->callback( FAIL_CONNECT );
      return;
    }
    boost::asio::async_write(
      this->socket,
      boost::asio::buffer( this->requestdoc->c_str(), this->requestdoc->length() ),
      [ this ]( boost::system::error_code errorcode, std::size_t bytestransferred )
      {
        if( errorcode )
        {
          this->callback( FAIL_WRITE );
          return;
        }
          boost::asio::async_read(
          this->socket,
          boost::asio::buffer( this->inbounddata ),
          [ this ]( boost::system::error_code errorcode, std::size_t bytestransferred )
          {
            if( errorcode )
            {
              this->callback( FAIL_READ );
            }
            
            this->callback( 0 );
            return;
          } );
      } );
  } );
}


/*******************************************************************************
Function: getresponse
Purpose: Return the response document.
Updated: 18.01.2019
*******************************************************************************/
projectwebdocumentptr projecthttpclient::getresponse( void )
{
#warning TODO
  /* Parsed our response into the doc. */
  return this->response;
}

