
#include <string>
#include <algorithm>
#include <boost/crc.hpp>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <openssl/md5.h>

/* Debuggng */
#include <iostream>

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
Function: contact
Purpose: Generate a contact parameter.
Updated: 22.01.2019
*******************************************************************************/
stringptr projectsippacket::contact( stringptr user, stringptr host, int expires, int port )
{
  stringptr s = stringptr( new std::string() );
  s->reserve( DEFAULTHEADERLINELENGTH );

  *s = "<sip:";
  *s += *user;
  *s += '@';
  *s += *host;

  if( 5060 != port )
  {
    *s += std::to_string( port );
  }

  *s += '>';
  if( 0 != expires )
  {
    *s += ";expires=";
    *s += std::to_string( expires );
  }

  return s;
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
Function: branch
Purpose: Generate a new call id.
Updated: 17.01.2019
*******************************************************************************/
stringptr projectsippacket::callid()
{
  boost::uuids::basic_random_generator<boost::mt19937> gen;
  boost::uuids::uuid u = gen();

  stringptr s = stringptr( new std::string() );

  s->reserve( DEFAULTHEADERLINELENGTH );

  try
  {
    *s = boost::lexical_cast< std::string >( u );
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
  retval.start( h.end() );

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
    searchfor[ 0 ] = ',';
    ppos = h.rfind( searchfor );
    if( 0 == ppos.end() )
    {
      ppos = h.rfind( &searchfor[ 1 ] );
      if( 0 == ppos.end() )
      {
        return substring( document, 0, 0 );
      }
    }
  }
  retval.start( ppos.end() );

  /* Check for a quoted string TODO - these are case sensative 
  unquoted strings should be insensative */
  substring pposquoted = ppos;
  pposquoted.end( h.end() );
  pposquoted = pposquoted.findsubstr( '"', '"' );

  if( pposquoted.start() == ppos.start() + l + 3 )
  {
    return pposquoted;
  }

  retval = retval.mvend_first_of( ";\r," );

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
  s += suri.host.str();
  
  s += "\",algorithm=\"MD5\",nonce=\"";

  try
  {
    s += boost::lexical_cast< std::string >( gen() );;
  }
  catch( boost::bad_lexical_cast &e )
  {
    // This shouldn't happen
  }
  s += "\",opaque=\"";

  try
  {
    s += boost::lexical_cast< std::string >( gen() );
  }
  catch( boost::bad_lexical_cast &e )
  {
    // This shouldn't happen
  }
  s += "\",qop=\"auth\"";
  
  this->addheader( projectsippacket::WWW_Authenticate, s.c_str() );
  return true;
}

/*******************************************************************************
Function: md5hashtostring
Purpose: Converts input 16 bytes hex to string hex. buf need to be 33 bytes to 
include null terminator.
Updated: 08.01.2019
*******************************************************************************/
static inline void md5hashtostring( char* buf )
{
  char *in = buf + 15;
  buf += 31;

  for( int i = 0; i < 16; i++ )
  {
    char c = *in;
    *buf = tohex( c );
    buf--;
    *buf = tohex( c >> 4 );
    buf--;
    in--;
  }
  buf[ 33 ] = 0;
}

/*******************************************************************************
Function: ha1
Purpose: Calculate the h(a1) using cnonce and cnonce (algorithm = "MD5-sess").
Ref RFC 2617.
Updated: 11.01.2019
*******************************************************************************/
static inline char * ha1( const char *username, size_t ul, 
                          const char *realm, size_t rl, 
                          const char *password, size_t pl,
                          const char *nonce, size_t cl, 
                          const char *cnonce, size_t cnl, 
                          const char *alg,
                          char *buf )
{
  MD5_CTX context;
  MD5_Init( &context);
  MD5_Update( &context, username, ul );
  MD5_Update( &context, ":", 1 );
  MD5_Update( &context, realm, rl );
  MD5_Update( &context, ":", 1 );
  MD5_Update( &context, password, pl );
  MD5_Final( ( unsigned char* ) buf, &context );

  if ( strcasecmp( alg, "md5-sess" ) == 0 )
  {
    MD5_Init( &context );
    MD5_Update( &context, buf, 16 );
    MD5_Update( &context, ":", 1 );
    MD5_Update( &context, nonce, cl );
    MD5_Update( &context, ":", 1 );
    MD5_Update( &context, cnonce, cnl );
    MD5_Final( ( unsigned char* ) buf, &context );
  }

  md5hashtostring( buf );

  return buf;
}

/*******************************************************************************
Function: ha2
Purpose: Calculate the h(a2). Ref RFC 2617.
Updated: 11.01.2019
*******************************************************************************/
static inline char * ha2( const char *method, size_t ml, 
                                    const char *uri, size_t ul,
                                    char *buf )
{
  MD5_CTX context;
  MD5_Init( &context);
  MD5_Update( &context, method, ml );
  MD5_Update( &context, ":", 1 );
  MD5_Update( &context, uri, ul );
  MD5_Final( ( unsigned char* ) buf, &context );

  md5hashtostring( buf );

  return buf;
}

/*******************************************************************************
Function: kd
Purpose: Calculate the kd. Ref RFC 2617.
Updated: 11.01.2019
*******************************************************************************/
static inline char *kd( const char *ha1,
                                  const char *nonce, size_t nl,
                                  const char *nc, size_t ncl,
                                  const char *cnonce, size_t cl,
                                  const char *qop, size_t ql,
                                  const char *ha2,
                                  char * buf )
{
  MD5_CTX context;
  MD5_Init( &context );
  MD5_Update( &context, ha1, 32 );
  MD5_Update( &context, ":", 1 );
  MD5_Update( &context, nonce, nl );
  MD5_Update( &context, ":", 1 );

  if ( *qop )
  {
    MD5_Update( &context, nc, ncl );
    MD5_Update( &context, ":", 1 );
    MD5_Update( &context, cnonce, cl );
    MD5_Update( &context, ":", 1);
    MD5_Update( &context, qop, ql );
    MD5_Update( &context, ":", 1 );
  }

  MD5_Update( &context, ha2, 32 );
  MD5_Final( ( unsigned char* ) buf, &context );
  md5hashtostring( buf );

  return buf;
}
#if 1
/*******************************************************************************
Function: requestdigest
Purpose: The 1 call to calculate the SIP request digest.
Updated: 11.01.2019
*******************************************************************************/
char * requestdigest( const char *username, size_t ul, 
                      const char *realm, size_t rl, 
                      const char *password, size_t pl,
                      const char *nonce, size_t nl, 
                      const char *nc, size_t ncl, 
                      const char *cnonce, size_t cnl,
                      const char *method, size_t ml, 
                      const char *uri, size_t url,
                      const char *qop, size_t ql,
                      const char *alg,
                      char *buf )
{
  char h1[ 33 ];
  char h2[ 33 ];

  kd( 
    ha1( username, ul, 
         realm, rl, 
         password, pl,
         nonce, nl, 
         cnonce, cnl,
         alg,
         h1 ),
    nonce, nl,
    nc, ncl,
    cnonce, cnl,
    qop, ql,
    ha2( method, ml, 
         uri, url,
         h2 ),
    buf );

  return buf;
}
#endif

/*******************************************************************************
Function: checkauth
Purpose: Check the auth of this packet. The reference contans the nonce to check
we set the nonce and it is not a replay. 
Updated: 10.01.2019
*******************************************************************************/
bool projectsippacket::checkauth( projectsippacket *ref, stringptr password )
{
  char h1[ 33 ];
  char h2[ 33 ];
  char response[ 33 ];

  substring requestopaque = ref->getheaderparam( projectsippacket::WWW_Authenticate, "opaque" );
  substring receivedopaque = this->getheaderparam( projectsippacket::Authorization, "opaque" );
  if( requestopaque != receivedopaque )
  {
    return false;
  }

  substring cnonce = this->getheaderparam( projectsippacket::Authorization, "cnonce" );
  substring noncecount = this->getheaderparam( projectsippacket::Authorization, "nc" );
  substring qop = this->getheaderparam( projectsippacket::Authorization, "qop" );
  substring user = this->getuser();

  substring realm = this->getheaderparam( projectsippacket::Authorization, "realm" );
  if( 0 == realm.end() )
  {
    realm = this->geturihost();
  }
  
  substring nonce = ref->getheaderparam( projectsippacket::WWW_Authenticate, "nonce" );
  substring uri = this->getheaderparam( projectsippacket::Authorization, "uri" );
  if( 0 == uri.end() )
  {
    uri = this->uri;
  }

  kd( 
    ha1( user.c_str(), user.length(), 
         realm.c_str(), realm.length(), 
         password->c_str(), password->length(),
         nonce.c_str(), nonce.length(), 
         cnonce.c_str(), cnonce.length(),
         "MD5",
         h1 ),
    nonce.c_str(), nonce.length(),
    noncecount.c_str(), noncecount.length(),
    cnonce.c_str(), cnonce.length(),
    qop.c_str(), qop.length(),
    ha2( this->methodstr.c_str(), this->methodstr.length(), 
         uri.c_str(), uri.length(),
         h2 ),
    response );

  substring cresponse = this->getheaderparam( projectsippacket::Authorization, "response" );

  if( cresponse == response )
  {
    return true;
  }
  /* we now know what our response should be */
  return false;
}

/*******************************************************************************
Function: gettouser
Purpose: Get the user addressed in the To header.
Updated: 15.01.2019
*******************************************************************************/
substring projectsippacket::getuser( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).user;
}

