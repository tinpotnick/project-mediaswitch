
#include <iostream>
#include <stdio.h>
#include <signal.h>

#include <sys/resource.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/lexical_cast.hpp>
#include <deque>
#include <unordered_map>

#include "projecthttpserver.h"
#include "json.hpp"
#include "projectdaemon.h"

#include "projectrtpchannel.h"


boost::asio::io_service io_service;

typedef std::deque<projectrtpchannel::pointer> rtpchannels;
typedef std::unordered_map<std::string, projectrtpchannel::pointer> activertpchannels;
rtpchannels dormantchannels;
activertpchannels activechannels;

/*!md
# stopserver
Actually do the stopping
*/
static void stopserver( void )
{
  io_service.stop();
}

/*!md
# killServer
As it says...
*/
static void killserver( int signum )
{
  std::cout << "OUCH" << std::endl;
  stopserver();
}

/*!md
# handlewebrequest
As it says...
*/
static void handlewebrequest( projectwebdocument &request, projectwebdocument &response )
{
  std::string path = request.getrequesturi().str();

  if( path.length() < 1 )
  {
    response.setstatusline( 404, "Not found" );
    return;
  }

  path.erase( 0, 1 ); /* remove the leading / */
  stringvector pathparts = splitstring( path, '/' );


  if( projectwebdocument::POST == request.getmethod() )
  {
    JSON::Object body = JSON::as_object( JSON::parse( *( request.getbody().strptr() ) ) );
    if( !body.has_key( "channel" ) )
    {
      if( dormantchannels.size() == 0 )
      {
        response.setstatusline( 503, "Currently out of channels" );
        response.addheader( projectwebdocument::Content_Length, 0 );
        response.addheader( projectwebdocument::Content_Type, "text/json" );
        return;
      }

      std::string u = uuid();

      projectrtpchannel::pointer p = *dormantchannels.begin();
      dormantchannels.pop_front();
      activechannels[ u ] = p;
      p->open( projectrtpchannel::PCMA );

      JSON::Object v;
      v[ "channel" ] = u;
      v[ "port" ] = ( JSON::Integer ) p->getport();
      v[ "ip" ] = "127.0.0.1";
      std::string t = JSON::to_string( v );

      response.setstatusline( 200, "Ok" );
      response.addheader( projectwebdocument::Content_Length, t.size() );
      response.addheader( projectwebdocument::Content_Type, "text/json" );
      response.setbody( t.c_str() );
    }
  }
  else
  {
    response.setstatusline( 404, "Not found" );
  }
}

/*!md
# startserver
As it says...
*/
void startserver( short port )
{
  try
  {
    projecthttpserver h( io_service, port, std::bind( &handlewebrequest, std::placeholders::_1, std::placeholders::_2 ) );
  io_service.run();
  }
  catch( std::exception& e )
  {
  std::cerr << e.what() << std::endl;
  }

  // Clean up
  std::cout << "Cleaning up" << std::endl;
  return;
}


/*!md
# initchannels
Create our channel objects and pre allocate any memory.
*/
void initchannels( short startport, short endport )
{
  int i;

  try
  {
    // Test we can open them all if needed and warn if necessary.
    for( i = startport; i < endport; i += 2 )
    {
      projectrtpchannel::pointer p = projectrtpchannel::create( io_service, i );
      p->open( projectrtpchannel::PCMA );
      dormantchannels.push_back( p );
    }
  }
  catch(...)
  {
    std::cerr << "I could only open " << dormantchannels.size() << " channels, you might want to review your OS settings." << std::endl;
  }

  // Now close.
  rtpchannels::iterator it;
  for( it = dormantchannels.begin(); it != dormantchannels.end(); it++ )
  {
    (*it)->close();
  }
}

/*!md
# main
As it says...
*/
int main( int argc, const char* argv[] )
{
  short port = 9002;

  short startrtpport = 10000;
  short endrtpport = 20000;

  srand( time( NULL ) );

  bool fg = false;

  for ( int i = 1; i < argc ; i++ )
  {
    if ( argv[i] != NULL )
    {
      std::string argvstr = argv[ i ];

      if ( "--fg" == argvstr)
      {
        fg = true;
      }
      else if( "--port" == argvstr )
      {
        try
        {
          if( argc > ( i + 1 ) )
          {
            port = boost::lexical_cast< int >( argv[ i + 1 ] );
            i++;
            continue;
          }
        }
        catch( boost::bad_lexical_cast &e )
        {
        }
        std::cerr << "What port was that?" << std::endl;
        return -1;
      }
    }
  }

  // Register our CTRL-C handler
  signal( SIGINT, killserver );
  std::cout << "Starting Project RTP server with control port listening on port " << port << std::endl;
  std::cout << "RTP ports "  << startrtpport << " => " << endrtpport << ": " << (int) ( ( endrtpport - startrtpport ) / 2 ) << " channels" << std::endl;

  initchannels( startrtpport, endrtpport );

  if ( !fg )
  {
    daemonize();
  }

  std::cout << "Started RTP server, waiting for requests." << std::endl;

  startserver( port );

  return 0;
}
