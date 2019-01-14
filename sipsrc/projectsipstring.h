

#include <string>
#include <boost/shared_ptr.hpp>

#ifndef PROJECTSIPSTRING_H
#define PROJECTSIPSTRING_H


typedef boost::shared_ptr< std::string > stringptr;


/*******************************************************************************
Function: moveonbyn
Purpose: Little helper function - safely move on n chars in a string.
Updated: 12.12.2018
*******************************************************************************/
inline std::string::iterator moveonbyn( stringptr s, size_t n )
{
  return s->length() <= n ? s->end() : s->begin() + n;
}

inline void moveonbyn( stringptr str, std::string::iterator &s, size_t n )
{
  for( size_t i = 0; i < n; i++ )
  {
    s++;
    if( str->end() == s )
    {
      return;
    }
  }
}

/*******************************************************************************
Class: substring
Purpose: Helper class, used to hold indexes into a larger string so that we
only need to maintain 1 buffer.
Updated: 12.12.2018
*******************************************************************************/
class substring
{
public:
  substring();
  substring( char * s );
  substring( substring ss, size_t start, size_t end ){ this->s = ss.get(); this->startpos = start; this->endpos = end; };
  substring( stringptr s );
  substring( stringptr s, size_t start, size_t end );
  stringptr substr();
  const char *c_str(){ return this->s->c_str() + this->startpos; };

  stringptr get( void ){ return this->s; }
  size_t end();
  size_t end( size_t end );
  size_t start();
  size_t start( size_t start );

  substring rfind( const char * );
  substring find( const char * );
  substring rfind( const char );
  substring find( const char, size_t offset = 0 );

  substring findsubstr( const char, const char end = 0 );
  substring findend( const char );
  substring aftertoken( const char * );
  substring aftertoken( const char );
  substring rtrim( void );
  substring ltrim( void );
  substring trim( void );

  size_t operator++( int );

  size_t operator--( int );
  size_t length( void );
  int toint();

  friend bool operator != ( const substring& lhs, const char *rhs );
  friend bool operator == ( const substring& lhs, const char *rhs );

private:
  size_t startpos;
  size_t endpos;
  stringptr s;
};

bool operator==( const substring& lhs, const char *rhs );
bool operator!=( const substring& lhs, const char *rhs );

#ifdef TESTCODE
/*******************************************************************************
Function: operator << ostream, substring
Purpose: For use with our tests, out put the result of a comparison. Not for live.
Updated: 30.12.2018
*******************************************************************************/
inline std::ostream & operator << ( std::ostream& os, substring obj )
{
  os << *( obj.substr() );
  return os;
}
#endif /* TESTCODE */

typedef boost::shared_ptr< substring > substringptr;

stringptr urldecode( stringptr str );
stringptr urlencode( stringptr str );

stringptr urldecode( substring str );
stringptr urlencode( substring str );

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
                      char *buf );

char fromhex( char ch );
char tohex( char code );


#endif /* PROJECTSIPSTRING_H */


