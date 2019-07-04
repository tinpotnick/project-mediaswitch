
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

#include <atomic>
#include <thread>

#include "projecthttpserver.h"
#include "json.hpp"
#include "projectdaemon.h"

#include "projectrtpchannel.h"

#include <chrono>

#include "firfilter.h"

boost::asio::io_service ioservice;
boost::asio::io_service workerservice;

typedef boost::asio::basic_waitable_timer< std::chrono::high_resolution_clock > ourhighrestimer;
ourhighrestimer keepalive( workerservice );

typedef std::deque<projectrtpchannel::pointer> rtpchannels;
typedef std::unordered_map<std::string, projectrtpchannel::pointer> activertpchannels;
rtpchannels dormantchannels;
activertpchannels activechannels;

#warning TODO
// We probably need a config class
short port;
std::string publicaddress;
unsigned maxworker;
std::atomic_bool running;

/*!md
# handlewebrequest
As it says...
*/
static void handlewebrequest( projectwebdocument &request, projectwebdocument &response )
{
  std::string path = request.getrequesturi().str();

  if( 0 == path.length() )
  {
    response.setstatusline( 404, "Not found" );
    return;
  }

  path.erase( 0, 1 ); /* remove the leading / */
  stringvector pathparts = splitstring( path, '/' );

  int method = request.getmethod();

  if( "channel" == pathparts[ 0 ] )
  {
    if( 1 == pathparts.size() && projectwebdocument::POST == method )
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
      p->open();

      JSON::Object v;
      v[ "uuid" ] = u;
      v[ "port" ] = ( JSON::Integer ) p->getport();
      v[ "ip" ] = publicaddress;

      JSON::Object c;
      c[ "active" ] = ( JSON::Integer ) activechannels.size();
      c[ "available" ] = ( JSON::Integer ) dormantchannels.size();
      v[ "channels" ] = c;

      std::string t = JSON::to_string( v );

      response.setstatusline( 200, "Ok" );
      response.addheader( projectwebdocument::Content_Length, t.size() );
      response.addheader( projectwebdocument::Content_Type, "text/json" );
      response.setbody( t.c_str() );
      return;
    }
    else if( 2 == pathparts.size() && projectwebdocument::DELETE == method )
    {
      std::string channel = JSON::as_string( pathparts[ 1 ] );
      activertpchannels::iterator chan = activechannels.find( channel );
      if ( activechannels.end() != chan )
      {
        chan->second->close();
        activechannels.erase( chan );
        dormantchannels.push_back( chan->second );

        JSON::Object v;
        JSON::Object c;
        c[ "active" ] = ( JSON::Integer ) activechannels.size();
        c[ "available" ] = ( JSON::Integer ) dormantchannels.size();
        v[ "channels" ] = c;

        std::string t = JSON::to_string( v );

        response.setstatusline( 200, "Ok" );
        response.addheader( projectwebdocument::Content_Length, t.size() );
        response.addheader( projectwebdocument::Content_Type, "text/json" );
        response.setbody( t.c_str() );
        return;
      }
    }
    /* The second part to creating a channel is to send it somewhere - that is our target */
    else if( 2 == pathparts.size() &&
            projectwebdocument::PUT == method )
    {
      JSON::Object body = JSON::as_object( JSON::parse( *( request.getbody().strptr() ) ) );

      /* If we get here - we have /channel/target/<uuid> */
      activertpchannels::iterator chan = activechannels.find( pathparts[ 1 ] );
      if ( activechannels.end() != chan )
      {
        short port = JSON::as_int64( body[ "port" ] );
        std::string address = JSON::as_string( body[ "ip" ] );

        chan->second->target( address, port );

        if( body.has_key( "audio" ) )
        {
          JSON::Object audio = JSON::as_object( body[ "audio" ] );
          if( audio.has_key( "payloads" ) )
          {
            JSON::Array payloads = JSON::as_array( audio[ "payloads" ] );

            /* this is the CODECs we have been asked to send to the remote endpoint */
            projectrtpchannel::codeclist ourcodeclist;

            for( JSON::Array::values_t::iterator it = payloads.values.begin();
                  it != payloads.values.end();
                  it++ )
            {
              ourcodeclist.push_back( JSON::as_int64( *it ) );
            }

            chan->second->audio( ourcodeclist );
          }
        }

        response.setstatusline( 200, "Ok" );
        response.addheader( projectwebdocument::Content_Length, "0" );
        return;
      }
    }
    /* /PUT /channel/<uuid>/mix/<uuid> */
    else if( 4 == pathparts.size() &&
        projectwebdocument::PUT == method &&
        "mix" == pathparts[ 2 ] )
    {
      activertpchannels::iterator chan1 = activechannels.find( pathparts[ 1 ] );
      activertpchannels::iterator chan2 = activechannels.find( pathparts[ 3 ] );
      if ( activechannels.end() != chan1 && activechannels.end() != chan2 )
      {
        if( chan1->second->mix( chan2->second ) )
        {
          response.setstatusline( 200, "Ok" );
          response.addheader( projectwebdocument::Content_Length, "0" );
          return;
        }
      }
    }
  }

  response.setstatusline( 404, "Not found" );
}

/*!md
## ontimer
Dummy timer to keep our worker service running. I can't see any other way to stop run from returning.
*/
void ontimer(const boost::system::error_code& /*e*/)
{
  keepalive.expires_at( keepalive.expiry() + std::chrono::seconds( 100 ) );
  keepalive.async_wait( &ontimer );
}


