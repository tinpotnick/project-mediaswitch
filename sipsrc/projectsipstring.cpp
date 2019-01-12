
#include <cstring>
#include <boost/lexical_cast.hpp>

/* md5 */
#include <openssl/md5.h>

#include "projectsipstring.h"


// to remove after fixing
#include <iostream>

/*******************************************************************************
Function: fromhex
Purpose: URL encode and decode. Thank you: http://www.geekhideout.com/urlcode.shtml
for the basis of this. Converts a hex character to its integer value.
Updated: 18.12.2018
*******************************************************************************/
char fromhex( char ch )
{
  return isdigit( ch ) ? ch - '0' : tolower( ch ) - 'a' + 10;
}

/*******************************************************************************
Function: tohex
Purpose: Converts an integer value to its hex character
Updated: 18.12.2018
*******************************************************************************/
char tohex( char code )
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
      *encoded += tohex( c >> 4 );
      *encoded += tohex( c & 15 );
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
          *decoded += fromhex( *digit1 ) << 4 | fromhex( *digit2 );
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
      return true;
    }

    index++;
    c++;
  }

  if( 0 != *c )
  {
    return true;
  }

  return false;
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


/*******************************************************************************
Class: substring
Purpose: Class to maintain pointers in a string for a substring without copying
the underlying string.
Updated: 08.01.2019
*******************************************************************************/

/*******************************************************************************
Function: substring constructor
Purpose: As it say.
Updated: 08.01.2019
*******************************************************************************/
substring::substring()
{
  this->startpos = 0;
  this->endpos = 0;
}

/*******************************************************************************
Function: substring constructor
Purpose: As it say.
Updated: 08.01.2019
*******************************************************************************/
substring::substring( char * s )
{
  this->s = stringptr( new std::string( s ) );
  this->startpos = 0;
  this->endpos = this->s->length();
}

/*******************************************************************************
Function: substring constructor
Purpose: As it say.
Updated: 08.01.2019
*******************************************************************************/
substring::substring( stringptr s )
{
  this->s = s;
  this->startpos = 0;
  this->endpos = s->length();
}

/*******************************************************************************
Function: substring constructor
Purpose: As it say.
Updated: 08.01.2019
*******************************************************************************/
substring::substring( stringptr s, size_t start, size_t end )
{
  this->s = s;
  this->startpos = start;
  this->endpos = end;
}

/*******************************************************************************
Function: substr
Purpose: Return a new stringptr (copy string).
Updated: 08.01.2019
*******************************************************************************/
stringptr substring::substr()
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

/*******************************************************************************
Function: end
Purpose: Returns the index of the end pointer.
Updated: 08.01.2019
*******************************************************************************/
size_t substring::end()
{
  return this->endpos;
}

/*******************************************************************************
Function: end
Purpose: Sets and returns the index of the end pointer.
Updated: 08.01.2019
*******************************************************************************/
size_t substring::end( size_t end )
{
  this->endpos = end;
  return this->endpos;
}

/*******************************************************************************
Function: start
Purpose: Returns the index of the start pointer.
Updated: 08.01.2019
*******************************************************************************/
size_t substring::start()
{
  return this->startpos;
}

/*******************************************************************************
Function: start
Purpose: Sets and returns the index of the start pointer.
Updated: 08.01.2019
*******************************************************************************/
size_t substring::start( size_t start )
{
  this->startpos = start;
  return this->startpos;
}

/*******************************************************************************
Function: operator++
Purpose: Moves on the end pointer.
Updated: 08.01.2019
*******************************************************************************/
size_t substring::operator++( int )
{
  return this->endpos ++;
}


/*******************************************************************************
Function: operator--
Purpose: Moves back the end pointer.
Updated: 08.01.2019
*******************************************************************************/
size_t substring::operator--( int )
{
  return this->endpos --;
}

/*******************************************************************************
Function: length
Purpose: Returns the length of the substring
Updated: 08.01.2019
*******************************************************************************/
size_t substring::length( void ) 
{ 
  return this->endpos - this->startpos; 
}

/*******************************************************************************
Function: toint
Purpose: Converts the substring to an int and returns it.
Updated: 08.01.2019
*******************************************************************************/
int substring::toint( void )
{
  char b[ 200 ];
  int retval = -1;
  size_t l = this->length();
  char *stptr = ( char * ) this->s->c_str() + this->startpos;

  if( l > 200 )
  {
    return -1;
  }

  memcpy( b, stptr, l );
  b[ l ] = 0;

  try
  {
    retval = boost::lexical_cast< int >( ( char * ) &b[ 0 ] );
  }
  catch( boost::bad_lexical_cast &e )
  {

  }

  return retval;
}

/*******************************************************************************
Function: rfind
Purpose: Equivalent to std::string rfind - but within our substring. Returns
a substring (rather than a size_t).
Updated: 08.01.2019
*******************************************************************************/
substring substring::rfind( const char *s )
{
  size_t l = strlen( s );
  size_t rfindpos = this->s->rfind( s, this->endpos - l );

  if( std::string::npos == rfindpos ||
        rfindpos < this->startpos )
  {
    return substring( this->s, 0, 0 );
  }

  return substring( this->s, rfindpos, rfindpos + l );
}

