

#ifndef PROJECTSIPDIRECTORY_H
#define PROJECTSIPDIRECTORY_H

#include <unordered_map>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "projectsipstring.h"
#include "projectwebdocument.h"
#include "projectsipstring.h"

#include "json.hpp"

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

typedef std::unordered_map< std::string, projectsipdiruser::pointer > projectsipdirusers;


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
  typedef std::unordered_map< std::string, pointer > map;
  static pointer create();

  map subdomains;
  stringptr domain;

  stringptr controlhost;
  short controlport;

  projectsipdirusers users;

  static bool userexist( std::string &domain, std::string &user );
  static stringptr lookuppassword( substring domain, substring user );
  static stringptr lookuppassword( std::string &domain, std::string &user );
  static projectsipdirdomain::pointer lookupdomain( substring domain );
  static projectsipdirdomain::pointer lookupdomain( std::string &domain );

  /* Addng entries to the directory */
  static projectsipdirdomain::pointer adddomain( std::string &domain );
  void adduser( std::string &user, std::string &secret );

  std::string geturiforcontrol( std::string path );

  static void httpget( stringvector &path, projectwebdocument &response );
  static void httppost( stringvector &path, JSON::Value &body, projectwebdocument &response );
};




#endif /* PROJECTSIPDIRECTORY_H */

