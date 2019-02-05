

#ifndef PROJECTSIPDIALOG_H
#define PROJECTSIPDIALOG_H

#include <boost/enable_shared_from_this.hpp>

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
#include "json.hpp"

/*******************************************************************************
Class: projectsipdialog
Purpose: Class to hold details about a specific dialog (INVITE).
Updated: 23.12.2018
*******************************************************************************/
class projectsipdialog :
  public boost::enable_shared_from_this< projectsipdialog >
{
public:
  projectsipdialog();
  ~projectsipdialog();
  typedef boost::shared_ptr< projectsipdialog > pointer;
  static pointer create();
  static bool invitesippacket( projectsippacketptr pk );
  static void httpget( stringvector &path, projectwebdocument &response );
  static void httppost( stringvector &path, JSON::Value &body, projectwebdocument &response );

  std::string callid;
  std::string alertinfo;

  /* our state functions */
  void invitestart( projectsippacketptr pk );
  void inviteauth( projectsippacketptr pk  );
  void donetrying( projectsippacketptr pk );
  void waitforackanddie( projectsippacketptr pk );
  std::function<void ( projectsippacketptr pk ) > laststate;
  std::function<void ( projectsippacketptr pk ) > nextstate;
  std::function<void ( void ) > timerstate;

  /* non state function */
  void passtocontrol( projectsippacketptr pk );
  void httpcallback( int errorcode );

  void waitfortimer( std::chrono::seconds s );
  void timerhandler( const boost::system::error_code& error );
  void canceltimer( void );
private:

  /* responses */
  void temporaryunavailable( void );
  void trying( void );
  void ringing( void );
  void answer( void );

  /* clean up */
  void untrack( void );

  projecthttpclient::pointer controlrequest;

  projectsippacketptr authrequest;
  projectsippacketptr lastpacket;

  boost::asio::steady_timer timer;

  int retries;
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