/*!md
# stopserver
Actually do the stopping
*/
static void stopserver( void )
{
  running = false;
  workerservice.stop();
  ioservice.stop();
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
# startserver
Start our server and kick off all of the worker threads.

We have a main ioservice which handles (as it says) all of the I/O. HTTP and RTP traffic is received in this thread. Further worker threads are created and should be tied to a CPU so we can spread the workload of heavy CPU load items (such as transcoding).

If we have 1 CPU (std::thread::hardware_concurrency) then we create 1 worker thread as we cannot tie up the main I/O ioservice. If we have 2, then we assign our main thread to the I/O and a created thread to workload. If we have enough cores then we assign each thread its own CPU. We may want to assess what happens with ounly 2 cores - we don't want 1 idle with 1 heavily transcoding - so we may wish to balance that somehow in the future.

To create a worker thread item:

workerservice->post( boost::bind( transcodeg722, shared_from_this() ) );

*/
void startserver( short port )
{
  running = true;
  unsigned numcpus = std::thread::hardware_concurrency();

  if( numcpus > 1 )
  {
    numcpus--;
  }

  if( 0 != maxworker && numcpus > maxworker )
  {
    numcpus = maxworker;
  }

  std::cout << "Starting " << numcpus << " worker threads" << std::endl;

  // A mutex ensures orderly access to std::cout from multiple threads.
  std::mutex iomutex;
  std::vector< std::thread > threads( numcpus );

  keepalive.expires_at( std::chrono::system_clock::now() );
  keepalive.async_wait( &ontimer );

  try
  {
    cpu_set_t cpuset;
    CPU_ZERO( &cpuset );
    CPU_SET( 0, &cpuset );

    if ( pthread_setaffinity_np( pthread_self(), sizeof( cpu_set_t ), &cpuset ) )
    {
      std::cerr << "Error tyng thread to CPU " << 0 << std::endl;
    }

    for ( unsigned i = 0; i < numcpus; i++ )
    {
      threads[ i ] = std::thread( []()
      {
        while( running )
        {
          try
          {
            workerservice.run();
          }
          catch( std::exception& e )
          {
            std::cerr << e.what() << std::endl;
          }
          catch( ... )
          {
            std::cerr << "Unhandled exception in worker bees - rentering workerservice" << std::endl;
          }
        }
      } );

      CPU_ZERO( &cpuset );
      CPU_SET( ( i + 1 ) % numcpus, &cpuset );

      if ( pthread_setaffinity_np( threads[ i ].native_handle() , sizeof( cpu_set_t ), &cpuset ) )
      {
        std::cerr << "Error tyng thread to CPU " << 0 << std::endl;
      }
    }

    projecthttpserver h( ioservice, port, std::bind( &handlewebrequest, std::placeholders::_1, std::placeholders::_2 ) );

    while( running )
    {
      try
      {
        ioservice.run();
      }
      catch( std::exception& e )
      {
        std::cerr << e.what() << std::endl;
      }
      catch( ... )
      {
        std::cerr << "Unhandled exception - rentering ioservice" << std::endl;
      }
    }

    for ( auto& t : threads )
    {
      t.join();
    }
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
void initchannels( unsigned short startport, unsigned short endport )
{
  int i;

  try
  {
    // Test we can open them all if needed and warn if necessary.
    for( i = startport; i < endport; i += 2 )
    {
      projectrtpchannel::pointer p = projectrtpchannel::create( ioservice, i );
      p->open();
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
# testatomic
Test an atomic variable to ensure it is lock free. Issue a warning if it is not as we rely on atomic variables for performance with threads.
*/
void testatomic( void )
{
  std::atomic_bool test;

  if( !test.is_always_lock_free )
  {
    std::cerr << "Warning: atomic variables appear to be not atomic so we will be using locks which will impact performance" << std::endl;
  }
}

/*!md
# main
As it says...
*/
int main( int argc, const char* argv[] )
{
  port = 9002;
  publicaddress = "127.0.0.1";
  maxworker = 0;

  unsigned short startrtpport = 10000;
  unsigned short endrtpport = 20000;

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
      else if( "--pa" == argvstr ) /* [P]ublic RTP [A]ddress */
      {
        try
        {
          if( argc > ( i + 1 ) )
          {
            publicaddress = argv[ i + 1 ];
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
      else if( "--maxworker" == argvstr )
      {
        try
        {
          if( argc > ( i + 1 ) )
          {
            maxworker = boost::lexical_cast< int >( argv[ i + 1 ] );
            i++;
            continue;
          }
        }
        catch( boost::bad_lexical_cast &e )
        {
        }
        std::cerr << "I need a maxworker count" << std::endl;
        return -1;
      }
      else if( "--testfir" == argvstr )
      {
        int frequency = boost::lexical_cast< int >( argv[ i + 1 ] );
        testlofir( frequency );
        return 0;
      }
    }
  }

  // Register our CTRL-C handler
  signal( SIGINT, killserver );
  std::cout << "Starting Project RTP server with control port listening on port " << port << std::endl;
  std::cout << "RTP published address is " << publicaddress << std::endl;
  std::cout << "RTP ports "  << startrtpport << " => " << endrtpport << ": " << (int) ( ( endrtpport - startrtpport ) / 2 ) << " channels" << std::endl;
  if( 0 != maxworker )
  {
    std::cout << "Max worker threads " << maxworker << std::endl;
  }

  testatomic();
  initchannels( startrtpport, endrtpport );

  if ( !fg )
  {
    daemonize();
  }

  std::cout << "Started RTP server, waiting for requests." << std::endl;

  startserver( port );

  return 0;
}
