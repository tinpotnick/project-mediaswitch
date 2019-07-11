
#include <cstring>
#include <boost/lexical_cast.hpp>

#include "projectsipstring.h"

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
static char __tohexhex[] = "0123456789abcdef";
char tohex( char code )
{
  return __tohexhex[ code & 15 ];
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
Function: urldecode
Purpose: Same as above but takes a substring as a param.
Updated: 14.01.2019
*******************************************************************************/
stringptr urldecode( substring str )
{
  return urldecode( str.strptr() );
}


/*******************************************************************************
Function: urlencode
Purpose: Same as above but takes a substring as a param.
Updated: 14.01.2019
*******************************************************************************/
stringptr urlencode( substring str )
{
  return urlencode( str.strptr() );
}



/*******************************************************************************
Function: operator != substring const char *
Purpose: compare a substring with a const char*
Updated: 30.12.2018
*******************************************************************************/
bool operator!=( const substring& lhs, const char *rhs )
{
  if( !lhs.s )
  {
    if( 0 == *rhs )
    {
      return true;
    }
    return false;
  }

  if( 0 == *rhs )
  {
    if( lhs.s->length() > 0 )
    {
      return false;
    }
    return true;
  }

  if( 0 == ( lhs.endpos - lhs.startpos ) && 0 == *rhs )
  {
    return false;
  }

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
Function: operator != substring const substring&
Purpose: compare a substring with a substring
Updated: 15.01.2019
*******************************************************************************/
bool operator!=( const substring& lhs, const substring& rhs )
{
  if( !lhs.s )
  {
    if( !rhs.s )
    {
      return true;
    }
    return false;
  }

  if( !rhs.s )
  {
    if( lhs.s->length() > 0 )
    {
      return false;
    }
    return true;
  }

  if( ( lhs.endpos - lhs.startpos ) == ( rhs.endpos - rhs.startpos ) )
  {
    return false;
  }

  std::string::iterator rit = moveonbyn( rhs.s, rhs.startpos );

  for( std::string::iterator it = moveonbyn( lhs.s, lhs.startpos );
        it != lhs.s->end();
        it ++ )
  {
    if( rhs.s->end() != rit )
    {
      break;
    }

    if( *it != *rit )
    {
      return true;
    }
    rit++;
  }

  if( rhs.s->end() != rit )
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
  if( !lhs.s )
  {
    if(  0 == *rhs )
    {
      return false;
    }
    return true;
  }

  const char *c = rhs;
  size_t index = lhs.startpos;

  if( 0 == ( lhs.endpos - lhs.startpos ) && 0 == *rhs )
  {
    return true;
  }

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
Function: operator == substring const substring&
Purpose: compare a substring with a substring
Updated: 15.01.2019
*******************************************************************************/
bool operator==( const substring& lhs, const substring& rhs )
{
  if( !lhs.s )
  {
    if( !rhs.s )
    {
      return false;
    }
    return true;
  }

  if( ( lhs.endpos - lhs.startpos ) != ( rhs.endpos - rhs.startpos ) )
  {
    return false;
  }

  std::string::iterator rit = moveonbyn( rhs.s, rhs.startpos );

  for( std::string::iterator it = moveonbyn( lhs.s, lhs.startpos );
        it != lhs.s->end();
        it ++ )
  {
    if( rhs.s->end() != rit )
    {
      break;
    }
    if( *it != *rit )
    {
      return false;
    }

    rit++;
  }

  if( rhs.s->end() != rit )
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
Updated: 05.02.2019
*******************************************************************************/
substring::substring( substring s, size_t start, size_t end )
{
  this->s = s.s;
  this->startpos = start;
  this->endpos = end;
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
Function: string
Purpose: Return a new std::string (copy string).
Updated: 14.01.2019
*******************************************************************************/
std::string substring::str( void )
{
  if ( !this->s )
  {
    return std::string( "" );
  }

  if( 0 == this->endpos )
  {
    return std::string( "" );
  }

  if( this->startpos > s->size() )
  {
    return std::string( "" );
  }

  if( this->endpos > s->size() )
  {
    return std::string( "" );
  }
  return s->substr( this->startpos, this->endpos - this->startpos );
}

/*******************************************************************************
Function: stringptr
Purpose: Return a new stringptr (copy string).
Updated: 08.01.2019
*******************************************************************************/
stringptr substring::strptr()
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
  this->endpos--;
  if( this->endpos < this-> startpos )
  {
    this->endpos = this->startpos;
  }
  return this->endpos;
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
substring substring::find( const char s, size_t offset )
{
  size_t rfindpos = this->s->find( s, this->startpos + offset );

  if( std::string::npos == rfindpos ||
        rfindpos > this->endpos )
  {
    return substring( this->s, 0, 0 );
  }

  return substring( this->s, rfindpos, rfindpos + 1 );
}

/*******************************************************************************
Function: find_first_of
Purpose: Similar to the std::string::find_first_of - finds the first character
in the str provided. Moves the end to that location.
Updated: 15.01.2019
*******************************************************************************/
substring substring::mvend_first_of( const char *str )
{
  size_t rfindpos = this->s->find_first_of( str, this->startpos );
    if( std::string::npos == rfindpos ||
        rfindpos > this->endpos )
  {
    return substring( this->s, this->startpos, this->endpos );
  }
  return substring( this->s, this->startpos, rfindpos );
}

substring substring::mvend_first_of( const char ch )
{
  size_t rfindpos = this->s->find_first_of( ch, this->startpos );
    if( std::string::npos == rfindpos ||
        rfindpos > this->endpos )
  {
    return substring( this->s, this->startpos, this->endpos );
  }
  return substring( this->s, this->startpos, rfindpos );
}


/*******************************************************************************
Function: findsubstr
Purpose: Finds a substring in a string surounded by start and stop. For example
s = " Test <1002@bling.babblevoice.com>"
r = findsubstr( '<'.'>' );
Wounld return a substring == "1002@bling.babblevoice.com"
Updated: 13.01.2019
*******************************************************************************/
substring substring::findsubstr( const char start, const char stop )
{
  size_t startpos = this->s->find( start, this->startpos );

  if( std::string::npos == startpos ||
        startpos > this->endpos )
  {
    return substring( this->s, 0, 0 );
  }

  if( 0 == stop )
  {
    return substring( this->s, startpos + 1, this->endpos );
  }

  size_t endpos = this->s->find( stop, startpos + 1 );

  if( std::string::npos == endpos ||
        endpos > this->endpos )
  {
    return substring( this->s, startpos + 1, this->endpos );
  }

  return substring( this->s, startpos + 1, endpos );
}

/*******************************************************************************
Function: findend
Purpose: Changes the end position based on searching for a character. It starts 
at the current start position and will set the end at character found (leaving
that character out). For example
s = " Test <1002@bling.babblevoice.com>"
r = findsubstr( '<'.'>' );
t = r.findend( '@' );
It it fails to find a new end, the current end is maintained.
Wounld return a substring == "1002"
Updated: 13.01.2019
*******************************************************************************/
substring substring::findend( const char ch )
{
  size_t findpos = this->s->find( ch, this->startpos );

  if( std::string::npos == findpos ||
        findpos > this->endpos )
  {
    return substring( this->s, this->startpos, this->endpos );
  }

  return substring( this->s, this->startpos, findpos );
}


/*******************************************************************************
Function: aftertoken
Purpose: Looks for the string after a token, for example:
s = "1002@bling.babblevoice.com"
r = aftertoken( "1002@" );
Wounld return a substring == "bling.babblevoice.com"
Updated: 13.01.2019
*******************************************************************************/
substring substring::aftertoken( const char * token )
{
  size_t findpos = this->s->find( token, this->startpos );

  if( std::string::npos == findpos ||
        findpos > this->endpos )
  {
    return substring( this->s, 0, 0 );
  }

  return substring( this->s, findpos + strlen( token ), this->endpos );
}

/*******************************************************************************
Function: aftertoken
Purpose: Looks for the string after a token, for example:
s = "1002@bling.babblevoice.com"
r = aftertoken( "1002@" );
Wounld return a substring == "bling.babblevoice.com"
Updated: 13.01.2019
*******************************************************************************/
substring substring::aftertoken( const char token )
{
  size_t findpos = this->s->find( token, this->startpos );

  if( std::string::npos == findpos ||
        findpos > this->endpos )
  {
    return substring( this->s, 0, 0 );
  }

  return substring( this->s, findpos + 1, this->endpos );
}


/*******************************************************************************
Function: ltrim
Purpose: Removes whitespace from beggining of substring.
Updated: 14.01.2019
*******************************************************************************/
substring substring::ltrim( void )
{
  const char *c = this->s->c_str();
  size_t newstartpos = this->startpos;
  for( size_t i = this->startpos; i < this->endpos; i++ )
  {
    switch( *c )
    {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      {
        newstartpos++;
        break;
      }
      default:
      {
        return substring( this->s, newstartpos, this->endpos );
      }
    }
    c++;
  }
  return substring( this->s, newstartpos, this->endpos );
}

/*******************************************************************************
Function: rtrim
Purpose: Removes whitespace from end of substring.
Updated: 14.01.2019
*******************************************************************************/
substring substring::rtrim( void )
{
  const char *c = this->s->c_str() + this->endpos;

  size_t newendpos = this->endpos;
  for( size_t i = this->endpos; i > this->startpos; i-- )
  {
    switch( *c )
    {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      {
        newendpos--;
        break;
      }
      default:
      {
        return substring( this->s, this->startpos, newendpos );
      }
    }
    c--;
  }
  return substring( this->s, this->startpos, newendpos );
}

/*******************************************************************************
Function: trim
Purpose: Removes whitespace from beggining and end of substring.
Updated: 14.01.2019
*******************************************************************************/
substring substring::trim( void )
{
  return this->ltrim().rtrim();
}

/*******************************************************************************
Function: joinstring
Purpose: Join a list of strings together delimitered with delim. This pair of 
functions are designed to be used with small strings/lists.
Updated: 22.01.2019
*******************************************************************************/
std::string joinstring( stringvector items, char delim )
{
	stringvector::iterator it;
	std::string retval;

	for ( it = items.begin(); it != items.end() ; it ++)
	{
		if ( retval != "" )
		{
			retval += delim;
		}
		retval += *it;
	}

	return retval;
}

/*******************************************************************************
Function: splitstring
Purpose: Split (Explode) a string using delim. e.g.
sub.domain.com
stringlist l;
l[0] = sub
l[1] = domain
l[2] = com
Updated: 22.01.2019
*******************************************************************************/
stringvector splitstring( std::string strtosplit, char delim )
{
	std::string stritem;
	stringvector strlist;

	for ( std::string::iterator it = strtosplit.begin() ; it != strtosplit.end(); it ++ )
	{
		if ( *it == delim )
		{
			strlist.push_back( stritem );
			stritem = "";
			continue;
		}

		stritem += *it;
	}

	// the final item - which doesn't have to have a delim...
	if ( stritem != "" )
	{
		strlist.push_back( stritem );
	}

  if( 0 == strlist.size() )
  {
    // Make sure we are an empty string.
    strlist.push_back( "" );
  }

	return strlist;
}
