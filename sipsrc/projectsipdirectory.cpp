


#include "projectsipdirectory.h"
#include "projecthttpclient.h"

static projectsipdirectory dir;


/*******************************************************************************
Function: projectsipdiruser
Purpose: Constructor
Updated: 22.01.2019
*******************************************************************************/
projectsipdiruser::projectsipdiruser( void )
{
}

/*******************************************************************************
Function: projectsipdirdomain::create
Purpose: Create auto pointer.
Updated: 22.01.2019
*******************************************************************************/
projectsipdiruser::pointer projectsipdiruser::create( void )
{
  return pointer( new projectsipdiruser() );
}

/*******************************************************************************
Function: projectsipdirdomain
Purpose: Constructor
Updated: 22.01.2019
*******************************************************************************/
projectsipdirdomain::projectsipdirdomain( void )
{

}

/*******************************************************************************
Function: projectsipdirdomain::create
Purpose: Create auto pointer.
Updated: 22.01.2019
*******************************************************************************/
projectsipdirdomain::pointer projectsipdirdomain::create( void )
{
  return pointer( new projectsipdirdomain() );
}

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

  projectsipdirdomain::pointer f = projectsipdirdomain::create();
  f->domain = stringptr( new std::string( "bling" ) );
  s->subdomains.emplace( *( f->domain ), f );

  projectsipdiruser::pointer t = projectsipdiruser::create();;
  t->username = stringptr( new std::string( "1003" ) );
  t->secret = stringptr( new std::string( "1123654789" ) );
  f->users.emplace( *( t->username ), t );

  dir.domains.emplace( *( d->domain ), d );
}

/*******************************************************************************
Function: lookuppassword
Purpose: Looks up the users password from our directory. Our data must be 
populated via the http server. This also then limits the look up on rogue
clients trying to randomly lookup username/password combos. This may create a 
slower load on startup and greater memory requirement. But seems to be the best
option.
Updated: 22.01.2019
*******************************************************************************/
stringptr projectsipdirectory::lookuppassword( substring domain, substring user )
{
  projectsipdirdomain::pointer ref = projectsipdirectory::lookupdomain( domain );

  if( ref )
  {
    projectsipdirusers::iterator uit;
    std::string userstr = user.str();
    uit = ref->users.find( userstr );

    if( ref->users.end() == uit )
    {
      // Does not exist.
      return stringptr();
    }

    return uit->second->secret;
  }

  return stringptr();
}


/*******************************************************************************
Function: lookupdomain
Purpose: Returns a domain object from our directory (assuming we have one).
Updated: 22.01.2019
*******************************************************************************/
projectsipdirdomain::pointer projectsipdirectory::lookupdomain( substring domain )
{
  projectsipdirdomain::pointer ref;

  std::string dstr = domain.str();
  stringvector l = splitstring( dstr, '.' );
  if( l.size() > 5 )
  {
    return ref;
  }

  projectsipdirdomain::map *dirref = &dir.domains;
  projectsipdirdomain::map::iterator it = dirref->end();
  for( int i = l.size(); i != 0; i-- )
  {
    it = dirref->find( l[ i - 1 ] );
    if( dirref->end() == it )
    {
      return ref;
    }
    ref = it->second;
    dirref = &it->second->subdomains;
  }

  return ref;
}


