

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
Purpose: Store and lookup actually username and passwords. Cache the results
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
subdomain or potentially users.
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

  projectsipdirusers users;

  static bool userexist( std::string &domain, std::string &user );
  bool userexist( std::string &user );
  static stringptr lookuppassword( substring domain, substring user );
  static stringptr lookuppassword( std::string &domain, std::string &user );
  static projectsipdirdomain::pointer lookupdomain( substring domain );
  static projectsipdirdomain::pointer lookupdomain( std::string &domain );

  /* Adding entries to the directory */
  static projectsipdirdomain::pointer adddomain( std::string &domain );
  static bool removedomain( std::string &domain, bool force = true );
  void adduser( std::string &user, std::string &secret );
  void removeuser( std::string &user );

  static void httpget( stringvector &path, projectwebdocument &response );
  static void httpput( stringvector &path, JSON::Value &body, projectwebdocument &response );
  static void httppatch( stringvector &path, JSON::Value &body, projectwebdocument &response );
  static void httpdelete( stringvector &path, projectwebdocument &response );
};




#endif /* PROJECTSIPDIRECTORY_H */
