

#include <string>
#include <algorithm>
#include <boost/crc.hpp>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "projectwebdocument.h"


/*******************************************************************************
Function: projectwebdocument
Purpose: Constructor
Updated: 01.01.2019
*******************************************************************************/
projectwebdocument::projectwebdocument() :
  document( stringptr( new std::string() ) ),
  body( document ),
  headersparsed( false ),
  headercount( 0 ),
  method( METHODUNKNOWN ),
  statuscode( STATUSUNKNOWN ),
  methodstr( document ),
  statuscodestr( document ),
  reasonphrase( document )
{
}

/*******************************************************************************
Function: projectwebdocument
Purpose: Constructor
Updated: 12.12.2018
*******************************************************************************/
projectwebdocument::projectwebdocument( stringptr doc ) :
  document( doc ),
  body( doc ),
  headersparsed( false ),
  headercount( 0 ),
  method( METHODUNKNOWN ),
  statuscode( STATUSUNKNOWN ),
  methodstr( doc ),
  statuscodestr( doc ),
  reasonphrase( doc )
{
}

/*******************************************************************************
Function: projectwebdocument
Purpose: Destructor
Updated: 01.01.2019
*******************************************************************************/
projectwebdocument::~projectwebdocument()
{
}

/*******************************************************************************
Function: getversion
Purpose: As it says.
Updated: 02.01.2019
*******************************************************************************/
const char* projectwebdocument::getversion( void ) 
{
  return "HTTP/1.1";
}

/*******************************************************************************
Function: projectsippacket getmethodstr
Purpose: Convert a header id to a string.
Updated: 03.01.2019
*******************************************************************************/
const char *projectwebdocument::getheaderstr( int header )
{
  switch( header )
  {
    /* Request */
    case Accept:
      return "Accept";
    case Accept_Charset:
      return "Accept-Charset";
    case Accept_Encoding:
      return "Accept-Encoding";
    case Accept_Language:
      return "Accept-Language";
    case Authorization:
      return "Authorization";
    case Expect:
      return "Expect";
    case From:
      return "From";
    case Host:
      return "Host";
    case If_Match:
      return "If-Match";
    case If_Modified_Since:
      return "If-Modified-Since";
    case If_None_Match:
      return "If-None-Match";
    case If_Range:
      return "If-Range";
    case If_Unmodified_Since:
      return "If-Unmodified-Since";
    case Max_Forwards:
      return "Max-Forwards";
    case Proxy_Authorization:
      return "Proxy-Authorization";
    case Range:
      return "Range";
    case Referer:
      return "Referer";
    case TE:
      return "TE";
    case User_Agent:
      return "User-Agent";

    /* Response */
    case Accept_Ranges:
      return "Accept-Ranges";
    case Age:
      return "Age";
    case ETag:
      return "ETag";
    case Location:
      return "Location";
    case Proxy_Authenticate:
      return "Proxy-Authenticate";
    case Retry_After:
      return "Retry-After";
    case Server:
      return "Server";
    case Vary:
      return "Vary";
    case WWW_Authenticate:
      return "WWW-Authenticate";

    /* Entity */
    case Allow:
      return "Allow";
    case Content_Encoding:
      return "Content-Encoding";
    case Content_Language:
      return "Content-Language";
    case Content_Length:
      return "Content-Length";
    case Content_Location:
      return "Content-Location";
    case Content_MD5:
      return "Content-MD5";
    case Content_Range:
      return "Content-Range";
    case Content_Type:
      return "Content-Type";
    case Expires:
      return "Expires";
    case Last_Modified:
      return "Last-Modified";
    
    default:
      return "";
  }
}

/*******************************************************************************
Function: projectsippacket getmethodstr
Purpose: Convert a method id to a string.
Updated: 03.01.2019
*******************************************************************************/
const char *projectwebdocument::getmethodstr( int method )
{
  switch( method )
  {
    case OPTIONS:
      return "OPTIONS";
    case GET:
      return "GET";
    case HEAD:
      return "HEAD";
    case POST:
      return "POST";
    case PUT:
      return "PUT";
    case DELETE:
      return "DELETE";
    case TRACE:
      return "TRACE";
    case CONNECT:
      return "CONNECT";
    default:
      return "";
  }
}

