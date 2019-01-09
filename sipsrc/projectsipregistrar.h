

#ifndef PROJECTSIPREGISTRAR_H
#define PROJECTSIPREGISTRAR_H

#define OPTIONSPINGFREQUENCY 30
#define OPTIONSMAXFAILCOUNT 7

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <string>

#include <boost/unordered_map.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/shared_ptr.hpp>

#include "projectsippacket.h"

/*******************************************************************************
Class: projectsipregistration
Purpose: Class to hold details about a specific registration.
Updated: 18.12.2018
*******************************************************************************/
class projectsipregistration
{
public:
  projectsipregistration( std::string u, int expires /*seconds */ );

  std::string user; /* fully qualified user@domain */
  boost::posix_time::ptime expires; /* when we exipre the registration */
  boost::posix_time::ptime nextping; /* when is the next options (ping) due */
  int outstandingping; /* how many pings we have sent without a response. */

  /* our state functions */
  void regstart( void );
  void regwaitauth( void );

  typedef void (projectsipregistration::*regstate)( void );
  regstate lastreg;
  regstate nextreg;

};

typedef boost::shared_ptr< projectsipregistration > projectsipregistrationptr;

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

/* Public functions */
void processsippacket( projectsippacket &pk );

#ifdef TESTCODE

void testregs( void );

#endif /* TESTCODE */

#endif  /* PROJECTSIPREGISTRAR_H */


