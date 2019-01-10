
#include <string>
#include <algorithm>
#include <boost/crc.hpp>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <openssl/md5.h>

#include "projectsippacket.h"


/*******************************************************************************
Function: projectsippacket constructor
Purpose:
Updated: 12.12.2018
*******************************************************************************/
projectsippacket::projectsippacket( stringptr pk )
  : projectwebdocument( pk ),
  nonce ( new std::string() )
{

}

projectsippacket::projectsippacket()
  : projectwebdocument(),
  nonce ( new std::string() )
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
Function: branch
Purpose: Generate a branch parameter.
Updated: 03.01.2019
*******************************************************************************/
stringptr projectsippacket::branch()
{
  boost::uuids::basic_random_generator<boost::mt19937> gen;
  boost::uuids::uuid u = gen();

  stringptr s = stringptr( new std::string() );

  s->reserve( DEFAULTHEADERLINELENGTH );

  try
  {
    *s = "branch=z9hG4bK" + boost::lexical_cast< std::string >( u );
  }
  catch( boost::bad_lexical_cast &e )
  {
    // This shouldn't happen
  }
  
  return s;
}

/*******************************************************************************
Function: getheaderparam
Purpose: Returns a param from the header, for example:
Via: SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1
"z9hG4bK4b43c2ff8.1" == getheaderparam( projectsippacket::Via, "branch" )
Updated: 08.01.2019
*******************************************************************************/
substring projectsippacket::getheaderparam( int header, const char *param )
{
  substring retval( document );
  substring h( this->getheader( header ) );

  if( 0 == h.end() )
  {
    return h;
  }

  size_t l = strlen( param );

  char searchfor[ DEFAULTHEADERLINELENGTH ];
  searchfor[ 0 ] = ';';
  memcpy( &searchfor[ 1 ], param, l );
  searchfor[ l + 1 ] = '=';
  searchfor[ l + 2 ] = 0;

  substring ppos = h.rfind( searchfor );

  if( 0 == ppos.end() )
  {
    return h;
  }

  h.start( ppos.start() + 1 );

  substring sepos = h.find( ';' );
  substring cepos = h.find( '\r' );
  sepos.end( std::min( sepos.end(), cepos.end() ) );

  retval.start( ppos.end() );

  if( 0 == sepos.end() )
  {
    retval.end( h.end() );
    return retval;
  }
  
  retval.end( sepos.end() - 1 );
  return retval;
}

/*******************************************************************************
Function: addviaheader
Purpose: Add a via header:
setheaderparam( "server10.biloxi.com" "z9hG4bK4b43c2ff8.1" );
Via: SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1
Updated: 09.01.2019
*******************************************************************************/
bool projectsippacket::addviaheader( const char *host, projectsippacket *ref )
{

  substring branch = ref->getheaderparam( projectsippacket::Via, "branch" );

  size_t lh = strlen( host );
  size_t lb = branch.end() - branch.start();

  if( ( lh + lb ) > DEFAULTHEADERLINELENGTH )
  {
    return false;
  }

  char paramvalue[ DEFAULTHEADERLINELENGTH ];
  memcpy( &paramvalue[ 0 ], "SIP/2.0/UDP ", 12 );
  memcpy( &paramvalue[ 12 ], host, lh );
  paramvalue[ lh + 12 ] = ';';

  if( 0 != lb )
  {
    memcpy( &paramvalue[ lh + 12 + 1 ], "branch=", 7 );
    const char *branchsrc = branch.c_str();
    memcpy( &paramvalue[ lh + 12 + 1 + 7 ], branchsrc, lb );
    paramvalue[ lh + 12 + 1 + 7 + lb ] = 0;
  }
  else
  {
    stringptr bptr = projectsippacket::branch();
    memcpy( &paramvalue[ lh + 12 + 1 ], bptr->c_str(), bptr->length() );
    paramvalue[ lh + 12 + 1 + 1 + bptr->length() ] = 0;
  }

  this->addheader( projectsippacket::Via, paramvalue );

  return true;
}

/*******************************************************************************
Function: addauthenticateheader
Purpose: Generate a nonce parameter used for auth and add the authenticate 
header. Ref RFC 2617 section 3.2.1.
Updated: 03.01.2019
*******************************************************************************/
bool projectsippacket::addwwwauthenticateheader( projectsippacket *ref )
{

  boost::uuids::basic_random_generator<boost::mt19937> gen;

  std::string s;
  s.reserve( DEFAULTHEADERLINELENGTH );

  s = "Digest realm=\"";
  sipuri suri( ref->getrequesturi() );
  s += *suri.host.substr();
  
  s += "\" algorithm=\"MD5\" nonce=\"";

  try
  {
    *( this->nonce ) = boost::lexical_cast< std::string >( gen() );
    s += *( this->nonce );
  }
  catch( boost::bad_lexical_cast &e )
  {
    // This shouldn't happen
  }
  s += "\" opaque=\"";

  try
  {
    s += boost::lexical_cast< std::string >( gen() );
  }
  catch( boost::bad_lexical_cast &e )
  {
    // This shouldn't happen
  }
  s += "\"";
  
  this->addheader( projectsippacket::WWW_Authenticate, s.c_str() );
  return true;
}

/*******************************************************************************
Function: getnonce
Purpose: Returns the nonce as a string.
Updated: 10.01.2019
*******************************************************************************/
stringptr projectsippacket::getnonce( void )
{
  return this->nonce;
}