/*******************************************************************************
Function: getmethodfromcrc
Purpose: Converts crc to method value. Switch statement comes from
gensipheadercrc.py. We only store references to the supported headers.
Updated: 03.01.2019
*******************************************************************************/
int projectwebdocument::getmethodfromcrc( int crc )
{
  switch( crc )
  {
    case 0xd035fa87:   /* options */
    {
      return OPTIONS;
    }
    case 0xfd3b2e70:   /* get */
    {
      return GET;
    }
    case 0xa7f3f69c:   /* head */
    {
      return HEAD;
    }
    case 0x5a8a6c8d:   /* post */
    {
      return POST;
    }
    case 0xae9089d4:   /* put */
    {
      return PUT;
    }
    case 0x3a127c87:   /* delete */
    {
      return DELETE;
    }
    case 0x315bd5a1:   /* trace */
    {
      return TRACE;
    }
    case 0x74cff91f:   /* connect */
    {
      return CONNECT;
    }
    default:
      return -1;
  }
}

/*******************************************************************************
Function: getheaderfromcrc
Purpose: Converts crc to header value. Switch statement comes from
gensipheadercrc.py. We only store references to the supported headers.
Updated: 03.01.2019
*******************************************************************************/
int projectwebdocument::getheaderfromcrc( int crc )
{
  switch( crc )
  {
    case 0xb320ed34:   /* accept */
    {
      return Accept;
    }
    case 0x16a25427:   /* accept-charset */
    {
      return Accept_Charset;
    }
    case 0xa036ae55:   /* accept-encoding */
    {
      return Accept_Encoding;
    }
    case 0x1ca526af:   /* accept-language */
    {
      return Accept_Language;
    }
    case 0x7a6d8bef:   /* authorization */
    {
      return Authorization;
    }
    case 0x1cc2b052:   /* expect */
    {
      return Expect;
    }
    case 0xb91aa170:   /* from */
    {
      return From;
    }
    case 0xcf2713fd:   /* host */
    {
      return Host;
    }
    case 0x45ab39d6:   /* if-match */
    {
      return If_Match;
    }
    case 0x6e2a7f4a:   /* if-modified-since */
    {
      return If_Modified_Since;
    }
    case 0x5b251281:   /* if-none-match */
    {
      return If_None_Match;
    }
    case 0xac77a69a:   /* if-range */
    {
      return If_Range;
    }
    case 0x1b92edff:   /* if-unmodified-since */
    {
      return If_Unmodified_Since;
    }
    case 0xb042e5b1:   /* max-forwards */
    {
      return Max_Forwards;
    }
    case 0x8c1d732:   /* proxy-authorization */
    {
      return Proxy_Authorization;
    }
    case 0x93875a49:   /* range */
    {
      return Range;
    }
    case 0x55dc3685:   /* referer */
    {
      return Referer;
    }
    case 0x37523bda:   /* te */
    {
      return TE;
    }
    case 0x82a3cb0f:   /* user-agent */
    {
      return User_Agent;
    }
    case 0xaea9d089:   /* accept-ranges */
    {
      return Accept_Ranges;
    }
    case 0xa13010b2:   /* age */
    {
      return Age;
    }
    case 0xd174b6bc:   /* etag */
    {
      return ETag;
    }
    case 0x5e9e89cb:   /* location */
    {
      return Location;
    }
    case 0xa56b5572:   /* proxy-authenticate */
    {
      return Proxy_Authenticate;
    }
    case 0x3499ab92:   /* retry-after */
    {
      return Retry_After;
    }
    case 0x5a6dd5f6:   /* server */
    {
      return Server;
    }
    case 0x12d553a7:   /* vary */
    {
      return Vary;
    }
    case 0x5d0c2c5a:   /* www-authenticate */
    {
      return WWW_Authenticate;
    }
    case 0x7e4940e8:   /* allow */
    {
      return Allow;
    }
    case 0xe4aaf8f3:   /* content-encoding */
    {
      return Content_Encoding;
    }
    case 0x58397009:   /* content-language */
    {
      return Content_Language;
    }
    case 0x12bc2f1c:   /* content-length */
    {
      return Content_Length;
    }
    case 0xd27c8877:   /* content-location */
    {
      return Content_Location;
    }
    case 0x9cb1d503:   /* content-md5 */
    {
      return Content_MD5;
    }
    case 0x3eaea251:   /* content-range */
    {
      return Content_Range;
    }
    case 0xc2ae0943:   /* content-type */
    {
      return Content_Type;
    }
    case 0x9a9c688c:   /* expires */
    {
      return Expires;
    }
    case 0xf95fc261:   /* last-modified */
    {
      return Last_Modified;
    }
    default:
      return -1;
  }
}


