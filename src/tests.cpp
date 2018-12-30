/*******************************************************************************
File: test.cpp
Purpose: Test framework. run "make clean; make test" to build this test file.
../Debug/test can then be run to functionaliy test parts of the program. 
To test for memory leakage use "valgrind ../Debug/test --leak-check=yes"
Updated: 30.12.2018
*******************************************************************************/

#include <iostream>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <sys/resource.h>

#include <fstream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>

#include "test.h"

#include "projectsipserver.h"
#include "projectsippacket.h"
#include "projectsipregistrar.h"

/*******************************************************************************
Function: readfile
Purpose: Read a file into a std::string
Updated: 13.12.2018
*******************************************************************************/
std::string readfile( std::string file )
{
  std::ifstream in;
  in.open( file );
  std::stringstream sstr;
  sstr << in.rdbuf();
  return sstr.str();
}

/*******************************************************************************
Function: gettestchunk
Purpose: Returns a block seperated by a start tag and end tag.

==TAG==
This is what is returned.
==END==

Updated: 14.12.2018
*******************************************************************************/
stringptr gettestchunk( std::string &contents, std::string tag)
{
  size_t startpos = contents.find( "==" + tag + "==\r\n" );
  size_t endpos = contents.find( "\r\n==END==", startpos );

  if( std::string::npos == startpos || std::string::npos == endpos )
  {
    return stringptr( new std::string( "" ) );
  }

  startpos += 6 + tag.size();

  if( startpos > contents.size() )
  {
    return stringptr( new std::string( "" ) );
  }

  if( endpos > contents.size() )
  {
    return stringptr( new std::string( "" ) );
  }

  return stringptr( new std::string( contents.substr( startpos, endpos - startpos ) ) );
}


/*******************************************************************************
Function: testsippacket
Purpose: sippacket tests
To ensure the test files have the correct formatting, use:
todos siptest1.txt (todos is found in the debian tofrodos package)
Updated: 13.12.2018
*******************************************************************************/
void testsippacket( void )
{
  std::string testdata = readfile( "../testfiles/siptest1.txt" );

  {
    projectsippacket testpacket( gettestchunk( testdata, "TEST1" ) );

    projecttest( testpacket.getmethod(), projectsippacket::REGISTER , "Invalid method");

    projecttest( testpacket.getheader( projectsippacket::Call_ID ),
            "843817637684230@998sdasdh09",
            "Wrong Call ID." );
    projecttest( testpacket.getheader( projectsippacket::To ),
            "Bob <sip:bob@biloxi.com>",
            "Wrong To header field." );

    projecttest( testpacket.getrequesturi(),
            "sip:registrar.biloxi.com",
            "Wrong Request URI." );

    projecttest( testpacket.hasheader( projectsippacket::Authorization ),
            false,
            "We don't have a Authorization header." )

    projecttest( testpacket.hasheader( projectsippacket::From ),
            true,
            "We should have a From header." )

  }

  {
    projectsippacket testpacket( gettestchunk( testdata, "TEST2" ) );

    projecttest( testpacket.getstatuscode(), 180, "Expecting 180." );

    projecttest( testpacket.getheader( projectsippacket::Contact ),
            "<sip:bob@192.0.2.4>",
            "Wrong Contact field." );

    projecttest( testpacket.getheader( projectsippacket::Call_ID ),
            "a84b4c76e66710",
            "Wrong Call ID." );

    projecttest( testpacket.getheader( projectsippacket::Content_Type ),
            "multipart/signed;\r\n "
            "protocol=\"application/pkcs7-signature\";\r\n "
            "micalg=sha1; boundary=boundary42",
            "Wrong Content Type." );

    projecttest( testpacket.getheader( projectsippacket::Via ),
            "SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1\r\n "
            ";received=192.0.2.3",
            "Wrong Content Type." );

  }

  std::cout << "All sippacket tests passed, looking good" << std::endl;
}