/*******************************************************************************
Function: rfind
Purpose: See above
Updated: 08.01.2019
*******************************************************************************/
substring substring::rfind( const char s )
{
  size_t rfindpos = this->s->rfind( s, this->endpos - 1 );

  if( std::string::npos == rfindpos ||
        rfindpos < this->startpos )
  {
    return substring( this->s, 0, 0 );
  }

  return substring( this->s, rfindpos, rfindpos + 1 );
}

/*******************************************************************************
Function: find
Purpose: Equivalent to std::string find - but within our substring. Returns
a substring (rather than a size_t).
Updated: 08.01.2019
*******************************************************************************/
substring substring::find( const char *s )
{
  size_t rfindpos = this->s->find( s, this->startpos );

  if( std::string::npos == rfindpos ||
        rfindpos > this->endpos )
  {
    return substring( this->s, 0, 0 );
  }

  size_t l = strlen( s );
  return substring( this->s, rfindpos, rfindpos + l );
}

/*******************************************************************************
Function: find
Purpose: See above
Updated: 08.01.2019
*******************************************************************************/
substring substring::find( const char s )
{
  size_t rfindpos = this->s->find( s, this->startpos );

  if( std::string::npos == rfindpos ||
        rfindpos > this->endpos )
  {
    return substring( this->s, 0, 0 );
  }

  return substring( this->s, rfindpos, rfindpos + 1 );
}


/*******************************************************************************
Function: md5hashtostring
Purpose: Converts input 16 bytes hex to string hex. buf need to be 33 bytes to 
include null terminator.
Updated: 08.01.2019
*******************************************************************************/
static inline void md5hashtostring( unsigned char* buf )
{
  unsigned char *in = buf + 15;
  buf += 31;

  for( int i = 0; i < 16; i++ )
  {
    unsigned char c = *in;
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
static inline unsigned char * ha1( const unsigned char *username, size_t ul, 
                          const unsigned char *realm, size_t rl, 
                          const unsigned char *password, size_t pl,
                          const unsigned char *nonce, size_t cl, 
                          const unsigned char *cnonce, size_t cnl, 
                          const char *alg,
                          unsigned char *buf )
{
  MD5_CTX context;
  MD5_Init( &context);
  MD5_Update( &context, username, ul );
  MD5_Update( &context, ":", 1 );
  MD5_Update( &context, realm, rl );
  MD5_Update( &context, ":", 1 );
  MD5_Update( &context, password, pl );
  MD5_Final( buf, &context );

  if ( strcasecmp( alg, "md5-sess" ) == 0 )
  {
    MD5_Init( &context );
    MD5_Update( &context, buf, 16 );
    MD5_Update( &context, ":", 1 );
    MD5_Update( &context, nonce, cl );
    MD5_Update( &context, ":", 1 );
    MD5_Update( &context, cnonce, cnl );
    MD5_Final( buf, &context );
  }

  md5hashtostring( buf );

  return buf;
}

/*******************************************************************************
Function: ha2
Purpose: Calculate the h(a2). Ref RFC 2617.
Updated: 11.01.2019
*******************************************************************************/
static inline unsigned char * ha2( const unsigned char *method, size_t ml, 
                          const unsigned char *uri, size_t ul,
                          unsigned char *buf )
{
  MD5_CTX context;
  MD5_Init( &context);
  MD5_Update( &context, method, ml );
  MD5_Update( &context, ":", 1 );
  MD5_Update( &context, uri, ul );
  MD5_Final( buf, &context );

  md5hashtostring( buf );

  return buf;
}

/*******************************************************************************
Function: kd
Purpose: Calculate the kd. Ref RFC 2617.
Updated: 11.01.2019
*******************************************************************************/
static inline unsigned char *kd( const unsigned char *ha1,
                                  const unsigned char *nonce, size_t nl,
                                  const unsigned char *nc, size_t ncl,
                                  const unsigned char *cnonce, size_t cl,
                                  const unsigned char *qop, size_t ql,
                                  const unsigned char *ha2,
                                  unsigned char * buf )
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
  MD5_Final( buf, &context );
  md5hashtostring( buf );

  return buf;
}

/*******************************************************************************
Function: requestdigest
Purpose: The 1 call to calculate the SIP request digest.
Updated: 11.01.2019
*******************************************************************************/
unsigned char * requestdigest( const unsigned char *username, size_t ul, 
                                const unsigned char *realm, size_t rl, 
                                const unsigned char *password, size_t pl,
                                const unsigned char *nonce, size_t nl, 
                                const unsigned char *nc, size_t ncl, 
                                const unsigned char *cnonce, size_t cnl,
                                const unsigned char *method, size_t ml, 
                                const unsigned char *uri, size_t url,
                                const unsigned char *qop, size_t ql,
                                const char *alg,
                                unsigned char *buf )
{
  unsigned char h1[ 33 ];
  unsigned char h2[ 33 ];

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



