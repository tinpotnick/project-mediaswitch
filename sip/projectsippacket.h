

#ifndef PROJECTSIPPACKET_H
#define PROJECTSIPPACKET_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include "projectwebdocument.h"

#define MASSIPPACKETLENGTH 1500

/*!md
# sipuri
Parse a SIP URI ref RFC 3261.
Examples:
  "A. G. Bell" <sip:agb@bell-telephone.com> ;tag=a48s
  sip:+12125551212@server.phone2net.com;tag=887s
  Anonymous <sip:c8oqz84zk7z@privacy.org>;tag=hyh8
More examples, from section 19.1.3
  sip:alice@atlanta.com
  sip:alice:secretword@atlanta.com;transport=tcp
  sips:alice@atlanta.com?subject=project%20x&priority=urgent
  sip:+1-212-555-1212:1234@gateway.com;user=phone
  sips:1212@gateway.com
  sip:alice@192.0.2.4
  sip:atlanta.com;method=REGISTER?to=alice%40atlanta.com
  sip:alice;day=tuesday@atlanta.com

  Strings are encoded by RFC 2396.
*/
class sipuri
{
public:
  sipuri( substring s );
  substring getparameter( std::string name );
  substring getheader( std::string name );

  substring s;
  /* uri- anything contained between the < & > or if not present the whole string.*/
  substring uri;
  substring displayname;
  substring protocol;
  substring user;
  substring secret;
  substring host;
  substring userhost;
  substring parameters;
  substring headers;
};

/*!md
# projectsippacket
Unpacks data following the standard: https://www.ietf.org/rfc/rfc3261.txt.
Our purpose is to be efficient (fast), we are not checking for perfect syntax
this object should still work even with inperfect SIP packets. Keep memory
allocation to a minimum. Work off just the main packet string and keep indexes
into that packet.

*/
class projectsippacket : public projectwebdocument
{

public:
  projectsippacket();
  projectsippacket( stringptr packet );
  virtual ~projectsippacket();

  typedef boost::shared_ptr< projectsippacket > pointer;
  static projectsippacket::pointer create();
  static projectsippacket::pointer create( stringptr packet );

  static stringptr branch( void );
  static stringptr tag( void );
  static stringptr callid( void );
  static stringptr contact( stringptr user, stringptr host, int expires = 0 );

  substring getheaderparam( int header, const char *param );
  substring getuser( int tofrom = To );
  substring gethost( int tofrom = To );
  substring getdisplayname( int tofrom = To );
  substring getuserhost( int tofrom = To );
  substring geturihost( void );
  int getexpires( void );
  int getcseq( void );

  /* specific headers */
  bool addviaheader( const char *host, projectsippacket *ref = NULL );
  bool addwwwauthenticateheader( projectsippacket *ref );
  bool addremotepartyid( const char * realm,
                          const char *calleridname,
                          const char * callerid,
                          bool hide );
  bool checkauth( projectsippacket *ref, stringptr password );

  /*
    Request-Line  =  Method SP Request-URI SP SIP-Version CRLF
    RESPONSE indicates it found a status code - so is a response
    not a request.
  */
  enum { REGISTER = projectwebdocument::METHODUNKNOWN + 1, INVITE, ACK, CANCEL, BYE, OPTIONS };

  enum { Authorization,
        Allow,
        Alert_Info,
        Call_ID,
        Content_Length,
        CSeq,
        Contact,
        Content_Type,
        Expires,
        From,
        Max_Forwards,
        Proxy_Authenticate,
        Proxy_Authorization,
        Record_Route,
        Remote_Party_ID,
        Route,
        Retry_After,
        Reason,
        Supported,
        To,
        Via,
        User_Agent,
        Min_Expires,
        WWW_Authenticate };

private:

  virtual int getheaderfromcrc( int crc );
  virtual const char* getversion( void );
  virtual int getmethodfromcrc( int crc );

  virtual const char *getheaderstr( int header );
  virtual const char *getmethodstr( int method );
};


#endif /* PROJECTSIPPACKET_H */
