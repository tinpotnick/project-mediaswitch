


#ifndef PROJECTWEBDOCUMENT_H
#define PROJECTWEBDOCUMENT_H

#define MAX_HEADERS 20
#define MAX_DUPLICATEHEADERS 3
#define METHODUNKNOWN -1
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
  inline substring()
  {
    this->startpos = 0;
    this->endpos = 0;
  }

  inline substring( char * s )
  {
    this->s = stringptr( new std::string( s ) );
    this->startpos = 0;
    this->endpos = this->s->length();
  }

  inline substring( stringptr s )
  {
    this->s = s;
    this->startpos = 0;
    this->endpos = s->length();
  }

  inline substring( stringptr s, size_t start, size_t end )
  {
    this->s = s;
    this->startpos = start;
    this->endpos = end;
  }

  inline stringptr substr()
  {
    if ( !this->s )
    {
      return stringptr( new std::string( "" ) );
    }

    if( 0 == this->endpos )
    {
      return stringptr( new std::string( "" ) );
    }

    if( this->startpos > s->size() )
    {
      return stringptr( new std::string( "" ) );
    }

    if( this->endpos > s->size() )
    {
      return stringptr( new std::string( "" ) );
    }
    return stringptr( new std::string( s->substr( this->startpos, this->endpos - this->startpos ) ) );
  }

  inline size_t end()
  {
    return this->endpos;
  }
  inline size_t start()
  {
    return this->startpos;
  }

  inline size_t end( size_t end )
  {
    this->endpos = end;
    return this->endpos;
  }
  inline size_t start( size_t start )
  {
    this->startpos = start;
    return this->startpos;
  }

  inline size_t operator++( int )
  {
    return this->endpos ++;
  }

  inline size_t operator--( int )
  {
    return this->endpos --;
  }

  inline size_t length( void )
  {
    return this->endpos - this->startpos;
  }

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
and values.
Updated: 16.12.2018
*******************************************************************************/
class projectwebdocument
{
public:
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
  void setbody( stringptr body );


  /* To be overridden by the (udp/tcp/test etc) server */
  virtual void respond( stringptr doc ) {};

  /* By default return our underlying document, however can be
  ovveridden to generate protocol specific document from headers etc */
  virtual stringptr strptr() { return this->document; };

protected:
#warning Finish off the default HTTP versions
  virtual int getheaderfromcrc( int crc ) = 0;
  virtual int getmethodfromcrc( int crc ) = 0;
  virtual const char *getheaderstr( int header ) = 0;
  virtual const char *getmethodstr( int method ) = 0;

  void parsersline( void );
  void parseheaders( void );
  substring getheadervalue( substring header );

  substring headers[ MAX_HEADERS * MAX_DUPLICATEHEADERS ];
  stringptr document;
  substring body;
  bool headersparsed;
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
