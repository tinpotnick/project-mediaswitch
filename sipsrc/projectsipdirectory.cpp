


#include "projectsipdirectory.h"
#include "projecthttpclient.h"

static projectsipdirdomains dirdoms;


/*******************************************************************************
Function: projectsipdirectory
Purpose: Constructor.
Updated: 15.01.2019
*******************************************************************************/
projectsipdirectory::projectsipdirectory( void )
{
  /* Example data */
  projectsipdirdomain::pointer d = projectsipdirdomain::create();
  d->domain = stringptr( new std::string( "com" ) );

  projectsipdirdomain::pointer s = projectsipdirdomain::create();
  s->domain = stringptr( new std::string( "babblevoice" ) );
  d->subdomains.emplace( *( s->domain ), s );

  s = projectsipdirdomain::create();
  s->domain = stringptr( new std::string( "bling" ) );
  d->subdomains.emplace( *( s->domain ), s );

  projectsipdiruser::pointer t;
  t->username = stringptr( new std::string( "1000" ) );
  t->secret = stringptr( new std::string( "1123654789" ) );
  s->users.emplace( *( t->username ), t );

  dirdoms.emplace( *( d->domain ), d );
}

/*******************************************************************************
Function: lookup
Purpose: Looks up the users password from our directory. The call is 
asynchronous as we may have to look elsewhere for our answer. We will have to
introduce controls which prevent a rogue client just increasing our memory
usage by querying random domain names.
Updated: 15.01.2019
*******************************************************************************/
void projectsipdirectory::lookup( substring domain, substring user, std::function<void ( stringptr ) > callback )
{
  projectsipdirdomain::map::iterator it = dirdoms.find( domain.str() );

  if( dirdoms.end() != it )
  {
    projectsipdirusers::iterator uit;
    uit = it->second->users.find( user.str() );

    if( it->second->users.end() == uit )
    {
      // Does not exist.
      callback( stringptr() );
      return;
    }

    callback( uit->second->secret );
    return;
  }
#if 0
  projecthttpclient::callback 
  projecthttpclient::create()->asyncrequest();

  stringptr result( new std::string( "1123654789" ) );
  callback( result );
#endif
}