/*******************************************************************************
Function: gettohost
Purpose: Get the host addressed in the To header.
Updated: 17.01.2019
*******************************************************************************/
substring projectsippacket::gethost( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).host;
}

/*******************************************************************************
Function: getdisplayname
Purpose: Returns the diaply name.
Updated: 29.01.2019
*******************************************************************************/
substring projectsippacket::getdisplayname( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).displayname;
}

/*******************************************************************************
Function: getuserhost
Purpose: Get the user@host addressed in the either To header and or URI
based on request/response.
Updated: 17.01.2019
*******************************************************************************/
std::string projectsippacket::getuserhost( int tofrom )
{
  std::string s;
  s.reserve( DEFAULTHEADERLINELENGTH );
  s = this->getuser( tofrom ).str();
  s += '@';

  if( this->isrequest() )
  {
    s += this->geturihost().str();
  }
  else
  {
    s += this->gethost().str();
  }
  return s;
}


/*******************************************************************************
Function: geturihost
Purpose: Get the host part from the uri addressed in the To header.
Updated: 15.01.2019
*******************************************************************************/
substring projectsippacket::geturihost( void )
{
  return sipuri( this->uri ).host;
}

/*******************************************************************************
Function: getexpires
Purpose: Get the expires value from the different placesit might be.
Updated: 17.01.2019
*******************************************************************************/
int projectsippacket::getexpires( void )
{
  int expires = -1;
  substring ex;
  if( this->hasheader( projectsippacket::Expires ) )
  {
    ex = this->getheader( projectsippacket::Expires );
  }
  else
  {
    ex = this->getheaderparam( projectsippacket::Contact, "expires" );
  }

  if( 0 != ex.end() )
  {
    expires = ex.toint();
  }

  return expires;
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
    case 0x2c17df2a:   /* min-expires */
    {
      return Min_Expires;
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
    case Min_Expires:
      return "Min-Expires";
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
sipuri::sipuri( substring s ) :
   s( s ),
   displayname( s ),
   protocol( s ),
   user( s ),
   secret( s ),
   host( s ),
   parameters( s ),
   headers( s )
{
  /* Find display name */
  this->displayname = s.findsubstr( '"', '"' );

  this->protocol = s.find( "sip:" );
  if( 0 == this->protocol.end() )
  {
    this->protocol = s.find( "sips:" );
    if( 0 == this->protocol.end() )
    {
      /* Nothing more we can do */
      return;
    }
  }
  this->protocol--; /* remove the : */

  substring sipuserhost = s.findsubstr( '<', '>' );
  if( 0 == sipuserhost.end() )
  {
    sipuserhost.end( s.end() );
  }

  substring userhost = sipuserhost;
  userhost.start( this->protocol.end() + 1 );

  this->host = userhost.aftertoken( '@' );
  if( 0 == this->host.end() )
  {
    this->host = userhost;
  }
  else
  {
    substring possiblehost = userhost.findsubstr( '@', ':' );
    if( 0 != possiblehost.end() )
    {
      this->host = possiblehost;
    }

    substring userpass = userhost.findend( '@' );
    this->user = userpass.findend( ':' );
    this->secret = userpass.aftertoken( ':' );
  }

  this->headers = s.findsubstr( '?', ';' );  
  this->parameters = s.findsubstr( ';', '?' );

  if( 0 != this->headers.end() )
  {
    this->host.end( std::min( this->headers.start() - 1, userhost.end() ) );
  }

  if( 0 != this->parameters.end() )
  {
    this->host.end( std::min( this->parameters.start() - 1, this->host.end() ) );
  }

  if( 0 == this->displayname.end() &&
      sipuserhost.start() > 1 )
  {
    // We can now look for an unquoted display name
    this->displayname.end( sipuserhost.start() - 2 );
    this->displayname.trim();
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

  substring param = this->parameters.aftertoken( std::string( name + '=' ).c_str() );
  if( 0 == param.end() )
  {
    return substring( this->s, 0, 0 );
  }
  return param.findend( ';' );
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

  substring header = this->headers.aftertoken( std::string( name + '=' ).c_str() );

  if( 0 == header.end() )
  {
    return substring( this->s, 0, 0 );
  }

  return header.findend( '&' );
}