/*******************************************************************************
Function: testurl
Purpose: URL class tests
Updated: 17.12.2018
*******************************************************************************/
void testurl( void )
{
  {
    stringptr u( new std::string( "http://myhost/my/big/path?myquerystring" ) );
    httpuri s( u );

    projecttest( s.protocol.substr(), "http", "Bad protocol." );
    projecttest( s.host.substr(), "myhost", "Bad host." );
    projecttest( s.path.substr(), "/my/big/path", "Bad path." );
    projecttest( s.query.substr(), "myquerystring", "Bad query." );
  }

  {
    stringptr u( new std::string( "\"Bob\" <sips:bob@biloxi.com> ;tag=a48s" ) );
    sipuri s( u );

    projecttest( s.displayname.substr(), "Bob", "Bad Display name." );
    projecttest( s.protocol.substr(), "sips", "Bad proto." );
    projecttest( s.user.substr(), "bob", "Bad user." );
    projecttest( s.host.substr(), "biloxi.com", "Bad host." );
    projecttest( s.parameters.substr(), "tag=a48s", "Bad params." );
  }

  {
    stringptr u( new std::string( "Anonymous <sip:c8oqz84zk7z@privacy.org>;tag=hyh8" ) );
    sipuri s( u );

    projecttest( s.displayname.substr(), "Anonymous", "Bad Display name." );
    projecttest( s.protocol.substr(), "sip", "Bad proto." );
    projecttest( s.user.substr(), "c8oqz84zk7z", "Bad user." );
    projecttest( s.host.substr(), "privacy.org", "Bad host." );
    projecttest( s.parameters.substr(), "tag=hyh8", "Bad params." );
  }

  {
    stringptr u( new std::string( "sip:+12125551212@phone2net.com;tag=887s" ) );
    sipuri s( u );

    projecttest( s.displayname.substr(), "", "Bad Display name." );
    projecttest( s.protocol.substr(), "sip", "Bad proto." );
    projecttest( s.user.substr(), "+12125551212", "Bad user." );
    projecttest( s.host.substr(), "phone2net.com", "Bad host." );
    projecttest( s.parameters.substr(), "tag=887s", "Bad params." );
    projecttest( s.getparameter( "tag" ).substr(), "887s", "Bad tag param." );
  }

  {
    stringptr u( new std::string( "<sip:+12125551212@phone2net.com>;tag=887s;blah=5566654gt" ) );
    sipuri s( u );

    projecttest( s.displayname.substr(), "", "Bad Display name." );
    projecttest( s.protocol.substr(), "sip", "Bad proto." );
    projecttest( s.user.substr(), "+12125551212", "Bad user." );
    projecttest( s.host.substr(), "phone2net.com", "Bad host." );
    projecttest( s.parameters.substr(), "tag=887s;blah=5566654gt", "Bad params." );
    projecttest( s.getparameter( "tag" ).substr(), "887s", "Bad tag param." );
    projecttest( s.getparameter( "blah" ).substr(), "5566654gt", "Bad blah param." );
  }

  /*
    Examples from RFC 3261.
  */
  {
    stringptr u( new std::string( "sip:alice@atlanta.com" ) );
    sipuri s( u );
    projecttest( s.protocol.substr(), "sip", "Bad proto." );
    projecttest( s.user.substr(), "alice", std::string( "Bad user: " ) + *u );
    projecttest( s.host.substr(), "atlanta.com", "Bad host." );
  }

  {
    stringptr u( new std::string( "sip:alice:secretword@atlanta.com;transport=tcp" ) );
    sipuri s( u );
    projecttest( s.protocol.substr(), "sip", "Bad proto." );
    projecttest( s.user.substr(), "alice", "Bad user." );
    projecttest( s.secret.substr(), "secretword", "Bad secret." );
    projecttest( s.host.substr(), "atlanta.com", "Bad host." );
    projecttest( s.parameters.substr(), "transport=tcp", "Bad params." );
    projecttest( s.getparameter( "transport" ).substr(), "tcp", "Bad transport param." );
  }

  {
    stringptr u( new std::string( "sips:alice@atlanta.com?subject=project%20x&priority=urgent" ) );
    sipuri s( u );
    projecttest( s.protocol.substr(), "sips", "Bad proto." );
    projecttest( s.user.substr(), "alice", "Bad user." );
    projecttest( s.host.substr(), "atlanta.com", "Bad host." );
    projecttest( urldecode( s.headers.substr() ), "subject=project x&priority=urgent", "Bad headers." );
    projecttest( urldecode( s.getheader( "subject" ).substr() ), "project x", "Bad header." );
    projecttest( urldecode( s.getheader( "priority" ).substr() ), "urgent", "Bad header." );
  }

  {
    stringptr u( new std::string( "sip:+1-212-555-1212:1234@gateway.com;user=phone" ) );
    sipuri s( u );
    projecttest( s.protocol.substr(), "sip", "Bad proto." );
    projecttest( s.user.substr(), "+1-212-555-1212", "Bad user." );
    projecttest( s.secret.substr(), "1234", "Bad secret." );
    projecttest( s.host.substr(), "gateway.com", "Bad host." );
    projecttest( s.parameters.substr(), "user=phone", "Bad params." );
    projecttest( s.getparameter( "user" ).substr(), "phone", "Bad transport param." );
  }

  {
    stringptr u( new std::string( "sips:1212@gateway.com" ) );
    sipuri s( u );
    projecttest( s.protocol.substr(), "sips", "Bad proto." );
    projecttest( s.user.substr(), "1212", "Bad user." );
    projecttest( s.host.substr(), "gateway.com", "Bad host." );
  }

  {
    stringptr u( new std::string( "sip:alice@192.0.2.4" ) );
    sipuri s( u );
    projecttest( s.protocol.substr(), "sip", "Bad proto." );
    projecttest( s.user.substr(), "alice", "Bad user." );
    projecttest( s.host.substr(), "192.0.2.4", "Bad host." );
  }

  {
    /* We can now just use the substring compare - this saves on a an allocation of a new string */
    stringptr u( new std::string( "sip:atlanta.com;method=REGISTER?to=alice%40atlanta.com" ) );
    sipuri s( u );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.host, "atlanta.com", "Bad host." );
    projecttest( s.parameters, "method=REGISTER", "Bad params." );
    projecttest( s.headers, "to=alice%40atlanta.com", "Bad headers." );
  }

  {
    stringptr u( new std::string( "param=the cat,the dog+" ) );
    projecttest( urlencode( u ), "param%3dthe+cat%2cthe+dog%2b", "Url encode failed." );
  }

  std::cout << "All url class tests passed, looking good" << std::endl;
}


