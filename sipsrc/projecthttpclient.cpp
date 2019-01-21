
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/placeholders.hpp>

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
      std::function< void ( boost::system::error_code errorcode ) > callback )
{
  this->callback = callback;
  boost::asio::ip::tcp::resolver resolve{ this->ioservice };

  httpuri uri( request->getrequesturi() );
  this->requestdoc = request->strptr();

  resolve.async_resolve( boost::asio::ip::tcp::resolver::query( uri.host.str(), "80" ), 
    [ this ]( boost::system::error_code errorcode, boost::asio::ip::tcp::resolver::iterator it )
    {
      if( errorcode )
      {
        this->callback( errorcode );
        return;
      }
      this->socket.async_connect( *it, 
      [ this ]( const boost::system::error_code &errorcode )
      {
        if( errorcode )
        {
          this->callback( errorcode );
          return;
        }
        boost::asio::async_write(
          this->socket,
          boost::asio::buffer( this->requestdoc->c_str(), this->requestdoc->length() ),
          [ this ]( boost::system::error_code errorcode, std::size_t bytestransferred )
          {
            if( errorcode )
            {
              this->callback( errorcode );
              return;
            }
              boost::asio::async_read(
              this->socket,
              boost::asio::buffer( this->inbounddata ),
              [ this ]( boost::system::error_code errorcode, std::size_t bytestransferred )
              {
                this->callback( errorcode );
                return;
              } );
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

