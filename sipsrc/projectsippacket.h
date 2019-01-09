

#ifndef PROJECTSIPPACKET_H
#define PROJECTSIPPACKET_H

#include <string>
#include <boost/shared_ptr.hpp>

#include "projectwebdocument.h"

#define MASSIPPACKETLENGTH 1500

/*******************************************************************************
Class: sipuri
Purpose: Parse a SIP URI ref RFC 3261.
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
Updated: 17.12.2018
*******************************************************************************/
class sipuri
{
public:
  sipuri( stringptr s );
  substring getparameter( std::string name );
  substring getheader( std::string name );

  stringptr s;
  substring displayname;
  substring protocol;
  substring user;
  substring secret;
  substring host;
  substring parameters;
  substring headers;
};

/*******************************************************************************
Class: projectsippacket
Purpose: Unpacks data following the standard: https://www.ietf.org/rfc/rfc3261.txt.
Our purpose is to be efficient (fast), we are not checking for perfect syntax
this object should still work even with inperfect SIP packets. Keep memory
allocation to a minimum. Work off just the main packet string and keep indexes
into that packet.
Updated: 12.12.2018
*******************************************************************************/
class projectsippacket : public projectwebdocument
{

public:
  projectsippacket();
  projectsippacket( stringptr packet );
  virtual ~projectsippacket();

  static stringptr branch();
  substring getheaderparam( int header, const char *param );

  /* specific headers */
  bool addviaheader( const char *host, projectsippacket *ref );

  /*
    Request-Line  =  Method SP Request-URI SP SIP-Version CRLF
    RESPONSE indicates it found a status code - so is a response
    not a request.
  */
  enum { REGISTER = projectwebdocument::METHODUNKNOWN + 1, INVITE, ACK, CANCEL, BYE, OPTIONS };

  enum { Authorization,
        Allow,
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
        Route,
        Retry_After,
        Supported,
        To,
        Via,
        User_Agent,
        WWW_Authenticate };

private:

  virtual int getheaderfromcrc( int crc );
  virtual const char* getversion( void );
  virtual int getmethodfromcrc( int crc );

  virtual const char *getheaderstr( int header );
  virtual const char *getmethodstr( int method );
  
};

typedef boost::shared_ptr< projectsippacket > projectsippacketptr;


#endif /* PROJECTSIPPACKET_H */