/*******************************************************************************
Function: stringtest
Purpose: Test some of our string classes. We have our own substring class
so we can maintain one string with indexes into it to save on both memory and 
allocations.
Updated: 30.12.2018
*******************************************************************************/
void stringtest( void )
{
  {
    /* These will test the != operator */
    stringptr u( new std::string( "12345hello6789" ) );
    substring t( u, 5, 10 );
    substring t2( u, 0, 5 );

    // std::cout << *t.substr() << std::endl;
    // std::cout << *t2.substr() << std::endl;

    projecttest( t, "hello", "Uh oh, I was expecting hello" );
    projecttest( t2, "12345", "Uh oh, I was expecting 12345" );

    if( !( t != "hello678" ) )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": I was expecting to get false for hello678" << std::endl;
      return;
    }

        if( !( t != "hell" ) )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": I was expecting to get false for hell" << std::endl;
      return;
    }

    /* Now test the == operator */
    if( ! ( t == "hello" ) )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": I was expecting to get true for hello" << std::endl;
      return;
    }

    if( ! ( t2 == "12345" ) )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": I was expecting to get true for 12345" << std::endl;
      return;
    }

    if( t == "hello678" )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": t should be shorter than hello678" << std::endl;
      return;
    }

    if( t == "hell" )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": t should be longer than hell" << std::endl;
      return;
    }
  }

  std::cout << "All string tests passed, looking good" << std::endl;
}

/*******************************************************************************
Function: main
Purpose: Run our tests
Updated: 12.12.2018
*******************************************************************************/
int main( int argc, const char* argv[] )
{
  stringtest();
  testurl();
  testsippacket();
  testregs();

  return 0;
}




