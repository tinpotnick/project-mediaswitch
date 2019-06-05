


#include "projectsipdirectory.h"
#include "projecthttpclient.h"
#include "json.hpp"

//static projectsipdirdomain dir;
static projectsipdirdomain::pointer dir = projectsipdirdomain::create();

/*!md
## projectsipdiruser
Constructor

*/
projectsipdiruser::projectsipdiruser( void )
{
}

/*!md
## projectsipdirdomain::create
Create auto pointer.

*/
projectsipdiruser::pointer projectsipdiruser::create( void )
{
  return pointer( new projectsipdiruser() );
}

/*!md
## projectsipdirdomain
Constructor

*/
projectsipdirdomain::projectsipdirdomain( void )
{

}

/*!md
## projectsipdirdomain::create
Create auto pointer.

*/
projectsipdirdomain::pointer projectsipdirdomain::create( void )
{
  return pointer( new projectsipdirdomain() );
}

/*!md
## adduser
Add a user to this domain. If the user exists then replace.

*/
void projectsipdirdomain::adduser( std::string &user, std::string &secret )
{
  projectsipdirusers::iterator it;
  it = this->users.find( user );
  if( this->users.end() != it )
  {
    this->users.erase( it );
  }

  projectsipdiruser::pointer u = projectsipdiruser::create();

  u->username = stringptr( new std::string( user ) );
  u->secret = stringptr( new std::string( secret ) );

  this->users.emplace( user, u );
}


/*!md
## removeuser
Remove a user from a domain.

*/
void projectsipdirdomain::removeuser( std::string &user )
{
  projectsipdirusers::iterator it;
  it = this->users.find( user );
  if( this->users.end() != it )
  {
    this->users.erase( it );
  }
}

