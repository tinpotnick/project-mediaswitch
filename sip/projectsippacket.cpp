
#include <string>
#include <algorithm>
#include <boost/crc.hpp>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <openssl/md5.h>

#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

/* Debuggng */
#include <iostream>

#include "projectsippacket.h"

/*!md
# projectsippacket constructor
*/
projectsippacket::projectsippacket( stringptr pk )
  : projectwebdocument( pk )
{

}

projectsippacket::projectsippacket()
  : projectwebdocument()
{
}

/*!md
# projectsippacket destructor
*/
projectsippacket::~projectsippacket()
{
}


/*!md
# Create
Create a pointer to a new object
*/
projectsippacket::pointer projectsippacket::create()
{
  return pointer( new projectsippacket() );
}

projectsippacket::pointer projectsippacket::create( stringptr packet )
{
  return pointer( new projectsippacket( packet ) );
}

/*!md
# contact
Generate a contact parameter.
*/
stringptr projectsippacket::contact( stringptr user, stringptr host, int expires )
{
  stringptr s = stringptr( new std::string() );
  s->reserve( DEFAULTHEADERLINELENGTH );

  *s = "<sip:";
  *s += *user;
  *s += '@';
  *s += *host;

  *s += '>';
  if( 0 != expires )
  {
    *s += ";expires=";
    *s += std::to_string( expires );
  }

  return s;
}

