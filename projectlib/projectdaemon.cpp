

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>


/*******************************************************************************
Function: daemonize
Purpose: As it says...
Updated: 12.12.2018
*******************************************************************************/
void daemonize( void )
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