/*******************************************************************************
Function: getstatus
Purpose: Gets the response code if a response packet.
Updated: 13.12.2018
*******************************************************************************/
stringptr projectwebdocument::getreasonphrase( void )
{
  if( METHODUNKNOWN == this->method )
  {
    this->parsersline();
  }

  return this->reasonphrase.substr();
}

/*******************************************************************************
Function: getstatus
Purpose: Gets the request URI.
Updated: 13.12.2018
*******************************************************************************/
stringptr projectwebdocument::getrequesturi( void )
{
  if( METHODUNKNOWN == this->method )
  {
    this->parsersline();
  }

  return this->statuscodestr.substr();
}

/*******************************************************************************
Function: projectsippacket
Purpose: Get the body of the packet
Updated: 13.12.2018
*******************************************************************************/
stringptr projectwebdocument::getbody( void )
{
  if( METHODUNKNOWN == this->method )
  {
    this->parsersline();
  }

  if( false == this->headersparsed )
  {
    this->parseheaders();
  }

  return this->body.substr();
}

/*******************************************************************************
Function: getstatuscode
Purpose: Gets the response code if a response packet.
Updated: 13.12.2018
*******************************************************************************/
int projectwebdocument::getstatuscode( void )
{
  if( METHODUNKNOWN == this->method )
  {
    this->parsersline();
  }

  return this->statuscode;
}

/*******************************************************************************
Function: getmethod
Purpose: Get the method of the packet
Updated: 13.12.2018
*******************************************************************************/
int projectwebdocument::getmethod( void )
{
  if( METHODUNKNOWN == this->method )
  {
    this->parsersline();
  }

  return this->method;
}

/*******************************************************************************
Function: hasheader
Purpose: Checks if we have a header
Updated: 17.12.2018
*******************************************************************************/
bool projectwebdocument::hasheader( int header )
{
  return this->headers[ header ].end() != 0;
}

/*******************************************************************************
Function: getheader
Purpose: get the header
Updated: 12.12.2018
*******************************************************************************/
substring projectwebdocument::getheader( int header )
{
  if( METHODUNKNOWN == this->method )
  {
    this->parsersline();
  }

  if( false == this->headersparsed )
  {
    this->parseheaders();
  }

  if( header >= 0 && header <= MAX_HEADERS )
  {
    return this->headers[ header ].substr();
  }

  return stringptr( new std::string() );
}

/*******************************************************************************
Function: getheadervalue
Purpose: From the position of the header get the positions of the header value.
Updated: 12.12.2018
*******************************************************************************/
substring projectwebdocument::getheadervalue( substring header )
{
  std::string::iterator it;

  substring value( this->document, header.end(), header.end() );

  for( it = moveonbyn( this->document, value.start() ); it != this->document->end(); it++ )
  {
    switch( *it )
    {
      case ':':
      case ' ':
      {
        value.start( value.start() + 1 );
        value++;
        continue;
      }
    }
    break;
  }

  for( ; it != this->document->end(); it++ )
  {
    if( '\r' == *it )
    {
      it++;
      if( it == this->document->end() ) goto exit_loop;
      if( '\n' == *it )
      {
        it++;
        if( it == this->document->end() ) goto exit_loop;
        switch( *it )
        {
          case ' ':
          case '\t':
          {
            break;
          }
          default:
          {
            goto exit_loop;
          }
        }
        value++;
      }
      value++;
    }

    value++;
  }
  exit_loop:

  return value;
}

