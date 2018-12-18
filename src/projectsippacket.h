

#ifndef PROJECTSIPPACKET_H
#define PROJECTSIPPACKET_H

#include <string>

#include "projectwebdocument.h"


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
  inline sipuri( std::string &s )
  {
    /* Find display name */
    size_t dispstart = s.find( '"' );
    if( std::string::npos != dispstart )
    {
      size_t dispend = s.find( '"', dispstart + 1 );
      if( std::string::npos != dispstart )
      {
        this->displayname = substring( dispstart + 1, dispend );
      }
    }

    size_t sipstart = s.find( "sip:" );
    if( std::string::npos == sipstart )
    {
      sipstart = s.find( "sips:" );
      if( std::string::npos == sipstart )
      {
        /* Nothing more we can do */
        return;
      }
      this->protocol = substring( sipstart, sipstart + 4 );
    }
    else
    {
      this->protocol = substring( sipstart, sipstart + 3 );
    }

    /* We return to the display name where no quotes are used */
    if( 0 == this->displayname.end() )
    {
      if( sipstart > 0 )
      {
        const char *start = s.c_str();
        char *end = (char *)start + sipstart;
        char *ptr = (char *)start;
        int startpos = 0;
        int endpos = sipstart;
        for(  ; ptr < end; ptr++ )
        {
          if( ' ' != *ptr )
          {
            break;
          }
          startpos++;
        }

        for( ptr = end; ptr > start; ptr-- )
        {
          if( ' ' == *ptr )
          {
            break;
          }
          endpos--;
        }
        this->displayname = substring( startpos, endpos );
      }
    }

    size_t starthost =  this->protocol.end() + 1; /* +1 = : */
    size_t offset = starthost;
    char *hoststart = (char *)s.c_str() + offset;
    char *endstr = (char *)s.c_str() + s.size();
    for( ; hoststart < endstr; hoststart++ )
    {
      switch( *hoststart )
      {
        case '@':
        {
          if( 0 == this->user.end() )
          {
            this->user = substring( starthost, offset );
          }
          else
          {
            this->secret.end( offset );
          }
          starthost = offset + 1;
          break;
        }
        case ':':
        {
          if( 0 == this->user.end() )
          {
            this->user = substring( starthost, offset );
          }
          this->secret = substring( offset + 1, offset );
          starthost = offset + 1;
          break;
        }
        case '>':
        {
          this->host = substring( starthost, offset );
          break;
        }
        case ';':
        {
          if( 0 == this->host.end() )
          {
            this->host = substring( starthost, offset );
          }
          if( 0 == this->parameters.start() )
          {
            this->parameters = substring( offset + 1, s.size() );
          }

          break;
        }
        case '?':
        {
          if( 0 == this->host.end() )
          {
            this->host = substring( starthost, offset );
          }

          if( 0 != this->parameters.end() )
          {
            this->parameters.end( offset );
          }
          this->headers = substring( offset + 1, s.size() );
          break;
        }
      }

      offset++;
    }

    if( 0 == this->host.end() )
    {
      this->host = substring( starthost, s.size() );
    }
  }

  /*****************************************************************************
  Funtion: getparameter
  Purpose: Returns the substring index into s for the given param name.
  std::string s = From: sip:+12125551212@server.phone2net.com;tag=887s
  getparameter( s, "tag" );
  Will return a substring index to '887s'.
  Updated: 18.12.2018
  *****************************************************************************/
  inline substring getparameter( std::string &s, std::string name )
  {
    if( 0 == this->parameters.end() )
    {
      return this->parameters;
    }

    size_t startpos = s.find( name + '=', this->parameters.start() );

    if( std::string::npos == startpos )
    {
      return substring( 0, 0 );
    }

    size_t endpos = s.find( ';', startpos + name.size() + 1 );

    if( std::string::npos == endpos )
    {
      return substring( startpos + name.size() + 1, s.size() );
    }
    return substring( startpos + name.size() + 1, endpos );
  }

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
Our purpose is to be effieicent (fast), we are not checking for perfect syntax
this object should still work even with inperfect SIP packets. Keep memory
allocation to a minimum. Work off just the main packet string and keep indexes
into that packet.
Updated: 12.12.2018
*******************************************************************************/
class projectsippacket : public projectwebdocument
{

public:
  projectsippacket( std::string packet );
  ~projectsippacket();


  /*
    Request-Line  =  Method SP Request-URI SP SIP-Version CRLF
    RESPONCE indicates it found a status code - so is a responce
    not a request.
  */
  enum { REGISTER, INVITE, ACK, CANCEL, BYE, OPTIONS, RESPONCE };

  enum { Authorization,
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

  virtual void parsemethod( void );
  virtual int getheaderfromcrc( int crc );
  substring getheadervalue( substring header );

  substring headers[ WWW_Authenticate + 1 ];
  substring body;

};


#endif /* PROJECTSIPPACKET_H */

