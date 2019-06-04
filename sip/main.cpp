
#include <iostream>
#include <stdio.h>
#include <signal.h>

#include <sys/resource.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "projectsipserver.h"
#include "projecthttpserver.h"

#include "projectsipregistrar.h"
#include "projectsipdialog.h"
#include "projectsipdirectory.h"
#include "json.hpp"
#include "projectdaemon.h"
#include "projectsipconfig.h"

#include "test.h"

boost::asio::io_service io_service;
projectsipserver::pointer sipserver;

static void stopserver( void )
{
  io_service.stop();
}

static void killserver( int signum )
{
  std::cout << "OUCH" << std::endl;
  stopserver();
}

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


  if( "reg" == pathparts[ 0 ] )
  {
    projectsipregistration::httpget( pathparts, response );
  }
  else if( "dir" == pathparts[ 0 ] )
  {
    switch( request.getmethod() )
    {
      case projectwebdocument::GET:
      {

        projectsipdirdomain::httpget( pathparts, response );
        break;
      }
      case projectwebdocument::PUT:
      {
        JSON::Value body = JSON::parse( *( request.getbody().strptr() ) );
        projectsipdirdomain::httpput( pathparts, body, response );
        break;
      }
      case projectwebdocument::PATCH:
      {
        JSON::Value body = JSON::parse( *( request.getbody().strptr() ) );
        projectsipdirdomain::httppatch( pathparts, body, response );
        break;
      }
      case projectwebdocument::DELETE:
      {
        projectsipdirdomain::httpdelete( pathparts, response );
        break;
      }
    }
  }
  else if( "dialog" == pathparts[ 0 ] )
  {
    switch( request.getmethod() )
    {
      case projectwebdocument::GET:
      {
        projectsipdialog::httpget( pathparts, response );
        break;
      }
      case projectwebdocument::PUT:
      {
        JSON::Value body = JSON::parse( *( request.getbody().strptr() ) );

        projectsipdialog::httpput( pathparts, body, response );
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
}

void startserver( short controlport, short sipport )
{
  try
  {
    sipserver = projectsipserver::create( io_service, sipport );
    projecthttpserver h( io_service, controlport, std::bind( &handlewebrequest, std::placeholders::_1, std::placeholders::_2 ) );
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


int main( int argc, const char* argv[] )
{
  short port = 9000;
  short sipport = 5060;
  srand( time( NULL ) );

  bool fg = false;
  bool tests = false;

  for ( int i = 1; i < argc ; i++ )
  {
    if ( argv[i] != NULL )
    {
      std::string argvstr = argv[i];

      if ( "--help" == argvstr )
      {
        std::cout << "--fg - run the server in the foreground" << std::endl;
        std::cout << "--cp - set the control server listening port (default 9000)" << std::endl;
        std::cout << "--sp - set the sip server listening port (default 5060)" << std::endl;
        std::cout << "--pa - set the sip server listening ip address (default 127.0.0.1)" << std::endl;
        return 1;
      }
      else if ( "--fg" == argvstr )
      {
        fg = true;
      }
      else if ( "--test" == argvstr )
      {
        tests = true;
      }
      else if( "--cp" == argvstr )
      {
        try
        {
          if( argc > ( i + 1 ) )
          {
            port = boost::lexical_cast< short >( argv[ i + 1 ] );
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
      else if( "--sp" == argvstr )
      {
        try
        {
          if( argc > ( i + 1 ) )
          {
            sipport = boost::lexical_cast< short >( argv[ i + 1 ] );
            projectsipconfig::setsipport( sipport );
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
      else if( "--pa" == argvstr ) /* [P]ublic SIP [A]ddress */
      {
        try
        {
          if( argc > ( i + 1 ) )
          {
            projectsipconfig::setsiphostip( argv[ i + 1 ] );
            i++;
            continue;
          }
        }
        catch( boost::bad_lexical_cast &e )
        {
        }
        std::cerr << "What address was that?" << std::endl;
        return -1;
      }
    }
  }

  // Register our CTRL-C handler
  signal( SIGINT, killserver );

  struct rlimit core_limits;
  core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
  setrlimit( RLIMIT_CORE, &core_limits );

  if( tests )
  {
    runtests();
    return 0;
  }

  std::cout << "Starting Project SIP server." << std::endl;
  std::cout << "Control port listening on port " << port << std::endl;
  std::cout << "SIP listening on port " << sipport << std::endl;
  std::cout << "SIP Contact IP used is " << projectsipconfig::gethostipsipport() << std::endl;

  if ( !fg )
  {
    daemonize();
  }

  startserver( port, sipport );

  return 0;
}