/*******************************************************************************
Function: storeheader
Purpose: Stores the header subsring in our table. Allows for
MAX_DUPLICATEHEADERS duplicates.
Updated: 12.12.2018
*******************************************************************************/
void projectwebdocument::storeheader( int headerindex, substring hval )
{
  for( int i = 0; i < MAX_DUPLICATEHEADERS; i++ )
  {
    if( 0 == this->headers[ headerindex + ( MAX_HEADERS * i ) ].end() )
    {
      this->headers[ headerindex + ( MAX_HEADERS * i ) ] = hval;
      break;
    }
  }
}

/*******************************************************************************
Function: parsersline
Purpose: Parse the Request/Respose line:
    Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    Request-Line = Method SP Request-URI SP HTTP-Version CRLF
Updated: 02.02.2019
*******************************************************************************/
void projectwebdocument::parsersline( void )
{
  size_t verpos = this->document->find( this->getversion() );

  if( std::string::npos == verpos )
  {
    /* Bad */
    return;
  }

  size_t sp1 = this->document->find( ' ' );
  if( std::string::npos == sp1 )
  {
    return;
  }
  size_t sp2 = this->document->find( ' ', sp1 + 1 );
  if( std::string::npos == sp2 )
  {
    return;
  }

  size_t endline = this->document->find( "\r\n" );
  if( std::string::npos == endline || sp2 > endline )
  {
    return;
  }

  this->rsline = substring( this->document, 0, endline );

  if( 0 == verpos )
  {
    this->statuscodestr = substring( this->document, sp1 + 1, sp2 );
    this->reasonphrase = substring( this->document, sp2, endline );

    /* Status */
    try
    {
      this->statuscode = boost::lexical_cast<int>( *( this->statuscodestr.substr() ) );
    }
    catch( const boost::bad_lexical_cast& )
    {
      /* TODO - change to some tag */
      this->statuscode = -1;
    }
  }
  else
  {
    /* Request */
    this->methodstr = substring( this->document, 0, sp1 );

    std::string::iterator crcit = this->document->begin();
    boost::crc_32_type crccheck;

    for( size_t i = 0; i < sp1; i ++ )
    {
      crccheck.process_byte( std::tolower( *crcit ) );
      crcit++;
    }

    this->method = this->getmethodfromcrc( crccheck.checksum() );
    this->uri = substring( this->document, sp1, sp2 );
  }
}

/*******************************************************************************
Function: parseheaders
Purpose: Parse the SIP packet for headers and store in our structure for common
headers we are going to be interested in. Warning, this will modify the SIP
packet as it converts the header name to lower.
Updated: 12.12.2018
*******************************************************************************/
void projectwebdocument::parseheaders( void )
{
  this->headersparsed = true;
  this->headercount = 0;

  std::string::iterator it;
  substring headername( this->document, this->rsline.end() + 2, this->rsline.end() + 2 );

  boost::crc_32_type crccheck;
  substring hval;

  for( it = moveonbyn( this->document, headername.end() );
        it != this->document->end();
        it++ )
  {
    switch( *it )
    {
      case ' ':
      case ':':
      {
        if( headername.start() + headername.length() > this->document->size() )
        {
          goto exit_loop;
        }

        int headerindex =  this->getheaderfromcrc( crccheck.checksum() );
        hval = this->getheadervalue( headername );
        if( headerindex >= 0 && headerindex <= MAX_HEADERS )
        {
          this->storeheader( headerindex, hval );
        }

        it = moveonbyn( this->document, hval.end() + 1 );
        std::string::iterator itcar = moveonbyn( this->document, hval.end() + 2 );
        std::string::iterator itline = moveonbyn( this->document, hval.end() + 3 );

        if ( itcar == this->document->end() || itline == this->document->end() )
        {
          goto exit_loop;
        }

        if( '\r' == *itcar && '\n' == *itline )
        {
          goto exit_loop;
        }

        headername = substring( this->document, hval.end() + 2, hval.end() + 2 );
        crccheck.reset();
        this->headercount++;
        break;
      }
      default:
      {
        crccheck.process_byte( std::tolower( *it ) );
        headername++;
      }
    }
  }

  exit_loop:

  size_t docsize = this->document->size();
  size_t bodystartpos = hval.end() + 4;
  if( bodystartpos > docsize )
  {
    bodystartpos = docsize;
  }
  this->body.start( bodystartpos );
  this->body.end( docsize );
}