/*!md
# branch
Generate a branch parameter.
*/
static std::string randomcharbase(
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890" );
stringptr projectsippacket::branch( void )
{
  std::string br;
  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> index_dist( 0, randomcharbase.size() - 1 );
  for( int i = 0; i < 8; ++i )
  {
    br += randomcharbase[ index_dist( rng ) ];
  }

  stringptr s = stringptr( new std::string() );
  s->reserve( DEFAULTHEADERLINELENGTH );

  try
  {
    *s = "branch=z9hG4bK" + br;
  }
  catch( boost::bad_lexical_cast &e )
  {
    // This shouldn't happen
  }

  return s;
}

/*!md
# tag
Generate a tag.
*/
stringptr projectsippacket::tag( void )
{
  stringptr s = stringptr( new std::string() );
  s->reserve( DEFAULTHEADERLINELENGTH );

  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> index_dist( 0, randomcharbase.size() - 1 );
  for( int i = 0; i < 8; ++i )
  {
    *s += randomcharbase[ index_dist( rng ) ];
  }
  return s;
}

/*!md
# branch
Generate a new call id.
*/
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

/*!md
# getheaderparam
Returns a param from the header, for example:
Via: SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1
"z9hG4bK4b43c2ff8.1" == getheaderparam( projectsippacket::Via, "branch" )
*/
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
    searchfor[ 0 ] = ' ';
    ppos = h.rfind( searchfor );
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

/*!md
# addcommonheaders
Add our common headers.
*/
void projectsippacket::addcommonheaders( void )
{
  this->addheader( projectsippacket::User_Agent, USERAGENT );
  this->addheader( projectsippacket::Allow, ALLOWSTRING );
}

/*!md
# addremotepartid
Add a remotepatyid header:
setheaderparam( "server10.biloxi.com" "z9hG4bK4b43c2ff8.1" );
Via: SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1
*/
bool projectsippacket::addremotepartyid( const char *realm,
                                          const char *calleridname,
                                          const char *callerid,
                                          bool hide )
{
  char paramvalue[ DEFAULTHEADERLINELENGTH ];
  char *ptr = paramvalue;

  if( NULL != calleridname )
  {
    size_t calleridnamelen = strlen( calleridname );
    if( calleridnamelen > 0 )
    {
      memcpy( ptr, "\"", 1 );
      ptr++;
      memcpy( ptr, calleridname, calleridnamelen );
      ptr += calleridnamelen;
      memcpy( ptr, "\" ", 2 );
      ptr += 2;
    }
  }

  memcpy( ptr, "<sip:", 5 );
  ptr += 5;

  if( NULL != callerid )
  {
    size_t calleridlen = strlen( callerid );
    if( calleridlen > 0 )
    {
      memcpy( ptr, callerid, calleridlen );
      ptr += calleridlen;
    }
  }

  memcpy( ptr, "@", 1 );
  ptr ++;

  if( NULL != realm )
  {
    size_t realmlen = strlen( realm );
    if( realmlen > 0 )
    {
      memcpy( ptr, realm, realmlen );
      ptr += realmlen;
    }
  }

  memcpy( ptr, ">", 1 );
  ptr ++;

  if( hide )
  {
    memcpy( ptr, ";privacy=full", 13 );
    ptr += 13;
  }

  *ptr = 0;

  this->addheader( projectsippacket::Remote_Party_ID, paramvalue );
  return true;
}

/*!md
# addviaheader
Add a via header:
setheaderparam( "server10.biloxi.com" "z9hG4bK4b43c2ff8.1" );
Via: SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1
*/
bool projectsippacket::addviaheader( const char *host, projectsippacket::pointer ref )
{
  size_t lh = strlen( host );
  char paramvalue[ DEFAULTHEADERLINELENGTH ];
  memcpy( &paramvalue[ 0 ], "SIP/2.0/UDP ", 12 );
  memcpy( &paramvalue[ 12 ], host, lh );
  memcpy( &paramvalue[ lh + 12 ], ";rport;", 7 );

  stringptr br;
  if( NULL == ref )
  {
    br = projectsippacket::branch();
    stringptr bptr = projectsippacket::branch();
    memcpy( &paramvalue[ lh + 12 + 7 ], bptr->c_str(), bptr->length() );
    paramvalue[ lh + 12 + 7 + bptr->length() ] = 0;

    this->addheader( projectsippacket::Via, paramvalue );
  }
  else
  {
    substring branch = ref->getheaderparam( projectsippacket::Via, "branch" );

    size_t lb = branch.end() - branch.start();

    if( ( lh + lb ) > DEFAULTHEADERLINELENGTH )
    {
      return false;
    }

    if( 0 != lb )
    {
      memcpy( &paramvalue[ lh + 12 + 7 ], "branch=", 7 );
      const char *branchsrc = branch.c_str();
      memcpy( &paramvalue[ lh + 12 + 7 + 7 ], branchsrc, lb );
      paramvalue[ lh + 12 + 7 + 7 + lb ] = 0;

      this->addheader( projectsippacket::Via, paramvalue );
    }
  }

  return true;
}

/*!md
# addauthenticateheader
Generate a nonce parameter used for auth and add the authenticate
header. Ref RFC 2617 section 3.2.1.
*/
bool projectsippacket::addwwwauthenticateheader( projectsippacket::pointer ref )
{

  boost::uuids::basic_random_generator<boost::mt19937> gen;

  std::string s;
  s.reserve( DEFAULTHEADERLINELENGTH );

  s = "Digest realm=\"";
  //sipuri suri( ref->getrequesturi() );
  //s += suri.host.str();
  s += ref->gethost().str();

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

/*!md
# md5hashtostring
Converts input 16 bytes hex to string hex. buf need to be 33 bytes to
include null terminator.
*/
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

/*!md
# ha1
Calculate the h(a1) using cnonce and cnonce (algorithm = "MD5-sess").
Ref RFC 2617.
*/
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

/*!md
# ha2
Calculate the h(a2). Ref RFC 2617.

*/
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

/*!md
# kd
Calculate the kd. Ref RFC 2617.

*/
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

/*!md
# checkauth
Check the auth of this packet. The reference contans the nonce to check we set the nonce and it is not a replay.
*/
bool projectsippacket::checkauth( projectsippacket::pointer ref, stringptr password )
{
  char h1[ 33 ];
  char h2[ 33 ];
  char response[ 33 ];

  if( !ref ) return false;

  substring requestopaque = ref->getheaderparam( projectsippacket::WWW_Authenticate, "opaque" );
  substring receivedopaque = this->getheaderparam( projectsippacket::Authorization, "opaque" );
  if( requestopaque != receivedopaque )
  {
    return false;
  }

  substring cnonce = this->getheaderparam( projectsippacket::Authorization, "cnonce" );
  substring noncecount = this->getheaderparam( projectsippacket::Authorization, "nc" );
  substring qop = this->getheaderparam( projectsippacket::Authorization, "qop" );
  substring user = this->getheaderparam( projectsippacket::Authorization, "username" );

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

/*!md
# addauthorizationheader
Add the auth header using the credentials.

See RFC 2617 section 3.2.1

We receive either
401 message
WWW-Authenticate: Digest realm="blah.net", nonce="5cf6929e0000d71c330c35e96f9c166f2405c2834423ca9c"
OR
407 message
Proxy-Authenticate: Digest realm="blah.net", nonce="5cf6929e0000d71c330c35e96f9c166f2405c2834423ca9c"

We add either a Authorization or Proxy-Authorization is added depending on either 401 or 407 in the response.

From RFC 3261 section 20.6.
Authorization: Digest username="Alice", realm="atlanta.com",
       nonce="84a4cc6f3082121f32b42a2187831a9e",
       response="7587245234b3434cc3412213e5f113a5432"
*/
void projectsippacket::addauthorizationheader( projectsippacket::pointer ref, std::string &user, std::string &password )
{
  char h1[ 33 ];
  char h2[ 33 ];
  char response[ 33 ];
  char headerstr[ 500 ];

  int sourceheader = projectsippacket::Proxy_Authenticate;
  int responseheader = projectsippacket::Proxy_Authorization;

  if( 401 == ref->getstatuscode() )
  {
    sourceheader = projectsippacket::WWW_Authenticate;
    responseheader = projectsippacket::Authorization;
  }

  substring nonce = ref->getheaderparam( sourceheader, "nonce" );
  substring realm = ref->getheaderparam( sourceheader, "realm" );
  substring opaque = ref->getheaderparam( sourceheader, "opaque" );
  substring algorithm = ref->getheaderparam( sourceheader, "algorithm" );
  substring qop = ref->getheaderparam( sourceheader, "qop" );
  std::string cnonce;
  std::string noncecount;
  if( qop.length() > 0 )
  {
    boost::random::random_device rng;
    boost::random::uniform_int_distribution<> index_dist( 0, randomcharbase.size() - 1 );
    for( int i = 0; i < 8; ++i )
    {
      cnonce += randomcharbase[ index_dist( rng ) ];
    }

    noncecount = "00000001";
  }

  substring uri = this->getheaderparam( sourceheader, "uri" );
  if( 0 == uri.end() )
  {
    uri = this->uri;
  }

  /* check for anything which could make our header str go oversized */
  if( (
      user.length() +
      nonce.length() +
      realm.length() +
      uri.length() +
      opaque.length()
    ) > 300 ) return;

  kd(
    ha1( user.c_str(), user.length(),
         realm.c_str(), realm.length(),
         password.c_str(), password.length(),
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

  /* now add all of the required params then header */
  int it = 17;
  memcpy( headerstr, "Digest username=\"", it );

  memcpy( headerstr + it, user.c_str(), user.size() );
  it += user.size();
  memcpy( headerstr + it, "\",realm=\"", 9 );
  it += 9;
  memcpy( headerstr + it, realm.c_str(), realm.length() );
  it += realm.length();
  memcpy( headerstr + it, "\",nonce=\"", 9 );
  it += 9;
  memcpy( headerstr + it, nonce.c_str(), nonce.length() );
  it += nonce.length();
  memcpy( headerstr + it, "\",uri=\"", 7 );
  it += 7;
  memcpy( headerstr + it, uri.c_str(), uri.length() );
  it += uri.length();
  memcpy( headerstr + it, "\",response=\"", 12 );
  it += 12;
  memcpy( headerstr + it, response, 32 );
  it += 32;
  memcpy( headerstr + it, "\"", 1 );
  it += 1;

  if( 0 != opaque.end() )
  {
    memcpy( headerstr + it, ",opaque=\"", 9 );
    it += 9;
    memcpy( headerstr + it, opaque.c_str(), opaque.length() );
    it += opaque.length();
    memcpy( headerstr + it, "\" ", 2 );
    it += 2;
  }

  if( qop.length() > 0 )
  {
    memcpy( headerstr + it, ",qop=\"auth\",algorithm=MD5,nc=", 29 );
    it += 29;

    memcpy( headerstr + it, noncecount.c_str(), noncecount.size() );
    it += noncecount.size();

    memcpy( headerstr + it, ",cnonce=\"", 9 );
    it += 9;

    memcpy( headerstr + it, cnonce.c_str(), cnonce.size() );
    it += cnonce.size();

    memcpy( headerstr + it, "\"", 1 );
    it += 1;
  }

  this->addheader( responseheader, headerstr );
}

/*!md
# gettouser
Get the user addressed in the To header.
*/
substring projectsippacket::getuser( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).user;
}

/*!md
# gettohost
Get the host addressed in the To header.

*/
substring projectsippacket::gethost( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).host;
}

/*!md
# gettohost
Get the host addressed in the To header.

*/
substring projectsippacket::gettag( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).getparameter( "tag" );
}



/*!md
# getdisplayname
Returns the display name.

*/
substring projectsippacket::getdisplayname( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).displayname;
}

/*!md
# getuserhost
Get the user@host addressed in the either To header and or URI
based on request/response.

*/
substring projectsippacket::getuserhost( int tofrom )
{
  return sipuri( this->getheader( tofrom ) ).userhost;
}

/*!md
# geturihost
Get the host part from the uri addressed in the To header.

*/
substring projectsippacket::geturihost( void )
{
  return sipuri( this->uri ).host;
}

/*!md
# getexpires
Get the expires value from the different placesit might be.

*/
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

/*!md
# getcseq
Get the cseq value from teh cseq header (the number value)

*/
int projectsippacket::getcseq( void )
{
  substring ex;
  if( this->hasheader( projectsippacket::CSeq ) )
  {
    ex = this->getheader( projectsippacket::CSeq );
    return std::stoi( ex.str() );
  }
  return -1;
}

/*!md
# projectsippacket destructor


*/
const char* projectsippacket::getversion( void )
{
  return "SIP/2.0";
}

/*!md
# getmethodfromcrc
Converts crc to header value. Switch statement comes from
gensipheadercrc.py. We only store references to the supported headers.

*/
int projectsippacket::getmethodfromcrc( int crc )
{
  switch( crc )
  {
    case 0x5ff94014:   /* register */
    {
      return REGISTER;
    }
    case 0x217bedc8:   /* notify */
    {
      return NOTIFY;
    }
    case 0xa3137be3:   /* refer */
    {
      return REFER;
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

/*!md
# projectsippacket getheaderstr
Convert a header id to a string.

*/
const char *projectsippacket::getmethodstr( int method )
{
  switch( method )
  {
    case REGISTER:
      return "REGISTER";
    case REFER:
      return "REFER";
    case NOTIFY:
      return "NOTIFY";
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


/*!md
# getheaderfromcrc
Converts crc to header value. Switch statement comes from
gensipheadercrc.py. We only store references to the supported headers.

*/
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
    case 0xc72b507c:   /* alert-info */
    {
      return Alert_Info;
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
    case 0x7b8e3878:   /* remote-party-id */
    {
      return Remote_Party_ID;
    }
    case 0x2c42079:   /* route */
    {
      return Route;
    }
    case 0x3499ab92:   /* retry-after */
    {
      return Retry_After;
    }
    case 0x3bb8880c:   /* reason */
    {
      return Reason;
    }
    case 0x1f417f42:   /* subscription-state */
    {
      return Subscription_State;
    }
    case 0xf724db04:   /* supported */
    {
      return Supported;
    }
    case 0xd787d2c4:   /* to */
    {
      return To;
    }
    case 0xba5b4323:   /* refer-to */
    {
      return Refer_To;
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

/*!md
# projectsippacket getheaderstr
Convert a header id to a string.

*/
const char *projectsippacket::getheaderstr( int header )
{
  switch( header )
  {
    case Authorization:
      return "Authorization";
    case Allow:
      return "Allow";
    case Alert_Info:
      return "Alert-Info";
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
    case Remote_Party_ID:
      return "Remote-Party-ID";
    case Route:
      return "Route";
    case Retry_After:
      return "Retry-After";
    case Reason:
      return "Reason";
    case Subscription_State:
      return "Subscription-State";
    case Supported:
      return "Supported";
    case To:
      return "To";
    case Refer_To:
      return "Refer-To";
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



/*!md
# sipuri constructor
Parse a SIP URI.

*/
sipuri::sipuri( substring s ) :
   s( s ),
   uri( s ),
   displayname( s ),
   protocol( s ),
   user( s ),
   secret( s ),
   host( s ),
   userhost( s ),
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
  this->uri.start( this->protocol.start() );
  this->protocol--; /* remove the : */

  substring sipuserhost = s.findsubstr( '<', '>' );
  if( 0 == sipuserhost.end() )
  {
    sipuserhost.end( s.end() );
    this->uri.end( s.end() );
  }
  else
  {
    this->uri.end( sipuserhost.end() );
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

  this->userhost.start( this->user.start() );
  this->userhost.end( this->host.end() );
}


/*****************************************************************************
# getparameter
Returns the substring index into s for the given param name.
std::string s = From: sip:+12125551212@server.phone2net.com;tag=887s
getparameter( s, "tag" );
Will return a substring index to '887s'.

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
# getheader
Similar to get parameter but for headers. In a SIP URI anything after
the ? is considered a header. Name value pairs.

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
