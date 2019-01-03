
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

projectsippacket::projectsippacket()
  : projectwebdocument()
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
Function: projectsippacket destructor
Purpose:
Updated: 16.12.2018
*******************************************************************************/
const char* projectsippacket::getversion( void )
{
  return "SIP/2.0";
}

/*******************************************************************************
Function: getmethodfromcrc
Purpose: Converts crc to header value. Switch statement comes from
gensipheadercrc.py. We only store references to the supported headers.
Updated: 12.12.2018
*******************************************************************************/
int projectsippacket::getmethodfromcrc( int crc )
{
  switch( crc )
  {
    case 0x5ff94014:   /* register */
    {
      return REGISTER;
    }
    case 0xc7e210d7:   /* invite */
    {
      return INVITE;
    }
    case 0x22e4f8b1:   /* ack */
    {
      return ACK;
    }
    case 0xd035fa87:   /* options */
    {
      return OPTIONS;
    }
    case 0x5616c572:   /* cancel */
    {
      return CANCEL;
    }
    case 0x77379134:   /* bye */
    {
      return BYE;
    }
  }
  return -1;
}

/*******************************************************************************
Function: projectsippacket getheaderstr
Purpose: Convert a header id to a string.
Updated: 02.01.2019
*******************************************************************************/
const char *projectsippacket::getmethodstr( int method )
{
  switch( method )
  {
    case REGISTER:
      return "REGISTER";
    case INVITE:
      return "INVITE";
    case ACK:
      return "ACK";
    case OPTIONS:
      return "OPTIONS";
    case CANCEL:
      return "CANCEL";
    case BYE:
      return "BYE";
    default:
      return "";
  }
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

/*******************************************************************************
Function: projectsippacket getheaderstr
Purpose: Convert a header id to a string.
Updated: 02.01.2019
*******************************************************************************/
const char *projectsippacket::getheaderstr( int header )
{
  switch( header )
  {
    case Authorization:
      return "Authorization";
    case Call_ID:
      return "Call-ID";
    case Content_Length:
      return "Content-Length";
    case CSeq:
      return "CSeq";
    case Contact:
      return "Contact";
    case Content_Type:
      return "Content-Type";
    case Expires:
      return "Expires";
    case From:
      return "From";
    case Max_Forwards:
      return "Max-Forwards";
    case Proxy_Authenticate:
      return "Proxy-Authenticate";
    case Proxy_Authorization:
      return "Proxy-Authorization";
    case Record_Route:
      return "Record-Route";
    case Route:
      return "Route";
    case Retry_After:
      return "Retry-After";
    case Supported:
      return "Supported";
    case To:
      return "To";
    case Via:
      return "Via";
    case User_Agent:
      return "User-Agent";
    case WWW_Authenticate:
      return "WWW-Authenticate";
    default:
      return "";
  }
}