/*******************************************************************************
Function: setstatusline
Purpose: Set the status line of the message.
    HTTP RFC:
    Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

    SIP RFC:
    Status-Line  =  SIP-Version SP Status-Code SP Reason-Phrase CRLF
Updated: 02.01.2019
*******************************************************************************/
void projectwebdocument::setstatusline( int code, std::string reason )
{
  this->statuscode = code;

  std::string newline;
  newline.reserve( DEFAULTHEADERLINELENGTH );

  newline = this->getversion();
  newline += " ";

  if( code < 100 )
  {
    newline += '0';
  }
  if( code < 10 )
  {
    newline += '0';
  }

  this->statuscodestr.start( newline.size() );
  newline += std::to_string( code );
  this->statuscodestr.end( newline.size() );

  newline += ' ';
  this->reasonphrase.start( newline.size() );
  newline += reason;
  this->reasonphrase.end( newline.size() );
  this->rsline.end( newline.size() );

  newline += "\r\n";

  if( 0 != this->rsline.end() )
  {
    this->document->erase( 0, this->rsline.end() );
  }

  if( 0 == this->document->length() )
  {
    ( *this->document ) = newline;
  }
  else
  {
    this->document->insert( 0, newline );
  }
}


/*******************************************************************************
Function: setrequestline
Purpose: Sets the status line.
    HTTP RFC:
    Request-Line   = Method SP Request-URI SP HTTP-Version CRLF

    SIP RFC:
    Request-Line  =  Method SP Request-URI SP SIP-Version CRLF
Updated: 02.01.2019
*******************************************************************************/
void projectwebdocument::setrequestline( int method, std::string uri )
{
  this->method = method;

  std::string newline;
  newline.reserve( DEFAULTHEADERLINELENGTH );

  newline = this->getmethodstr( method );
  this->methodstr.start( newline.size() );
  newline += ' ';
  this->uri.start( newline.size() );
  newline += uri;
  this->uri.end( newline.size() );

  newline += ' ';
  newline += this->getversion();
  this->rsline.end( newline.size() );

  newline += "\r\n";

  if( 0 != this->rsline.end() )
  {
    this->document->erase( 0, this->rsline.end() );
  }

  if( 0 == this->document->length() )
  {
    ( *this->document ) = newline;
  }
  else
  {
    this->document->insert( 0, newline );
  }
}

/*******************************************************************************
Function: addheader
Purpose: Adds a header to the message.
Updated: 02.01.2019
*******************************************************************************/
void projectwebdocument::addheader( int header, std::string value )
{
  std::string completeheader;
  completeheader.reserve( DEFAULTHEADERLINELENGTH );
  completeheader = this->getheaderstr( header );
  completeheader += ": " + value + "\r\n";

  if( 0 == this->body.end() )
  {
    if( this->headercount > 0 )
    {
      this->document->erase( this->document->size() - 2, 2 );
    }

    /* We can just append */
    size_t headerstart = this->document->size();
    *this->document += completeheader;
    this->storeheader( header, substring( this->document, headerstart, this->document->size() - 2 ) );
    *this->document += "\r\n";
  }
  else
  {
    if( 0 == this->headercount )
    {
      this->document->insert( this->body.start(), completeheader + "\r\n" );
    }
    else
    {
      this->document->insert( this->body.start() - 2, completeheader );
    }

    this->storeheader( header, substring( this->document, this->body.start(), this->body.start() + completeheader.size() - 2 ) );

    this->body = substring( this->document, 
                            this->body.start() + completeheader.size(), 
                            this->body.end() + completeheader.size() );
  }
  this->headercount++;
}