/*!md
## lookuppassword
Looks up the users password from our directory. Our data must be
populated via the http server. This also then limits the look up on rogue
clients trying to randomly lookup username/password combos. This may create a
slower load on startup and greater memory requirement. But seems to be the best
option.

*/
stringptr projectsipdirdomain::lookuppassword( substring domain, substring user )
{
  projectsipdirdomain::pointer ref = projectsipdirdomain::lookupdomain( domain );

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

/*!md
## lookuppassword
As above but can pass in string references.

*/
stringptr projectsipdirdomain::lookuppassword( std::string &domain, std::string &user )
{
  projectsipdirdomain::pointer ref = projectsipdirdomain::lookupdomain( domain );

  if( ref )
  {
    projectsipdirusers::iterator uit;
    uit = ref->users.find( user );

    if( ref->users.end() == uit )
    {
      // Does not exist.
      return stringptr();
    }

    return uit->second->secret;
  }

  return stringptr();
}

/*!md
## userexist
Looks up a user in a domain to see if they exist.

*/
bool projectsipdirdomain::userexist( std::string &domain, std::string &user )
{
  projectsipdirdomain::pointer ref = projectsipdirdomain::lookupdomain( domain );

  if( ref )
  {
    projectsipdirusers::iterator uit;
    uit = ref->users.find( user );

    if( ref->users.end() == uit )
    {
      // Does not exist.
      return false;
    }

    return true;
  }

  return false;
}


/*!md
# userexist
Does the user exist in this domain.
*/
bool projectsipdirdomain::userexist( std::string &user )
{
  projectsipdirusers::iterator uit;
  uit = this->users.find( user );

  if( this->users.end() == uit )
  {
    /* Does not exist. */
    return false;
  }

  return true;
}


/*!md
## lookupdomain
Returns a domain object from our directory (assuming we have one).

*/
projectsipdirdomain::pointer projectsipdirdomain::lookupdomain( substring domain )
{
  projectsipdirdomain::pointer ref;

  std::string dstr = domain.str();
  stringvector l = splitstring( dstr, '.' );
  if( l.size() > 5 )
  {
    return ref;
  }

  projectsipdirdomain::map *dirref = &dir->subdomains;
  projectsipdirdomain::map::iterator it = dirref->end();
  for( int i = l.size(); i != 0; i-- )
  {
    it = dirref->find( l[ i - 1 ] );
    if( dirref->end() == it )
    {
      return projectsipdirdomain::pointer();
    }
    ref = it->second;
    dirref = &it->second->subdomains;
  }

  return ref;
}

/*!md
## lookupdomain
Returns a domain object from our directory (assuming we have one).

*/
projectsipdirdomain::pointer projectsipdirdomain::lookupdomain( std::string &dstr )
{
  projectsipdirdomain::pointer ref;
  stringvector l = splitstring( dstr, '.' );
  if( l.size() > 5 )
  {
    return ref;
  }

  projectsipdirdomain::map *dirref = &dir->subdomains;
  projectsipdirdomain::map::iterator it = dirref->end();
  for( int i = l.size(); i != 0; i-- )
  {
    it = dirref->find( l[ i - 1 ] );
    if( dirref->end() == it )
    {
      return projectsipdirdomain::pointer();
    }
    ref = it->second;
    dirref = &it->second->subdomains;
  }

  return ref;
}

/*!md
## adddomain
Returns a domain object from our directory (assuming we have one).

*/
projectsipdirdomain::pointer projectsipdirdomain::adddomain( std::string &domain )
{
  projectsipdirdomain::pointer retval;

  stringvector parts = splitstring( domain, '.' );

  map *domptr = &dir->subdomains;

  while( 0 != parts.size() )
  {
    map::iterator it = domptr->find( parts[ parts.size() - 1 ] );
    if( domptr->end() == it )
    {
      retval = projectsipdirdomain::create();
      domptr->emplace( parts[ parts.size() - 1 ], retval );
      domptr = &retval->subdomains;
    }
    else
    {
      domptr = &( it->second->subdomains );
      retval = it->second;
    }

    parts.pop_back();
  }

  return retval;

}

/*!md
## removedomain
Removes a domain. Will tidy up the tree of domains if they are unused.
com ---> babblevoice ---> bling
                     ---> blong
removedomain( bling.babblevoice.com ) will remove only bling

if
com ---> babblevoice ---> bling
         yadaya      ---> bling
removedomain( bling.babblevoice.com ) will remove babblevoice.bling i.e. you are
left with
com ---> yadaya      ---> bling

It ill remove the specified domain regardless of it having children, however
parent domains will be tidied up (removed) if they are empty of other domains
and users.

*/
bool projectsipdirdomain::removedomain( std::string &domain, bool force )
{
  std::string::size_type i = domain.find( '.' );
  std::string hostdom, domdom;
  projectsipdirdomain::pointer ptr;

  if ( i != std::string::npos )
  {
    hostdom = domain.substr( 0, i );
    domdom = domain.substr( i + 1, domain.size() );
    ptr = projectsipdirdomain::lookupdomain( domdom );

    if( !ptr )
    {
      return false;
    }
    map::iterator domit = ptr->subdomains.find( hostdom );
    if( ptr->subdomains.end() != domit )
    {
      if( force ||
          ( 0 == domit->second->users.size() && 0 == domit->second->subdomains.size() )
        )
      {
        ptr->subdomains.erase( domit );
        projectsipdirdomain::removedomain( domdom, false );
        return true;
      }
    }
  }
  else
  {
    map::iterator domit = dir->subdomains.find( domain );
    if( dir->subdomains.end() != domit )
    {
      if( force ||
        ( 0 == domit->second->users.size() || 0 == domit->second->subdomains.size() ) )
      {
        dir->subdomains.erase( domit );
        return true;
      }
    }
  }

  return false;
}


/*!md
## httpget
Report on directory information

*/
void projectsipdirdomain::httpget( stringvector &path, projectwebdocument &response )
{
  projectsipdirdomain::pointer ptr;

  if( path.size() > 1 )
  {
    ptr = lookupdomain( path[ 1 ] );
  }
  else
  {
    ptr = dir;
  }

  if( ptr )
  {
    JSON::Object v;
    JSON::Array a;

    projectsipdirusers::iterator userit;
    for( userit = ptr->users.begin(); userit != ptr->users.end(); userit++ )
    {
      a.values.push_back( *userit->second->username );
    }

    if( a.values.size() > 0 )
    {
      v[ "users" ] = a;
    }

    projectsipdirdomain::map::iterator domit;
    JSON::Array d;
    for( domit = ptr->subdomains.begin(); domit != ptr->subdomains.end(); domit++ )
    {
      std::string domain = domit->first;
      d.values.push_back( domain );
    }

    if( d.values.size() > 0 )
    {
      v[ "domains" ] = d;
    }

    if( ptr->controlhost->size() > 0 )
    {
      v[ "control" ] = JSON::as_string( *ptr->controlhost );;
    }

    std::string t = JSON::to_string( v );

    response.setstatusline( 200, "Ok" );
    response.addheader( projectwebdocument::Content_Length, t.length() );
    response.addheader( projectwebdocument::Content_Type, "text/json" );
    response.setbody( t.c_str() );
    return;
  }

  response.setstatusline( 404, "Not found" );
}

/*!md
## httpput
Respods to put requests form our control server.
It needs to handle the following array:
HTTP PUT /dir/bling.babblevoice.com
{
  "control": "http://127.0.0.1:9000",
  "users":
  [
    { "username": "1003", "secret": "mysecret"}
  ]
}
OR
HTTP PUT /dir/bling.babblevoice.com/1003
{ "secret": "mysecret"}


*/
void projectsipdirdomain::httpput( stringvector &path, JSON::Value &body, projectwebdocument &response )
{
  if( path.size() <= 1 )
  {
    response.setstatusline( 400, "Path too short" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }

  if( 3 == path.size() )
  {
    projectsipdirdomain::httppatch( path, body, response );
    return;
  }

  JSON::Object d = JSON::as_object( body);

  projectsipdirdomain::pointer domentry = projectsipdirdomain::adddomain( path[ 1 ] );

  domentry->controlhost = stringptr( new std::string( JSON::as_string( d[ "control" ] ) ) );

  domentry->users.clear();

  JSON::Array userarray = JSON::as_array( d[ "users" ] );
  JSON::Array::values_t::iterator userit;
  for( userit = userarray.values.begin();
        userit != userarray.values.end();
        userit++ )
  {
    JSON::Object userobj = JSON::as_object( *userit );
    if( !userobj.has_key( "username" ) )
    {
      continue;
    }

    if( !userobj.has_key( "secret" ) )
    {
      continue;
    }

    std::string user = JSON::as_string( userobj[ "username" ] );
    std::string secret = JSON::as_string( userobj[ "secret" ] );

    domentry->adduser( user, secret );
  }

  response.setstatusline( 201, "Created" );
  response.addheader( projectwebdocument::Content_Length, 0 );
}

/*!md
## httppatch
Responds to patch requests form our control server. The domain must
already exist.
It needs to handle the following array:
HTTP PUT /dir/bling.babblevoice.com
{
  "control": "http://127.0.0.1:9001".
  "users":
  [
    { "username": "1003", "secret": "mysecret"}
  ]
}

OR

HTTP PUT /dir/bling.babblevoice.com/1003
{ "secret": "mysecret"}

The document can be the same as put, but we will update the entry without removing
some items.

*/
void projectsipdirdomain::httppatch( stringvector &path, JSON::Value &body, projectwebdocument &response )
{

  if( 2 != path.size() || 3 != path.size() )
  {
    response.setstatusline( 400, "Path wrong length" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }

  projectsipdirdomain::pointer domentry = projectsipdirdomain::lookupdomain( path[ 1 ] );

  if( !domentry )
  {
    response.setstatusline( 404, "Domain not found" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }

  if( 3 == path.size() )
  {
    JSON::Object u = JSON::as_object( body );

    if( !u.has_key( "secret" ) )
    {
      response.setstatusline( 400, "Secret required" );
      response.addheader( projectwebdocument::Content_Length, 0 );
      return;
    }

    domentry->adduser( path[ 2 ], JSON::as_string( u[ "secret" ] ) );

    response.setstatusline( 200, "Updated" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }
  else if( 2 == path.size() )
  {
    JSON::Object d = JSON::as_object( body );
    if( d.has_key( "control" ) )
    {
      domentry->controlhost = stringptr( new std::string( JSON::as_string( d[ "control" ] ) ) );
    }

    JSON::Array userarray = JSON::as_array( d[ "users" ] );
    JSON::Array::values_t::iterator userit;
    for( userit = userarray.values.begin();
          userit != userarray.values.end();
          userit++ )
    {
      JSON::Object userobj = JSON::as_object( *userit );
      if( !userobj.has_key( "username" ) )
      {
        continue;
      }

      if( !userobj.has_key( "secret" ) )
      {
        continue;
      }

      std::string user = JSON::as_string( userobj[ "username" ] );
      std::string secret = JSON::as_string( userobj[ "secret" ] );

      domentry->adduser( user, secret );
    }

    response.setstatusline( 200, "Updated" );
    response.addheader( projectwebdocument::Content_Length, 0 );
  }
}

/*!md
## httpdelete
Report on directory information

*/
void projectsipdirdomain::httpdelete( stringvector &path, projectwebdocument &response )
{
  if( 3 == path.size() )
  {
    projectsipdirdomain::pointer ptr;
    ptr = lookupdomain( path[ 1 ] );
    if( !ptr )
    {
      response.setstatusline( 404, "No such domain" );
      response.addheader( projectwebdocument::Content_Length, 0 );
      return;
    }
    ptr->removeuser( path[ 2 ] );

    response.setstatusline( 200, "Removed" );
    response.addheader( projectwebdocument::Content_Length, 0 );
    return;
  }

  if( 2 == path.size() )
  {
    if( projectsipdirdomain::removedomain( path[ 1 ] ) )
    {
      response.setstatusline( 200, "Removed" );
      response.addheader( projectwebdocument::Content_Length, 0 );
      return;
    }
  }

  response.setstatusline( 400, "Bad path" );
  response.addheader( projectwebdocument::Content_Length, 0 );
  return;

}
