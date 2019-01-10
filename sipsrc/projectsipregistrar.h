

#ifndef PROJECTSIPREGISTRAR_H
#define PROJECTSIPREGISTRAR_H

#define OPTIONSPINGFREQUENCY 30
#define OPTIONSMAXFAILCOUNT 7
#define DEFAULTSIPEXPIRES 3600

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <string>

#include <boost/unordered_map.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/shared_ptr.hpp>
#include <functional>

#include "projectsippacket.h"

/*******************************************************************************
Class: projectsipregistration
Purpose: Class to hold details about a specific registration.
Updated: 18.12.2018
*******************************************************************************/
class projectsipregistration
{
public:
  projectsipregistration( std::string u );

  std::string user; /* fully qualified user@domain */
  boost::posix_time::ptime expires; /* when we expire the registration */
  boost::posix_time::ptime nextping; /* when is the next options (ping) due */
  int outstandingping; /* how many pings we have sent without a response. */

  /* our state functions */
  void regstart( projectsippacketptr pk );
  void regwaitauth( projectsippacketptr pk );

  std::function<void ( projectsippacketptr pk ) > nextstate;
  std::function<void ( projectsippacketptr pk ) > laststate;
};

typedef boost::shared_ptr< projectsipregistration > projectsipregistrationptr;

class projectsipupdateexpires
{
public:
  projectsipupdateexpires( const boost::posix_time::ptime &newexpire )
      : expires( newexpire ){}

  void operator()( projectsipregistrationptr e )
  {
    e->expires = this->expires;
  }

private:
  boost::posix_time::ptime expires;
};

/*******************************************************************************
Functions: Some public functions.
Updated: 10.01.2019
*******************************************************************************/
void registrarsippacket( projectsippacketptr pk );

/* tags for multi index */
struct regindexuser{};
struct regindexexpires{};
struct regindexnextping{};

/*******************************************************************************
Class: projectsipregistrations
Purpose: Boost multi index set to store our registrations. Indexed by user
(hashed unique), expires (ordered none unique) and nextping (ordered none unique).
Updated: 18.12.2018
*******************************************************************************/
typedef boost::multi_index::multi_index_container<
  projectsipregistrationptr,
  boost::multi_index::indexed_by
  <
    boost::multi_index::hashed_unique
    <
      boost::multi_index::tag< regindexuser >,
      boost::multi_index::member
      <
        projectsipregistration,
        std::string,
        &projectsipregistration::user
      >
    >,
    boost::multi_index::ordered_non_unique
    <
      boost::multi_index::tag< regindexexpires >,
      boost::multi_index::member
      <
        projectsipregistration,
        boost::posix_time::ptime,
        &projectsipregistration::expires
      >
    >,
    boost::multi_index::ordered_non_unique
    <
      boost::multi_index::tag< regindexnextping >,
      boost::multi_index::member
      <
        projectsipregistration,
        boost::posix_time::ptime,
        &projectsipregistration::nextping
      >
    >
  >
> projectsipregistrations;

#ifdef TESTCODE

void testregs( void );

#endif /* TESTCODE */

#endif  /* PROJECTSIPREGISTRAR_H */