/*******************************************************************************
Function: setbody
Purpose: Adds the body of a message.
Updated: 02.01.2019
*******************************************************************************/
void projectwebdocument::setbody( const char *body )
{
  if( 0 != this->body.end() )
  {
    this->document->erase( this->body.start(), this->body.end() - this->body.start() );
  }

  this->body.start( this->document->size() );
  ( *this->document ) += body;
  this->body.end( this->document->size() );
  return;
}


/*******************************************************************************
Function: from_hex
Purpose: URL encode and decode. Thank you: http://www.geekhideout.com/urlcode.shtml
for the basis of this. Converts a hex character to its integer value.
Updated: 18.12.2018
*******************************************************************************/
static char from_hex( char ch )
{
  return isdigit( ch ) ? ch - '0' : tolower( ch ) - 'a' + 10;
}

/*******************************************************************************
Function: to_hex
Purpose: Converts an integer value to its hex character
Updated: 18.12.2018
*******************************************************************************/
static char to_hex( char code )
{
  static char hex[] = "0123456789abcdef";
  return hex[ code & 15 ];
}

/*******************************************************************************
Function: urlencode
Purpose: Returns a url-encoded version of str.
Updated: 18.12.2018
*******************************************************************************/
stringptr urlencode( stringptr str )
{
  stringptr encoded( new std::string() );
  encoded->reserve( ( str->size() * 3 ) + 1);

  std::string::iterator it = str->begin();

  while( it != str->end() )
  {
    char c = *it;
    if ( isalnum( c ) || c == '-' || c == '_' || c == '.' || c == '~' )
    {
      *encoded += c;
    }
    else if ( ' ' == c )
    {
      *encoded += '+';
    }
    else
    {
      *encoded += '%';
      *encoded += to_hex( c >> 4 );
      *encoded += to_hex( c & 15 );
    }

    it++;
  }
  return encoded;
}

/*******************************************************************************
Function: urldecode
Purpose: Returns a url-decoded version of str.
Updated: 18.12.2018
*******************************************************************************/
stringptr urldecode( stringptr str )
{
  stringptr decoded( new std::string() );
  decoded->reserve( str->size() + 1 );

  std::string::iterator it = str->begin();

  while( it != str->end() )
  {
    if( '%' == *it )
    {
      std::string::iterator digit1 = it + 1;
      if( digit1 != str->end() )
      {
        std::string::iterator digit2 = digit1 + 1;

        if( digit1 != str->end() )
        {
          *decoded += from_hex( *digit1 ) << 4 | from_hex( *digit2 );
          moveonbyn( str, it, 3 );
          continue;
        }
      }
    }
    else if ( '+' == *it )
    {
      *decoded += ' ';
    }
    else
    {
      *decoded += *it;
    }
    it++;
  }

  return decoded;
}



/*******************************************************************************
Function: operator != substring const char *
Purpose: compare a substring with a const char*
Updated: 30.12.2018
*******************************************************************************/
bool operator!=( const substring& lhs, const char *rhs )
{ 
  const char *c = rhs;
  size_t index = lhs.startpos;

  for( std::string::iterator it = moveonbyn( lhs.s, lhs.startpos );
        it != lhs.s->end();
        it ++ )
  {
    if( index == lhs.endpos )
    {
      break;
    }

    if( 0 == *c )
    {
      /* Found the end too soon */
      return true;
    }

    if( *it != *c )
    {
      return false;
    }

    index++;
    c++;
  }

  if( 0 == *c )
  {
    return false;
  }

  return true;
}

/*******************************************************************************
Function: operator == substring const char *
Purpose: compare a substring with a const char*
Updated: 30.12.2018
*******************************************************************************/
bool operator==( const substring& lhs, const char *rhs )
{
  const char *c = rhs;
  size_t index = lhs.startpos;

  for( std::string::iterator it = moveonbyn( lhs.s, lhs.startpos );
        it != lhs.s->end();
        it ++ )
  {
    if( index == lhs.endpos )
    {
      break;
    }

    if( *it != *c )
    {
      return false;
    }

    index++;
    c++;
  }

  if( 0 != *c )
  {
    return false;
  }

  return true;
}



