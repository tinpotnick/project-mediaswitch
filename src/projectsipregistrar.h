

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
};

/*******************************************************************************
Class: projectsipregistrations
Purpose: Boost multi index set to store our registrations. Indexed by user
(hashed unique), expires (ordered none unique) and nextping (ordered none unique).
Updated: 18.12.2018
*******************************************************************************/
typedef boost::multi_index::multi_index_container<
  projectsipregistration,
  boost::multi_index::indexed_by
  <
    boost::multi_index::hashed_unique
    <
      boost::multi_index::member
      <
        projectsipregistration,
        std::string ,
        &projectsipregistration::user
      >
    >,
    boost::multi_index::ordered_non_unique
    <
      boost::multi_index::member
      <
        projectsipregistration,
        boost::posix_time::ptime,
        &projectsipregistration::expires
      >
    >,
    boost::multi_index::ordered_non_unique
    <
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

#endif /* TESTCODE */

#endif  /* PROJECTSIPREGISTRAR_H */


