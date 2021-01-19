

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

#include <string>
#include <ctime>

#include "projectsipstring.h"
#include "projectsippacket.h"
#include "projecthttpclient.h"
#include "projectsipendpoint.h"
#include "json.hpp"

#define DIALOGSETUPTIMEOUT 180000  /* 3 minutes */
#define DIALOGACKTIMEOUT 3000  /* 3 seconds */
#define DIALOGSE 3600000 /* 1 hour */

/*!md
# projectsipdialog
Class to hold details about a specific dialog (INVITE).

*/
class projectsipdialog :
  public projectsipendpoint
{
public:
  projectsipdialog();
  virtual ~projectsipdialog();

  typedef boost::shared_ptr< projectsipdialog > pointer;
  static projectsipdialog::pointer create();

  static bool invitesippacket( projectsippacket::pointer pk );
  static void httpget( stringvector &path, projectwebdocument &response );
  static void httpput( stringvector &path, JSON::Value &body, projectwebdocument &response );
  static void httppost( stringvector &path, JSON::Value &body, projectwebdocument &response );

  std::string callid;
  std::string alertinfo;
  uint32_t cseq;

  /* our state functions */

  /* inbound invite */
  void invitestart( projectsippacket::pointer pk );
  void handlerefer( projectsippacket::pointer pk );
  void inviteauth( projectsippacket::pointer pk  );
  void referauth( projectsippacket::pointer pk  );
  void invite( projectsippacket::pointer pk  );
  void refer( projectsippacket::pointer pk  );
  bool checkforhold( void );
  void waitfornextinstruction( projectsippacket::pointer pk );
  void waitforack( projectsippacket::pointer pk );
  void waitforackanddie( projectsippacket::pointer pk );
  void waitfor200( projectsippacket::pointer pk );
  void waitfor200anddie( projectsippacket::pointer pk );
  void waitfor200thenackanddie( projectsippacket::pointer pk );
  void waitforacktheninviteauth( projectsippacket::pointer pk );

  /* outbound invite */
  void waitforinviteprogress( projectsippacket::pointer pk );
  void waitforinviteprogressafterauth( projectsippacket::pointer pk );

  /* Our state */
  std::function<void ( projectsippacket::pointer pk ) > nextstate;

  /* non state function */
  void handlebye( projectsippacket::pointer pk );
  bool updatecontrol( void );
  void httpcallback( int errorcode );

  /* timer functions */
  void waitfortimer( std::chrono::milliseconds s, std::function<void ( const boost::system::error_code& error ) > );
  void canceltimer( void );

  /*timer handlers */
  void ontimeoutenddialog( const boost::system::error_code& error );
  void ontimeout486andenddialog( const boost::system::error_code& error );
  void resend200( const boost::system::error_code& error );
  void resend401( const boost::system::error_code& error );
  void resenderror( const boost::system::error_code& error );
  void resendbye( const boost::system::error_code& error );
  void resendcancel( const boost::system::error_code& error );
  void resendnotify( const boost::system::error_code& error );

private:

  /* responses */
  void temporaryunavailable( void );
  void trying( void );
  void ringing( void );
  void answer( std::string body );
  void ok( std::string body = "" );
  void notify( void );
  void hangup( void );
  void send200( std::string body = "", bool final = false );
  void send202( void );
  void send401( void );
  void send403( void );
  void sendack( void );
  void sendcancel( void );
  void senderror( void );

  /* Verbs */
  bool sendinvite( void );
  void sendbye( void );
  void sendnotify( void );

  /* clean up */
  void untrack( void );

  projecthttpclient::pointer controlrequest;

  projectsippacket::pointer invitepk;
  projectsippacket::pointer referpk;
  projectsippacket::pointer ackpk;

  projectsippacket::pointer authrequest;
  projectsippacket::pointer lastackpacket;

  boost::asio::steady_timer timer;
  boost::asio::steady_timer setimer;

  int retries;
  bool authenticated;
  bool originator;

  std::string domain;

  JSON::Value remotesdp;
  JSON::Value oursdp;

  bool callringing;
  bool callanswered;
  bool callhungup;
  bool callhold;

  std::time_t startat;
  std::time_t ringingat;
  std::time_t answerat;
  std::time_t endat;

  substring ourtag;
  substring theirtag;

  /* outbound values */
  std::string authsecret;
  std::string fromuser;
  std::string todomain;
  std::string touser;
  int maxforwards;
  std::string callerid, calleridname;
  bool hidecid;
  std::string control;
};


/* tags for multi index */
struct projectsipdialogcallid{};

/*!md
# projectsipdialogs
Boost multi index set to store our dialogs.
*/
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
