

#include "projecthttpserver.h"
#include "projectwebdocument.h"
#include "projectsipregistrar.h"
#include "projectsipdialog.h"
#include "projectsipdirectory.h"

#include "json.hpp"

#include <iostream>

/*******************************************************************************
Function: projecthttpserver
Purpose: Constructor
Updated: 22.01.2019
*******************************************************************************/
projecthttpserver::projecthttpserver( boost::asio::io_service& io_service, short port ) :
		httpacceptor( io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port ) )
{
  this->waitaccept();
}

/*******************************************************************************
Function: waitaccept
Purpose: Wait for a new connection.
Updated: 22.01.2019
*******************************************************************************/
void projecthttpserver::waitaccept( void )
{
  projecthttpconnection::pointer newconnection =
    projecthttpconnection::create( httpacceptor.get_io_service() );

  httpacceptor.async_accept( newconnection->socket(),
    boost::bind( &projecthttpserver::handleaccept, this, newconnection,
    boost::asio::placeholders::error ) );
}


/*******************************************************************************
Function: handleaccept
Purpose: We have a new connection.
Updated: 22.01.2019
*******************************************************************************/
void projecthttpserver::handleaccept( 
        projecthttpconnection::pointer newconnection,
        const boost::system::error_code& error )
{
  if ( !error )
  {
    newconnection->start();
    this->waitaccept();
  }
}


/*******************************************************************************
Function: projecthttpconnection
Purpose: Constructor
Updated: 22.01.2019
*******************************************************************************/
projecthttpconnection::projecthttpconnection( boost::asio::io_service& ioservice ) :
  ioservice( ioservice ),
  tcpsocket( ioservice ),
  //inbuffer( new std::string() ),
  outbuffer( new std::string() ),
  timer( ioservice )
{

}

/*******************************************************************************
Function: create
Purpose: Create a new projecthttpconnection::pointer.
Updated: 22.01.2019
*******************************************************************************/
projecthttpconnection::pointer projecthttpconnection::create( boost::asio::io_service& io_service )
{
  return pointer( new projecthttpconnection( io_service ) );
}

/*******************************************************************************
Function: socket
Purpose: Returns a reference to our socket.
Updated: 22.01.2019
*******************************************************************************/
boost::asio::ip::tcp::socket& projecthttpconnection::socket( void )
{
  return this->tcpsocket;
}

/*******************************************************************************
Function: start
Purpose: Starts our read for http.
Updated: 22.01.2019
*******************************************************************************/
void projecthttpconnection::start( void )
{
  this->timer.expires_after( std::chrono::seconds( HTTPCLIENTDEFAULTTIMEOUT ) );
  this->timer.async_wait( boost::bind( &projecthttpconnection::handletimeout, 
                                        shared_from_this(), 
                                        boost::asio::placeholders::error ) );

  this->tcpsocket.async_read_some(
    boost::asio::buffer( this->inbuffer, this->inbuffer.size() - 1 ),
    boost::bind(&projecthttpconnection::handleread, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred) );
}

/*******************************************************************************
Function: handleread
Purpose: Wait for data then parses and acts. We must be able to handle multiple
writes to our socket.
Updated: 22.01.2019
*******************************************************************************/
void projecthttpconnection::handleread( 
    const boost::system::error_code& error, 
    std::size_t bytes_transferred )
{

  projectwebdocument response;

  try
  {
    if( !error )
    {

      this->request.append( this->inbuffer, bytes_transferred );
      if( !this->request.iscomplete() )
      {
        this->tcpsocket.async_read_some(
          boost::asio::buffer( this->inbuffer, this->inbuffer.size() - 1 ),
          boost::bind(&projecthttpconnection::handleread, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred) );
        return;
      }

      std::string path = this->request.getrequesturi().str();

      if( path.length() < 1 )
      {
        // 404?
        return;
      }

      path.erase( 0, 1 ); /* remove the leading / */
      stringvector pathparts = splitstring( path, '/' );
      

      if( "reg" == pathparts[ 0 ] )
      {
        projectsipregistration::httpget( pathparts, response );
      }
      else if( "dir" == pathparts[ 0 ] )
      {
        switch( this->request.getmethod() )
        {
          case projectwebdocument::GET:
          {
            
            projectsipdirdomain::httpget( pathparts, response );
            break;
          }
          case projectwebdocument::POST:
          {
            JSON::Value body = JSON::parse( *( request.getbody().strptr() ) );
            projectsipdirdomain::httppost( pathparts, body, response );
            break;
          }
        }
      }
      else if( "dialog" == pathparts[ 0 ] )
      {
        switch( this->request.getmethod() )
        {
          case projectwebdocument::GET:
          {
            projectsipdialog::httpget( pathparts, response );
            break;
          }
          case projectwebdocument::POST:
          {
            JSON::Value body = JSON::parse( *( request.getbody().strptr() ) );
            
            projectsipdialog::httppost( pathparts, body, response );
            break;
          }
        }
      }
      else
      {
        response.setstatusline( 404, "Not found" );
      }

      this->outbuffer = response.strptr();
    }

    boost::asio::async_write( this->tcpsocket,
      boost::asio::buffer( *this->outbuffer ),
      boost::bind(&projecthttpconnection::handlewrite, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred) );
  }
  catch(...)
  {
    response.setstatusline( 500, "Bad request" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    this->outbuffer = response.strptr();

    boost::asio::async_write( this->tcpsocket,
        boost::asio::buffer( *this->outbuffer ),
        boost::bind(&projecthttpconnection::handlewrite, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred) );
  }
}

/*******************************************************************************
Function: handlewrite
Purpose: Once we have finished...
Updated: 22.01.2019
*******************************************************************************/
void projecthttpconnection::handlewrite( 
    const boost::system::error_code& error, 
    std::size_t bytes_transferred )
{
  this->tcpsocket.close();
}

/*******************************************************************************
Function: handletimeout
Purpose: Our timer callback.
Updated: 29.01.2019
*******************************************************************************/
void projecthttpconnection::handletimeout( const boost::system::error_code& error )
{
  if ( error != boost::asio::error::operation_aborted )
  {
    this->tcpsocket.close();
  }
}


