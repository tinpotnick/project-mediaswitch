
#include <string>
#include <algorithm>
#include <boost/crc.hpp>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "projectsippacket.h"


/*******************************************************************************
Function: projectsippacket constructor
Purpose:
Updated: 12.12.2018
*******************************************************************************/
projectsippacket::projectsippacket( stringptr pk )
  : projectwebdocument( pk )
{

}

/*******************************************************************************
Function: projectsippacket destructor
Purpose:
Updated: 16.12.2018
*******************************************************************************/
projectsippacket::~projectsippacket()
{
}

/*******************************************************************************
Function: parsemethod
Purpose: Parse the SIP packet for the method
Updated: 13.12.2018
*******************************************************************************/
void projectsippacket::parsemethod( void )
{
  std::string::iterator it;
  int counter = 0;

  if( this->document->size() < 9 )
  {
    this->method = METHODBADFORMAT;
    return;
  }

  it = this->document->begin();

  switch( std::tolower( (*this->document)[ 0 ] ) )
  {
    case 'r':
    {
      this->method = REGISTER;
      it += 8;
      counter += 8;
      break;
    }
    case 'i':
    {
      this->method = INVITE;
      it += 6;
      counter += 6;
      break;
    }
    case 'a':
    {
      this->method = ACK;
      it += 3;
      counter += 3;
      break;
    }
    case 'o':
    {
      this->method = OPTIONS;
      it += 7;
      counter += 7;
      break;
    }
    case 'c':
    {
      this->method = CANCEL;
      it += 6;
      counter += 6;
      break;
    }
    case 'b':
    {
      this->method = BYE;
      it += 3;
      counter += 3;
      break;
    }
    case 's':
    {
      this->method = RESPONCE;
      it += 7;
      counter += 7;
      break;
    }
  }

  int spacesfound = 0;
  for ( ; it != this->document->end(); it++ )
  {
    counter++;
    switch( *it )
    {
      case '\r':
      {
        std::string::iterator itline = it + 1;
        if( this->document->end() == itline )
        {
          goto exit_loop;
        }
        if( '\n' == * itline )
        {
          goto exit_loop;
        }
        break;
      }
      case ' ':
      {
        spacesfound++;
        switch( spacesfound )
        {
          case 1:
          {
            this->statuscodestr.start( counter );
            break;
          }
          case 2:
          {
            this->reasonphrase.start( counter );
            this->statuscodestr.end( counter - 1 );
            break;
          }
        }

        break;
      }
    }
  }
  exit_loop:

  this->reasonphrase.end( counter - 1 );

  if( RESPONCE == this->method )
  {
    try
    {
      this->statuscode = boost::lexical_cast<int>( *( this->statuscodestr.substr() ) );
    }
    catch( const boost::bad_lexical_cast& )
    {
      this->method = METHODBADFORMAT;
    }
  }

  this->methodstr = substring( this->document, 0, counter - 1 );
}

/*******************************************************************************
Function: getheaderfromcrc
Purpose: Converts crc to header value. Switch statement comes from
gensipheadercrc.py. We only store references to the supported headers.
Updated: 12.12.2018
*******************************************************************************/
int projectsippacket::getheaderfromcrc( int crc )
{
  switch( crc )
  {
    case 0x7a6d8bef:   /* authorization */
    {
      return Authorization;
    }
    case 0x7dd2712:   /* call-id */
    {
      return Call_ID;
    }
    case 0x12bc2f1c:   /* content-length */
    {
      return Content_Length;
    }
    case 0x61e88fb0:   /* cseq */
    {
      return CSeq;
    }
    case 0x4c62e638:   /* contact */
    {
      return Contact;
    }
    case 0xc2ae0943:   /* content-type */
    {
      return Content_Type;
    }
    case 0x9a9c688c:   /* expires */
    {
      return Expires;
    }
    case 0xb91aa170:   /* from */
    {
      return From;
    }
    case 0xb042e5b1:   /* max-forwards */
    {
      return Max_Forwards;
    }
    case 0xa56b5572:   /* proxy-authenticate */
    {
      return Proxy_Authenticate;
    }
    case 0x8c1d732:   /* proxy-authorization */
    {
      return Proxy_Authorization;
    }
    case 0xc5752def:   /* record-route */
    {
      return Record_Route;
    }
    case 0x2c42079:   /* route */
    {
      return Route;
    }
    case 0x3499ab92:   /* retry-after */
    {
      return Retry_After;
    }
    case 0xf724db04:   /* supported */
    {
      return Supported;
    }
    case 0xd787d2c4:   /* to */
    {
      return To;
    }
    case 0x21b74cd0:   /* via */
    {
      return Via;
    }
    case 0x82a3cb0f:   /* user-agent */
    {
      return User_Agent;
    }
    case 0x5d0c2c5a:   /* www-authenticate */
    {
      return WWW_Authenticate;
    }
    default:
    {
      return -1;
    }
  }
  return -1;
}







