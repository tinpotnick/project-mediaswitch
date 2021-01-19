


#ifndef PROJECTWEBDOCUMENT_H
#define PROJECTWEBDOCUMENT_H

#define MAX_HEADERS 50
#define MAX_DUPLICATEHEADERS 3
#define METHODBADFORMAT -2
#define STATUSUNKNOWN -1
#define DEFAULTHEADERLINELENGTH 200

#include <iostream>
#include <stdio.h>

#include <boost/shared_ptr.hpp>

#include "projectsipstring.h"

/*******************************************************************************
Class: httpuri
Purpose: Helper class to parse urls. Must be paired with the origional buffer
to retreive the actual string. TODO move this into our http file.
Updated: 17.12.2018
*******************************************************************************/
class httpuri
{
public:
  httpuri(){};
  httpuri( substring s );

  substring s;
  substring protocol;
  substring host;
  substring port;
  substring path;
  substring query;
};

/*******************************************************************************
Class: projectwebdocument
Purpose: Base class with common function to parse web documents. For example
SIP packets or HTTP requests - who both have a header line followed by headers
and values. This class will handle HTTP.
Updated: 16.12.2018
*******************************************************************************/
class projectwebdocument
{
public:

  /* Methods */
  enum { OPTIONS, GET, HEAD, POST, PUT, PATCH, DELETE, TRACE, CONNECT, RESPONSE, METHODUNKNOWN };

  enum { /* Request */
          Accept,
          Accept_Charset,
          Accept_Encoding,
          Accept_Language,
          Authorization,
          Expect,
          From,
          Host,
          If_Match,
          If_Modified_Since,
          If_None_Match,
          If_Range,
          If_Unmodified_Since,
          Max_Forwards,
          Proxy_Authorization,
          Range,
          Referer,
          TE,
          User_Agent,
          /* Response */
          Accept_Ranges,
          Age,
          ETag,
          Location,
          Proxy_Authenticate,
          Retry_After,
          Server,
          Vary,
          WWW_Authenticate,
          /* Entity */
          Allow,
          Content_Encoding,
          Content_Language,
          Content_Length,
          Content_Location,
          Content_MD5,
          Content_Range,
          Content_Type,
          Expires,
          Last_Modified
        };

  projectwebdocument();
  projectwebdocument( stringptr doc );
  virtual ~projectwebdocument();

  virtual const char* getversion( void );

  bool iscomplete( void );
  void append( chararray &in, size_t length );

  /*
    Get functions.
  */
  int getmethod( void );
  int getstatuscode( void );
  substring getreasonphrase( void );
  substring getrequesturi( void );
  substring getheader( int header );
  substring getbody();
  bool hasheader( int header );
  bool isrequest( void );

  /*
    Set functions.
  */
  void setstatusline( int code, std::string reason );
  void setrequestline( int method, std::string uri );
  void addheader( int header, std::string value );
  void addheader( int header, substring value );
  void addheader( int header, const char * value );
  void addheader( int header, int value );
  void setbody( const char *body );


  /* To be overridden by the (udp/tcp/test etc) server */
  virtual void respond( stringptr doc ) {};
  virtual std::string getremotehost( void ) { return std::string(); }
  virtual unsigned short getremoteport( void ) { return 0; }

  /* By default return our underlying document, however can be
  ovveridden to generate protocol specific document from headers etc */
  virtual stringptr strptr() { return this->document; };

protected:
  virtual int getheaderfromcrc( unsigned int crc );
  virtual int getmethodfromcrc( unsigned int crc );
  virtual const char *getheaderstr( int header );
  virtual const char *getmethodstr( int method );

  void parsersline( void );
  void parseheaders( void );
  substring getheadervalue( substring header );

  substring headers[ MAX_HEADERS * MAX_DUPLICATEHEADERS ];
  stringptr document;
  substring body;
  bool headersparsed;
  int headercount;
  int method;
  int statuscode;

  substring methodstr;
  substring statuscodestr;
  substring reasonphrase;
  substring uri;
  substring rsline;

private:
  void storeheader( int headerindex, substring hval );
};

typedef boost::shared_ptr< projectwebdocument > projectwebdocumentptr;

#endif /* PROJECTWEBDOC_H */
