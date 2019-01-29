

#ifndef PROJECTSIPDIALOG_H
#define PROJECTSIPDIALOG_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <boost/asio.hpp>

#include <boost/shared_ptr.hpp>
#include <functional>

#include "projectsipstring.h"
#include "projectsippacket.h"
#include "projecthttpclient.h"

/*******************************************************************************
Class: projectsipdialog
Purpose: Class to hold details about a specific dialog (INVITE).
Updated: 23.12.2018
*******************************************************************************/
class projectsipdialog
{
public:
  projectsipdialog();
  typedef boost::shared_ptr< projectsipdialog > pointer;
  static pointer create();

  std::string callid;

  /* our state functions */
  void invitestart( projectsippacketptr pk );
  void inviteauth( projectsippacketptr pk  );
  void donetrying( projectsippacketptr pk );
  std::function<void ( projectsippacketptr pk ) > laststate;
  std::function<void ( projectsippacketptr pk ) > nextstate;

  /* non state function */
  void passtocontrol( projectsippacketptr pk );

  static bool invitesippacket( projectsippacketptr pk );

  void httpcallback( int errorcode );
private:

  projecthttpclient::pointer controlrequest;

  projectsippacketptr authrequest;
  projectsippacketptr lastpacket;
};


/* tags for multi index */
struct projectsipdialogcallid{};

/*******************************************************************************
Class: projectsipdialogs
Purpose: Boost multi index set to store our dialogs. 
Updated: 23.01.2019
*******************************************************************************/
typedef boost::multi_index::multi_index_container<
  projectsipdialog::pointer,
  boost::multi_index::indexed_by
  <
    boost::multi_index::hashed_unique
    <
      boost::multi_index::tag< projectsipdialogcallid >,
      boost::multi_index::member
      <
        projectsipdialog,
        std::string,
        &projectsipdialog::callid
      >
    >
  >
> projectsipdialogs;



#endif /* PROJECTSIPDIALOG_H */



