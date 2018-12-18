

#include <string>
#include <algorithm>
#include <boost/crc.hpp>
#include <iostream>

#include "projectwebdocument.h"


/*******************************************************************************
Function: projectwebdocument
Purpose: Constructor
Updated: 12.12.2018
*******************************************************************************/
projectwebdocument::projectwebdocument( stringptr doc ) :
  document( doc ),
  body( doc ),
  headersparsed( false ),
  method( METHODUNKNOWN ),
  statuscode( STATUSUNKNOWN ),
  methodstr( doc ),
  statuscodestr( doc ),
  reasonphrase( doc )
{
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
    this->parsemethod();
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
    this->parsemethod();
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
    this->parsemethod();
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
    this->parsemethod();
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
    this->parsemethod();
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
    this->parsemethod();
  }

  if( false == this->headersparsed )
  {
    this->parseheaders();
  }

  if( header >= 0 && header <= MAX_HEADERS )
  {
    return this->headers[ header ].substr();
  }

  return stringptr( new std::string( "" ) );
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
  for( int i = 1; i <= MAX_DUPLICATEHEADERS; i++ )
  {
    if( 0 == this->headers[ headerindex * i ].end() )
    {
      this->headers[ headerindex * i ] = hval;
    }
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
  std::string::iterator it;
  substring headername( this->document, this->methodstr.end() + 2, this->methodstr.end() + 2 );

  boost::crc_32_type crccheck;
  substring hval;

  for( it = moveonbyn( this->document, headername.end() );
        it != this->document->end();
        it++ )
  {
    *it = std::tolower( *it );

    switch( *it )
    {
      case ' ':
      case ':':
      {
        break;
      }
      default:
      {
        headername++;
      }
    }

    if( ':' == *it )
    {
      if( headername.start() + headername.length() > this->document->size() )
      {
        goto exit_loop;
      }

      crccheck.process_bytes(
        this->document->c_str() + headername.start(),
        headername.length()
        );

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
Function: from_hex
Purpose: URL encode and decode. Thank you: http://www.geekhideout.com/urlcode.shtml
for the basis of this. Converts a hex character to its integer value.
Updated: 18.12.2018
*******************************************************************************/
static char from_hex(char ch)
{
  return isdigit( ch ) ? ch - '0' : tolower( ch ) - 'a' + 10;
}

/*******************************************************************************
Function: to_hex
Purpose: Converts an integer value to its hex character
Updated: 18.12.2018
*******************************************************************************/
static char to_hex(char code)
{
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
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



