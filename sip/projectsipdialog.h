

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

#include <ctime>

#include "projectsipstring.h"
#include "projectsippacket.h"
#include "projecthttpclient.h"
#include "json.hpp"

#define DIALOGSETUPTIMEOUT 180000  /* 3 minutes */
#define DIALOGACKTIMEOUT 3000  /* 3 seconds */

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
  static void httpput( stringvector &path, JSON::Value &body, projectwebdocument &response );
  static void httppost( stringvector &path, JSON::Value &body, projectwebdocument &response );

  std::string callid;
  std::string alertinfo;

  /* our state functions */

  /* inbound invite */
  void invitestart( projectsippacketptr pk );
  void inviteauth( projectsippacketptr pk  );
  void waitfornextinstruction( projectsippacketptr pk );
  void waitforack( projectsippacketptr pk );
  void waitforackanddie( projectsippacketptr pk );
  void waitfor200anddie( projectsippacketptr pk );

  /* outbound invite */
  void waitforinviteprogress( projectsippacketptr pk );

  std::function<void ( projectsippacketptr pk ) > laststate;
  std::function<void ( projectsippacketptr pk ) > nextstate;

  /* non state function */
  void handlebye( projectsippacketptr pk );
  bool updatecontrol( void );
  void httpcallback( int errorcode );

  /* timer functions */
  void waitfortimer( std::chrono::milliseconds s, std::function<void ( const boost::system::error_code& error ) > );
  void canceltimer( void );

  /*timer handlers */
  void ontimeoutenddialog( const boost::system::error_code& error );
  void ontimeout486andenddialog( const boost::system::error_code& error );
  void resend200( const boost::system::error_code& error );
  void resenderror( const boost::system::error_code& error );
  void resendbye( const boost::system::error_code& error );

private:

  /* responses */
  void temporaryunavailable( void );
  void trying( void );
  void ringing( void );
  void answer( std::string body );
  void hangup( void );
  void send200( std::string body = "", bool final = false );
  void sendack( void );
  void senderror( void );

  /* Verbs */
  void sendinvite( JSON::Object &request, projectwebdocument &response );
  void sendbye( void );

  /* clean up */
  void untrack( void );

  projecthttpclient::pointer controlrequest;

  projectsippacketptr invitepacket;
  projectsippacketptr authrequest;
  projectsippacketptr lastpacket;
  projectsippacketptr lastackpacket;

  boost::asio::steady_timer timer;

  int retries;
  bool authenticated;

  std::string domain;
  JSON::Value remotesdp;

  bool callringing;
  bool callanswered;
  bool callhungup;

  std::time_t startat;
  std::time_t ringingat;
  std::time_t answerat;
  std::time_t endat;

  int errorcode;
  std::string errorreason;

  stringptr ourtag;
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
