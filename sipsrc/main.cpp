
#include <iostream>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <sys/resource.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "projectsipserver.h"
#include "projecthttpserver.h"

#include "test.h"

boost::asio::io_service io_service;

/*******************************************************************************
Function: daemonize
Purpose: As it says...
Updated: 12.12.2018
*******************************************************************************/
static void daemonize( void )
{
  pid_t pid, sid;

  /* already a daemon */
  if ( getppid() == 1 )
  {
    return;
  }

  /* Fork off the parent process */
  pid = fork();
  if (pid < 0)
  {
    exit( EXIT_FAILURE );
  }
  /* If we got a good PID, then we can exit the parent process. */
  if ( pid > 0 )
  {
    exit( EXIT_SUCCESS );
  }

  /* At this point we are executing as the child process */

  /* Change the file mode mask */
  umask( 0 );

  /* Create a new SID for the child process */
  sid = setsid();
  if ( sid < 0 )
  {
    exit( EXIT_FAILURE );
  }

  /* Change the current working directory.  This prevents the current
     directory from being locked; hence not being able to remove it. */
  if ( ( chdir( "/" ) ) < 0 )
  {
    exit( EXIT_FAILURE );
  }

  /* Redirect standard files to /dev/null */
  freopen( "/dev/null", "r", stdin );
  freopen( "/dev/null", "w", stdout );
  freopen( "/dev/null", "w", stderr );
}

/*******************************************************************************
Function: stopserver
Purpose: Actually do the stopping
Updated: 12.12.2018
*******************************************************************************/
static void stopserver( void )
{
	io_service.stop();
}

/*******************************************************************************
Function: killServer
Purpose: As it says...
Updated: 12.12.2018
*******************************************************************************/
static void killserver( int signum )
{
	std::cout << "OUCH" << std::endl;
	stopserver();
}

/*******************************************************************************
Function: startserver
Purpose: As it says...
Updated: 12.12.2018
*******************************************************************************/
void startserver( void )
{
	try
	{
    projectsipserver s( io_service, 5060 );
    projecthttpserver h( io_service, 8080 );
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

/*******************************************************************************
Function: main
Purpose: As it says...
Updated: 12.12.2018
*******************************************************************************/
int main( int argc, const char* argv[] )
{
  srand( time( NULL ) );

	bool fg = false;
  bool tests = false;

	for ( int i = 1; i < argc ; i++ )
	{
		if ( argv[i] != NULL )
		{
			std::string argvstr = argv[i];

			if ( "--fg" == argvstr)
			{
				fg = true;
			}
      else if ( "--test" == argvstr)
			{
				tests = true;
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

	if ( !fg )
	{
		daemonize();
	}

	startserver();

	return 0;
}
