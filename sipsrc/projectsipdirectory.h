

#ifndef PROJECTSIPDIRECTORY_H
#define PROJECTSIPDIRECTORY_H

#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "projectsipstring.h"
#include "projectwebdocument.h"
#include "projectsipstring.h"

/*******************************************************************************
File: projectsipdirectory.h/cpp.
Purpose: Store and lookup actualy username and passwords. Cache the results
in a fast lookup memory structure.
*******************************************************************************/
class projectsipdiruser :
  public boost::enable_shared_from_this< projectsipdiruser >
{
public:
  projectsipdiruser();
  typedef boost::shared_ptr< projectsipdiruser > pointer;
  static pointer create();

  stringptr username;
  stringptr secret;
};

typedef boost::unordered_map< std::string, projectsipdiruser::pointer > projectsipdirusers;


/*******************************************************************************
Class: projectsipdirdomain
Purpose: Store information about a domains. For example, com would point to a 
subdomain or pottentially users.
Updated: 22.01.2019
*******************************************************************************/
class projectsipdirdomain :
  public boost::enable_shared_from_this< projectsipdirdomain >
{
public:
  projectsipdirdomain();
  typedef boost::shared_ptr< projectsipdirdomain > pointer;
  typedef boost::unordered_map< std::string, pointer > map;
  static pointer create();

  map subdomains;
  stringptr domain;
  projectsipdirusers users;
};



/*******************************************************************************
Class: projectsipdirectory
Purpose: Lookup and store domain, username and password information for our UACs.
Updated: 09.01.2019
*******************************************************************************/
class projectsipdirectory 
{
public:
  projectsipdirectory( void );
  static stringptr lookup( substring domain, substring user );

  projectsipdirdomain::map domains;
};

#endif /* PROJECTSIPDIRECTORY_H */

