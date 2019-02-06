

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
  void waitfornextinstruction( projectsippacketptr pk );
  void waitforack( projectsippacketptr pk );
  void waitforackanddie( projectsippacketptr pk );
  std::function<void ( projectsippacketptr pk ) > laststate;
  std::function<void ( projectsippacketptr pk ) > nextstate;

  /* non state function */
  void handlebye( projectsippacketptr pk );
  void updatecontrol( projectsippacketptr pk );
  void httpcallback( int errorcode );

  /* timer functions */
  void waitfortimer( std::chrono::milliseconds s, std::function<void ( const boost::system::error_code& error ) > );
  void canceltimer( void );

  /*timer handlers */
  void ontimeoutenddialog( const boost::system::error_code& error );
  void resend200( const boost::system::error_code& error );

private:

  /* responses */
  void temporaryunavailable( void );
  void trying( void );
  void ringing( void );
  void answer( void );
  void send200( bool final = false );

  /* clean up */
  void untrack( void );

  projecthttpclient::pointer controlrequest;

  projectsippacketptr authrequest;
  projectsippacketptr lastpacket;

  boost::asio::steady_timer timer;

  int retries;
  bool authenticated;

  bool callringing;
  bool callanswered;
  bool callhungup;
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