/*******************************************************************************
Function: checkauth
Purpose: Check the auth of this packet. The reference contans the nonce to check
we set the nonce and it is not a replay. 
Updated: 10.01.2019
*******************************************************************************/
bool projectsippacket::checkauth( stringptr nonce, stringptr password )
{
#if 0
  uint32_t ha1[ 4 ];
  uint32_t ha2[ 4 ];
  uint32_t hash[ 4 ];
  std::string urp;
  urp.reserve( DEFAULTHEADERLINELENGTH );

  sipuri turi( this->getheader( projectsippacket::To ) );
  urp = *turi.user.substr();
  urp += ':';

  /* realm */
  sipuri suri( this->getrequesturi() );
  urp += *suri.host.substr();

  urp += *password;

  md5_hash( urp.c_str(), urp.length(), ha1 );
  hex_decode( ha1, );

  urp = this->getmethodstr( this->getmethod() );
  urp += ':';
  urp += this->getrequesturi().substr();

  md5_hash( urp.c_str(), urp.length(), ha2 );

  urp = tostring( ha1 );
  urp += ':';
  urp += *nonce;
  urp += tostrin( ha2 );
  md5_hash( urp.c_str(), urp.length(), hash );

  /*
    nonce = 8e4a6b9f
    HA1 = MD5("myusername:myrealm.org:password”)
    HA2 = MD5("REGISTER:sip:sip.example.com")
    response = MD5(HA1+":8e4a6b9f:"+HA2);
  */
  //stringptr s = stringptr( std::string() );
  //s->reserve( DEFAULTHEADERLINELENGTH );

  //*s = boost::md5( "message" ).hex_str_value();
#endif
  return false;
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
    case 0x7e4940e8:   /* allow */
    {
      return Allow;
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
    case Allow:
      return "Allow";
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



/*******************************************************************************
Function: sipuri constructor
Purpose: Parse a SIP URI.
Updated: 03.01.2019
*******************************************************************************/
sipuri::sipuri( stringptr s )
{
  this->s = s;

  /* Find display name */
  size_t dispstart = s->find( '"' );
  if( std::string::npos != dispstart )
  {
    size_t dispend = s->find( '"', dispstart + 1 );
    if( std::string::npos != dispstart )
    {
      this->displayname = substring( s, dispstart + 1, dispend );
    }
  }

  size_t sipstart = s->find( "sip:" );
  if( std::string::npos == sipstart )
  {
    sipstart = s->find( "sips:" );
    if( std::string::npos == sipstart )
    {
      /* Nothing more we can do */
      return;
    }
    this->protocol = substring( s, sipstart, sipstart + 4 );
  }
  else
  {
    this->protocol = substring( s, sipstart, sipstart + 3 );
  }

  /* We return to the display name where no quotes are used */
  if( 0 == this->displayname.end() )
  {
    if( sipstart > 0 )
    {
      const char *start = s->c_str();
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
      this->displayname = substring( s, startpos, endpos );
    }
  }

  size_t starthost =  this->protocol.end() + 1; /* +1 = : */
  size_t offset = starthost;
  char *hoststart = (char *)s->c_str() + offset;
  char *endstr = (char *)s->c_str() + s->size();
  for( ; hoststart < endstr; hoststart++ )
  {
    switch( *hoststart )
    {
      case '@':
      {
        if( 0 == this->user.end() )
        {
          this->user = substring( s, starthost, offset );
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
          this->user = substring( s, starthost, offset );
        }
        this->secret = substring( s, offset + 1, offset );
        starthost = offset + 1;
        break;
      }
      case '>':
      {
        this->host = substring( s, starthost, offset );
        break;
      }
      case ';':
      {
        if( 0 == this->host.end() )
        {
          this->host = substring( s, starthost, offset );
        }
        if( 0 == this->parameters.start() )
        {
          this->parameters = substring( s, offset + 1, s->size() );
        }

        break;
      }
      case '?':
      {
        if( 0 == this->host.end() )
        {
          this->host = substring( s, starthost, offset );
        }

        if( 0 != this->parameters.end() )
        {
          this->parameters.end( offset );
        }
        this->headers = substring( s, offset + 1, s->size() );
        break;
      }
    }

    offset++;
  }

  if( 0 == this->host.end() )
  {
    this->host = substring( s, starthost, s->size() );
  }
}


/*****************************************************************************
Function: getparameter
Purpose: Returns the substring index into s for the given param name.
std::string s = From: sip:+12125551212@server.phone2net.com;tag=887s
getparameter( s, "tag" );
Will return a substring index to '887s'.
Updated: 18.12.2018
*****************************************************************************/
substring sipuri::getparameter( std::string name )
{
  if( 0 == this->parameters.end() )
  {
    return this->parameters;
  }

  size_t startpos = this->s->find( name + '=', this->parameters.start() );

  if( std::string::npos == startpos )
  {
    return substring();
  }

  size_t endpos = this->s->find( ';', startpos + name.size() + 1 );

  if( std::string::npos == endpos )
  {
    endpos = this->s->find( '?', startpos + name.size() + 1 );
    if( std::string::npos == endpos )
    {
      return substring( this->s, startpos + name.size() + 1, this->s->size() );
    }
  }
  return substring( this->s, startpos + name.size() + 1, endpos );
}


/*****************************************************************************
Function: getheader
Purpose: Similar to get parameter but for headers. In a SIP URI anything after
the ? is considered a header. Name value pairs.
Updated: 18.12.2018
*****************************************************************************/
substring sipuri::getheader( std::string name )
{
  if( 0 == this->headers.end() )
  {
    return this->headers;
  }

  size_t startpos = this->s->find( name + '=', this->headers.start() );

  if( std::string::npos == startpos )
  {
    return substring();
  }
  size_t endpos = this->s->find( '&', startpos + name.size() + 1 );

  if( std::string::npos == endpos )
  {
    return substring( this->s, startpos + name.size() + 1, this->s->size() );
  }
  return substring( this->s, startpos + name.size() + 1, endpos );
}

