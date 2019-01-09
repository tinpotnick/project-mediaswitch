


#ifndef PROJECTWEBDOCUMENT_H
#define PROJECTWEBDOCUMENT_H

#define MAX_HEADERS 20
#define MAX_DUPLICATEHEADERS 3
#define METHODBADFORMAT -2
#define STATUSUNKNOWN -1
#define DEFAULTHEADERLINELENGTH 200

#include <array>
#include <string>
#include <boost/shared_ptr.hpp>

// Please remove
#include <iostream>
#include <stdio.h>

typedef boost::shared_ptr< std::string > stringptr;

/*******************************************************************************
Function: moveonbyn
Purpose: Little helper function - safely move on n chars in a string.
Updated: 12.12.2018
*******************************************************************************/
inline std::string::iterator moveonbyn( stringptr s, size_t n )
{
  return s->length() <= n ? s->end() : s->begin() + n;
}

inline void moveonbyn( stringptr str, std::string::iterator &s, size_t n )
{
  for( size_t i = 0; i < n; i++ )
  {
    s++;
    if( str->end() == s )
    {
      return;
    }
  }
}

/*******************************************************************************
Class: substring
Purpose: Helper class, used to hold indexes into a larger string so that we
only need to maintain 1 buffer.
Updated: 12.12.2018
*******************************************************************************/
class substring
{
public:
  substring();
  substring( char * s );
  substring( stringptr s );
  substring( stringptr s, size_t start, size_t end );
  stringptr substr();
  const char *c_str(){ return this->s->c_str() + this->startpos; };

  size_t end();
  size_t end( size_t end );
  size_t start();
  size_t start( size_t start );

  substring rfind( const char * );
  substring find( const char * );
  substring rfind( const char );
  substring find( const char );

  size_t operator++( int );

  size_t operator--( int );
  size_t length( void );
  int toint();

  friend bool operator != ( const substring& lhs, const char *rhs );
  friend bool operator == ( const substring& lhs, const char *rhs );

private:
  size_t startpos;
  size_t endpos;
  stringptr s;
};

bool operator==( const substring& lhs, const char *rhs );
bool operator!=( const substring& lhs, const char *rhs );

#ifdef TESTCODE
/*******************************************************************************
Function: operator << ostream, substring
Purpose: For use with our tests, out put the result of a comparison. Not for live.
Updated: 30.12.2018
*******************************************************************************/
inline std::ostream & operator << ( std::ostream& os, substring obj )
{
  os << *( obj.substr() );
  return os;
}
#endif /* TESTCODE */

typedef boost::shared_ptr< substring > substringptr;

stringptr urldecode( stringptr str );
stringptr urlencode( stringptr str );

/*******************************************************************************
Class: httpuri
Purpose: Helper class to parse urls. Must be paired with the origional buffer
to retreive the actual string. TODO move this into our http file.
Updated: 17.12.2018
*******************************************************************************/
class httpuri
{
public:
  inline httpuri( stringptr s )
  {
    this->s = s;
    size_t protopos = s->find( "://" );
    if( std::string::npos != protopos )
    {
      this->protocol = substring( s, 0, protopos );
    }

    size_t hostpos = this->protocol.end();
    if( hostpos != 0 )
    {
      hostpos += 3;
    }

    hostpos = s->find( '/', hostpos );
    if( std::string::npos == hostpos )
    {
      hostpos = s->size();
    }
    this->host = substring( s, this->protocol.end() + 3, hostpos );
    if( this->host.end() == s->size() )
    {
      return;
    }

    size_t querypos = s->find( '?', this->host.end() );
    if( std::string::npos != querypos )
    {
      this->query = substring( s, querypos + 1, s->size() );
      /* The path is inbetween the host and query */
      this->path = substring( s, this->host.end(), querypos );
      return;
    }

    this->path = substring( s, this->host.end(), s->size() );

  }

  stringptr s;
  substring protocol;
  substring host;
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
  enum { OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT, RESPONSE, METHODUNKNOWN };

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
  
  /*
    Get functions.
  */
  int getmethod( void );
  int getstatuscode( void );
  stringptr getreasonphrase( void );
  stringptr getrequesturi( void );
  substring getheader( int header );
  stringptr getbody();
  bool hasheader( int header );

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

  /* By default return our underlying document, however can be
  ovveridden to generate protocol specific document from headers etc */
  virtual stringptr strptr() { return this->document; };

protected:
  virtual int getheaderfromcrc( int crc );
  virtual int getmethodfromcrc( int crc );
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



#endif /* PROJECTWEBDOC_H */
