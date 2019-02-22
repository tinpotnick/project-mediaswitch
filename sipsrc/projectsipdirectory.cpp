


#include "projectsipdirectory.h"
#include "projecthttpclient.h"
#include "json.hpp"

static projectsipdirdomain dir;

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
Function: adduser
Purpose: Add a user to this domain. If hte user exists then replace.
Updated: 20.02.2019
*******************************************************************************/
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


/*******************************************************************************
Function: lookuppassword
Purpose: Looks up the users password from our directory. Our data must be 
populated via the http server. This also then limits the look up on rogue
clients trying to randomly lookup username/password combos. This may create a 
slower load on startup and greater memory requirement. But seems to be the best
option.
Updated: 22.01.2019
*******************************************************************************/
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

/*******************************************************************************
Function: lookuppassword
Purpose: As above but can pass in string references.
Updated: 20.02.2019
*******************************************************************************/
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

/*******************************************************************************
Function: userexist
Purpose: Looks up a user in a domain to see if they exist.
Updated: 20.02.2019
*******************************************************************************/
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


/*******************************************************************************
Function: lookupdomain
Purpose: Returns a domain object from our directory (assuming we have one).
Updated: 22.01.2019
*******************************************************************************/
projectsipdirdomain::pointer projectsipdirdomain::lookupdomain( substring domain )
{
  projectsipdirdomain::pointer ref;

  std::string dstr = domain.str();
  stringvector l = splitstring( dstr, '.' );
  if( l.size() > 5 )
  {
    return ref;
  }

  projectsipdirdomain::map *dirref = &dir.subdomains;
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

/*******************************************************************************
Function: lookupdomain
Purpose: Returns a domain object from our directory (assuming we have one).
Updated: 22.01.2019
*******************************************************************************/
projectsipdirdomain::pointer projectsipdirdomain::lookupdomain( std::string &dstr )
{
  projectsipdirdomain::pointer ref;
  stringvector l = splitstring( dstr, '.' );
  if( l.size() > 5 )
  {
    return ref;
  }

  projectsipdirdomain::map *dirref = &dir.subdomains;
  projectsipdirdomain::map::iterator it = dirref->end();
  for( int i = l.size(); i != 0; i-- )
  {
    it = dirref->find( l[ i - 1 ] );
    if( dirref->end() == it )
    {
      projectsipdirdomain::pointer n;
      return n;
    }
    ref = it->second;
    dirref = &it->second->subdomains;
  }

  return ref;
}

/*******************************************************************************
Function: adddomain
Purpose: Returns a domain object from our directory (assuming we have one).
Updated: 20.02.2019
*******************************************************************************/
projectsipdirdomain::pointer projectsipdirdomain::adddomain( std::string &domain )
{
  projectsipdirdomain::pointer retval;

  stringvector parts = splitstring( domain, '.' );

  map *domptr = &dir.subdomains;

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

/*******************************************************************************
Function: geturiforcontrol
Purpose: Returns the full http uri for the path given.
Updated: 22.02.2019
*******************************************************************************/
std::string projectsipdirdomain::geturiforcontrol( std::string path )
{

  std::string controluri = "http://";
  controluri += *this->controlhost;
  
  if( 80 != this->controlport )
  {
    controluri += ":";
    controluri += std::to_string( this->controlport );
  }

  if( '/' != path[ 0 ] )
  {
    controluri += "/";
  }
  controluri += path;
  return controluri;
}


/*******************************************************************************
Function: httpget
Purpose: Report on directoru information
Updated: 20.02.2019
*******************************************************************************/
void projectsipdirdomain::httpget( stringvector &path, projectwebdocument &response )
{
  if( path.size() > 1 )
  {
    projectsipdirdomain::pointer ptr = lookupdomain( path[ 1 ] );
    if( ptr )
    {
      JSON::Object v;
      v[ "count" ] = ( JSON::Integer ) ptr->users.size();
      std::string t = JSON::to_string( v );

      response.setstatusline( 200, "Ok" );
      response.addheader( projectwebdocument::Content_Length, t.length() );
      response.addheader( projectwebdocument::Content_Type, "text/json" );
      response.setbody( t.c_str() );
      return;
    }
  }

  response.setstatusline( 404, "Not found" );
}

/*******************************************************************************
Function: httppost
Purpose: Respods to post requests form our control server.
It needs to handle the following array:
[
  {
    domain: "bling.babblevoice.com",
    users:
    [
      {
        username: "1000",
        secret: "blah"
      }
    ]
  }
]
Updated: 20.02.2019
*******************************************************************************/
void projectsipdirdomain::httppost( stringvector &path, JSON::Value &body, projectwebdocument &response )
{
  JSON::Array mainarray = JSON::as_array( body );

  JSON::Array::values_t::iterator it;
  for( it = mainarray.values.begin();
        it != mainarray.values.end();
        it++ )
  {
    JSON::Object d = JSON::as_object( *it );

    std::string domain = JSON::as_string( d[ "domain" ] );
    JSON::Object control = JSON::as_object( d[ "control" ] );

    projectsipdirdomain::pointer domentry = projectsipdirdomain::adddomain( domain );

    domentry->controlhost = stringptr( new std::string( JSON::as_string( control[ "host" ] ) ) );
    domentry->controlport = JSON::as_int64( control[ "port" ] );

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
  }

  response.setstatusline( 200, "Ok" );
  response.addheader( projectwebdocument::Content_Length, 0 );
}


