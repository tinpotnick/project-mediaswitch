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
#include "projectsipsm.h"
#include "projectsipstring.h"
#include "projectsipsdp.h"
#include "projectsipdirectory.h"

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
    return stringptr( new std::string() );
  }

  startpos += 6 + tag.size();

  if( startpos > contents.size() )
  {
    return stringptr( new std::string() );
  }

  if( endpos > contents.size() )
  {
    return stringptr( new std::string() );
  }

  return stringptr( new std::string( contents.substr( startpos, endpos - startpos ) ) );
}

/*******************************************************************************
Function: respond
Purpose: Store the results of any respnse from our test sm.
Updated: 08.01.2019
*******************************************************************************/
void projectsipservertestpacket::respond( stringptr doc )
{
  this->response = doc;
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

    projecttest( testpacket.getheaderparam( projectsippacket::Via, "branch" ),
                  "z9hG4bK4b43c2ff8.1",
                  "Bad branch value" );

    projecttest( testpacket.getheaderparam( projectsippacket::Via, "received" ),
                  "192.0.2.3",
                  "Bad received value" );

  }

  // Test sip packet creation
  {
    projectsippacket::pointer testpacket = projectsippacket::create();

    testpacket->setstatusline( 180, "Ringing" );
    testpacket->addheader( projectsippacket::From, "Alice <sip:alice@atlanta.com>;tag=1928301774" );
    testpacket->addheader( projectsippacket::CSeq, "314159 INVITE" );

    projecttest( *testpacket->strptr(),
                  "SIP/2.0 180 Ringing\r\n"
                  "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                  "CSeq: 314159 INVITE\r\n"
                  "\r\n",
                  "Unexpected SIP packet."
                  );


    testpacket->setbody( "Some SDP?" );
    testpacket->addheader( projectsippacket::Via, "SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1" );

    projecttest( *testpacket->strptr(),
                  "SIP/2.0 180 Ringing\r\n"
                  "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                  "CSeq: 314159 INVITE\r\n"
                  "Via: SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bK4b43c2ff8.1\r\n"
                  "\r\n"
                  "Some SDP?",
                  "Unexpected SIP packet."
                  );


    projectsippacket::pointer testpacket2 = projectsippacket::create();
    testpacket2->setrequestline( projectsippacket::REGISTER, "sip:registrar.biloxi.com" );

    testpacket2->addheader( projectsippacket::From, "Alice <sip:alice@atlanta.com>;tag=1928301774" );
    testpacket2->addheader( projectsippacket::CSeq, "314159 INVITE" );

    testpacket2->getmethod();
    projecttest( testpacket2->getheader( projectsippacket::From ), "Alice <sip:alice@atlanta.com>;tag=1928301774", "Header does not match." );

    testpacket2->addviaheader( "myhost", testpacket );
    testpacket2->setbody( "Some SDP" );

    projecttest( *testpacket2->strptr(),
                  "REGISTER sip:registrar.biloxi.com SIP/2.0\r\n"
                  "From: Alice <sip:alice@atlanta.com>;tag=1928301774\r\n"
                  "CSeq: 314159 INVITE\r\n"
                  "Via: SIP/2.0/UDP myhost;rport;branch=z9hG4bK4b43c2ff8.1\r\n"
                  "\r\n"
                  "Some SDP",
                  "Unexpected SIP packet."
                  );

  }

  {

    projectsippacket testpacket( gettestchunk( testdata, "AUTHREGISTER5" ) );

    projecttest(
      testpacket.getheaderparam( projectsippacket::Authorization, "response" ),
      "fcb607534a1cf55bfe5641539d7371ef",
      "Couldn't find response correctly." );

    projecttest(
      testpacket.getheaderparam( projectsippacket::Authorization, "algorithm" ),
      "MD5",
      "Algorithm was wrong." );

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
    stringptr u( new std::string( "http://127.0.0.1:9000" ) );
    httpuri uri( u );

    projecttest( uri.protocol, "http", "Bad protocol." );
    projecttest( uri.host, "127.0.0.1", "Bad host." );
    projecttest( uri.port, "9000", "Bad port." );
  }

  {
    stringptr u( new std::string( "http://myhost/my/big/path?myquerystring" ) );
    httpuri s( u );

    projecttest( s.protocol, "http", "Bad protocol." );
    projecttest( s.host, "myhost", "Bad host." );
    projecttest( s.path, "/my/big/path", "Bad path." );
    projecttest( s.query, "myquerystring", "Bad query." );
  }

  {
    stringptr u( new std::string( "rubbishhttp://myhost/my/big/path?myquerystring" ) );
    substring t = substring( u, 7, u->length() );
    httpuri s( t );

    projecttest( s.protocol, "http", "Bad protocol." );
    projecttest( s.host, "myhost", "Bad host." );
    projecttest( s.path, "/my/big/path", "Bad path." );
    projecttest( s.query, "myquerystring", "Bad query." );
  }

  {
    stringptr u( new std::string( "http://myhost:9000/my/big/path?myquerystring" ) );
    substring t = substring( u );
    httpuri s( t );

    projecttest( s.protocol, "http", "Bad protocol." );
    projecttest( s.host, "myhost", "Bad host." );
    projecttest( s.path, "/my/big/path", "Bad path." );
    projecttest( s.query, "myquerystring", "Bad query." );
    projecttest( s.port, "9000", "Bad Port." );
  }

  {
    std::string testdata = readfile( "../testfiles/siptest1.txt" );
    projectsippacket testpacket( gettestchunk( testdata, "AUTHREGISTER1" ) );

    projecttest( testpacket.gethost(), "bling.babblevoice.com", "Bad host." );
  }

  {
    std::string testdata = readfile( "../testfiles/siptest1.txt" );
    projectsippacket testpacket( gettestchunk( testdata, "REFER1" ) );

    stringptr referto = urldecode( testpacket.getheaderparam( projectsippacket::Refer_To, "replaces" ) );

    substring callid = substring( referto ).mvend_first_of( ';' ).trim();
    substring totag = substring( referto ).findsubstr( "to-tag=", ';' ).trim();
    substring fromtag = substring( referto ).findsubstr( "from-tag=", ';' ).trim();

    projecttest( testpacket.getheaderparam( projectsippacket::Refer_To, "replaces" ), "773901b0-89a9-4c1a-9254-a03325066431%3Bto-tag%3DUBUkt5cE%3Bfrom-tag%3D8cb96940", "Bad replaces." );
    projecttest( callid, "773901b0-89a9-4c1a-9254-a03325066431", "Bad replaces callid." );
    projecttest( totag, "UBUkt5cE", "Bad replaces to-tag." );
    projecttest( fromtag, "8cb96940", "Bad replaces from-tag." );

    projecttest( testpacket.getreplaces(), "773901b0-89a9-4c1a-9254-a03325066431", "Bad replaces callid." );
    projecttest( testpacket.getreplacestotag(), "UBUkt5cE", "Bad replaces to-tag." );
    projecttest( testpacket.getreplacesfromtag(), "8cb96940", "Bad replaces from-tag." );
  }

  {
    std::string testdata = readfile( "../testfiles/siptest1.txt" );
    stringptr s = gettestchunk( testdata, "REPLACES1" );
    projectsippacket testpacket2( s );

    projecttest( testpacket2.getreplaces(), "425928@phone.example.org", "Bad replaces callid." );
    projecttest( testpacket2.getreplacestotag(), "7743", "Bad replaces to-tag." );
    projecttest( testpacket2.getreplacesfromtag(), "6472", "Bad replaces to-tag." );
  }

  {
    stringptr u( new std::string( "\"Bob\" <sips:bob@biloxi.com> ;tag=a48s" ) );
    sipuri s( u );

    projecttest( s.displayname, "Bob", "Bad Display name." );
    projecttest( s.protocol, "sips", "Bad proto." );
    projecttest( s.user, "bob", "Bad user." );
    projecttest( s.host, "biloxi.com", "Bad host." );
    projecttest( s.parameters, "tag=a48s", "Bad params." );
  }

  {
    stringptr u( new std::string( "Anonymous <sip:c8oqz84zk7z@privacy.org>;tag=hyh8" ) );
    sipuri s( u );

    projecttest( s.displayname, "Anonymous", "Bad Display name." );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.user, "c8oqz84zk7z", "Bad user." );
    projecttest( s.host, "privacy.org", "Bad host." );
    projecttest( s.parameters, "tag=hyh8", "Bad params." );
    projecttest( s.uri, "sip:c8oqz84zk7z@privacy.org", "Bad uri." );
    projecttest( s.userhost, "c8oqz84zk7z@privacy.org", "Bad uri." );
  }

  {
    stringptr u( new std::string( "sip:+12125551212@phone2net.com;tag=887s" ) );
    sipuri s( u );

    projecttest( s.displayname, "", "Bad Display name." );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.user, "+12125551212", "Bad user." );
    projecttest( s.host, "phone2net.com", "Bad host." );
    projecttest( s.parameters, "tag=887s", "Bad params." );
    projecttest( s.uri, "sip:+12125551212@phone2net.com;tag=887s", "Bad uri." );
    projecttest( s.userhost, "+12125551212@phone2net.com", "Bad userhost." );
    projecttest( s.getparameter( "tag" ), "887s", "Bad tag param." );
  }

  {
    stringptr u( new std::string( "<sip:+12125551212@phone2net.com>;tag=887s;blah=5566654gt" ) );
    sipuri s( u );

    projecttest( s.displayname, "", "Bad Display name." );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.user, "+12125551212", "Bad user." );
    projecttest( s.host, "phone2net.com", "Bad host." );
    projecttest( s.parameters, "tag=887s;blah=5566654gt", "Bad params." );
    projecttest( s.getparameter( "tag" ), "887s", "Bad tag param." );
    projecttest( s.getparameter( "blah" ), "5566654gt", "Bad blah param." );
  }

  /*
    Examples from RFC 3261.
  */
  {
    stringptr u( new std::string( "sip:alice@atlanta.com" ) );
    sipuri s( u );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.user, "alice", std::string( "Bad user: " ) + *u );
    projecttest( s.host, "atlanta.com", "Bad host." );
  }

  {
    stringptr u( new std::string( "sip:alice:secretword@atlanta.com;transport=tcp" ) );
    sipuri s( u );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.user, "alice", "Bad user." );
    projecttest( s.secret, "secretword", "Bad secret." );
    projecttest( s.host, "atlanta.com", "Bad host." );
    projecttest( s.parameters, "transport=tcp", "Bad params." );
    projecttest( s.getparameter( "transport" ), "tcp", "Bad transport param." );
  }

  {
    stringptr u( new std::string( "sips:alice@atlanta.com?subject=project%20x&priority=urgent" ) );
    sipuri s( u );
    projecttest( s.protocol, "sips", "Bad proto." );
    projecttest( s.user, "alice", "Bad user." );
    projecttest( s.host, "atlanta.com", "Bad host." );
    projecttest( urldecode( s.headers ), "subject=project x&priority=urgent", "Bad headers." );
    projecttest( urldecode( s.getheader( "subject" ) ), "project x", "Bad header." );
    projecttest( urldecode( s.getheader( "priority" ) ), "urgent", "Bad header." );
  }

  {
    stringptr u( new std::string( "sip:+1-212-555-1212:1234@gateway.com;user=phone" ) );
    sipuri s( u );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.user, "+1-212-555-1212", "Bad user." );
    projecttest( s.secret, "1234", "Bad secret." );
    projecttest( s.host, "gateway.com", "Bad host." );
    projecttest( s.parameters, "user=phone", "Bad params." );
    projecttest( s.getparameter( "user" ), "phone", "Bad transport param." );
  }

  {
    stringptr u( new std::string( "sips:1212@gateway.com" ) );
    sipuri s( u );
    projecttest( s.protocol, "sips", "Bad proto." );
    projecttest( s.user, "1212", "Bad user." );
    projecttest( s.host, "gateway.com", "Bad host." );
  }

  {
    stringptr u( new std::string( "sip:alice@192.0.2.4" ) );
    sipuri s( u );
    projecttest( s.protocol, "sip", "Bad proto." );
    projecttest( s.user, "alice", "Bad user." );
    projecttest( s.host, "192.0.2.4", "Bad host." );
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
    substring t3( u, 0, 5 );
    substring t4;

    if( t4.valid() )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": We should not get here..." << std::endl;
      return;
    }

    if( !t.valid() )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": We should not get here..." << std::endl;
      return;
    }

    projecttest( t, "hello", "Uh oh, I was expecting hello" );
    projecttest( t2, "12345", "Uh oh, I was expecting 12345" );

    projecttest( t2, t3, "This really should be the same." );
    if( t == t2 )
    {
      std::cout << __FILE__ << ":" << __LINE__ << ": We should not get here..." << std::endl;
      return;
    }

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

  {
    stringptr u( new std::string( "Contact: <sip:1003@82.19.197.210:55381>;methods=" ) );
    substring t( u );

    substring r = t.findsubstr( '<', '>' );

    projecttest( r, "sip:1003@82.19.197.210:55381", "Uh oh, I was expecting 'sip:1003@82.19.197.210:55381'" );
  }

  {
    stringptr u( new std::string( "Contact: \"Bob\" <sip:1003@82.19.197.210:55381>;methods=" ) );
    substring t( u );

    substring r = t.findsubstr( '"', '"' );

    projecttest( r, "Bob", "Uh oh, I was expecting 'Bob'" );
  }

  {
    stringptr u( new std::string( "Contact: \"Bob\" <sip:1003@82.19.197.210:55381>;methods=" ) );
    substring t( u );

    substring r = t.findsubstr( ';' );

    projecttest( r, "methods=", "Uh oh, I was expecting 'methods='" );
  }

  {
    stringptr u( new std::string( "Contact: \"Bob\" <sip:1003@82.19.197.210:55381>;methods=" ) );
    substring t( u );

    substring r = t.findsubstr( '<', '>' ).aftertoken( "sip:" ).findend( '@' );

    projecttest( r, "1003", "Uh oh, I was expecting '1003'" );
  }

  std::cout << "All string tests passed, looking good" << std::endl;
}

/*******************************************************************************
Function: optionstest
Purpose: Respond to an options request
Updated: 08.01.2019
*******************************************************************************/
void optionstest( void )
{
  std::string testdata = readfile( "../testfiles/siptest1.txt" );

  projectsipservertestpacket *p = new projectsipservertestpacket( gettestchunk( testdata, "OPTIONSTEST1" ) );
  projectsippacket::pointer request( p );

  projectsipsm::handlesippacket( request );

  std::cout << "Packet size: " << p->response->length() << std::endl;
  std::cout << "===========================================" << std::endl;
  std::cout << *( p->response ) << std::endl;
  std::cout << "===========================================" << std::endl;
}

/*******************************************************************************
Function: authtest
Purpose: Test authenticating a SIP packet. Adds user to directory then looks
up and checks authentication.
Updated: 20.02.2019
*******************************************************************************/
void authtest( void )
{
  std::string dom = "bling.babblevoice.com";
  std::string u = "1003";
  std::string s = "123654789";
  projectsipdirdomain::pointer domentry = projectsipdirdomain::adddomain( dom );
  domentry->adduser( u, s );

  std::string testdata = readfile( "../testfiles/siptest1.txt" );

  projectsipservertestpacket *p = new projectsipservertestpacket( gettestchunk( testdata, "AUTHREGISTER3" ) );
  projectsippacket::pointer request( p );

  projectsipservertestpacket *auth = new projectsipservertestpacket( gettestchunk( testdata, "AUTHREGISTER4" ) );
  projectsippacket::pointer authrequest( auth );

  projecttest( authrequest->getmethod(), projectsippacket::REGISTER , "Invalid method");

  projecttest( authrequest->getheader( projectsippacket::Call_ID ),
            "f87f4505d3431a912cf5e72c778131fe",
            "Wrong Call ID." );

  projecttest( authrequest->getheader( projectsippacket::To ),
            "<sip:1003@bling.babblevoice.com>",
            "Wrong To header field." );

  projecttest( authrequest->getheaderparam( projectsippacket::Authorization, "nonce" ),
            "dbb93832-9090-4763-b9f1-49f0ee8b0b4a",
            "Wrong nonce." );

  projecttest( request->getheaderparam( projectsippacket::WWW_Authenticate, "nonce" ),
            "dbb93832-9090-4763-b9f1-49f0ee8b0b4a",
            "Wrong nonce." );


  if( !authrequest->checkauth( request, projectsipdirdomain::lookuppassword( dom, u ) ) )
  {
    std::cout << "Failed authentication - that should not have happened" << std::endl;
  }

  projectsipsm::handlesippacket( authrequest );

}

/*******************************************************************************
Function: sdptest
Purpose: Test our SDP convertion routines.
Updated: 31.01.2019
*******************************************************************************/
void sdptest( void )
{
  std::string testdata = readfile( "../testfiles/siptest1.txt" );
  {
    stringptr sdp = gettestchunk( testdata, "SDP1" );

    JSON::Value val;
    sdptojson( substring( sdp ), val );
    std::cout << JSON::to_string( val ) << std::endl;
  }

  {
    stringptr sdp = gettestchunk( testdata, "SDP2" );

    JSON::Value val;
    sdptojson( substring( sdp ), val );

    std::cout << JSON::to_string( val ) << std::endl;

    std::cout << "===========================================================" << std::endl;
    stringptr generated = jsontosdp( JSON::as_object( val ) );
    std::cout << *generated << std::endl;
    std::cout << "===========================================================" << std::endl;
  }

  {
    stringptr sdp = gettestchunk( testdata, "SDP3" );
    substring s( sdp, 7, sdp->length() );

    JSON::Value val;
    sdptojson( s, val );

    std::cout << JSON::to_string( val ) << std::endl;

    std::cout << "===========================================================" << std::endl;
    stringptr generated = jsontosdp( JSON::as_object( val ) );
    std::cout << *generated << std::endl;
    std::cout << "===========================================================" << std::endl;
  }

  {
    /*
    stringptr sdp = gettestchunk( testdata, "SDP4" );
    projectsippacket p( sdp );
    substring s( p.getbody() );

    JSON::Value val;
    sdptojson( s, val );

    std::cout << JSON::to_string( val ) << std::endl;

    {"v":0,"o":{"username":"Z","sessionid":0,"sessionversion":3,"nettype":"IN","ipver":4,"address":"192.168.0.141"},"s":"Z","c":[{"nettype":"IN","ipver":4,"address":"192.168.0.141"}],"t":{"start":0,"stop":0},"m":[{"media":"audio","port":8000,"proto":"RTP/AVP","payloads":[8,106,9,0,18,3,111,97,110,112,102,101,98,100,99],"rtpmap":{"106":{"encoding":"opus","clock":48000},"18":{"encoding":"G729","clock":8000},"111":{"encoding":"speex","clock":16000},"97":{"encoding":"iLBC","clock":8000},"110":{"encoding":"speex","clock":8000},"112":{"encoding":"speex","clock":32000},"102":{"encoding":"G726-32","clock":8000},"101":{"encoding":"telephone-event","clock":8000},"98":{"encoding":"telephone-event","clock":48000},"100":{"encoding":"telephone-event","clock":16000},"99":{"encoding":"telephone-event","clock":32000}},"fmtp":{"106":"minptime=20; cbr=1; maxaveragebitrate=40000; useinbandfec=1","18":"annexb=no","97":"mode=20","101":"0-16","98":"0-16","100":"0-16","99":"0-16"},"direction":"inactive"}]}
    */
  }
}

/*******************************************************************************
Function: runtests
Purpose: Run our tests
Updated: 12.12.2018
*******************************************************************************/
int runtests( void )
{
  sdptest();
  authtest();
  optionstest();
  stringtest();
  testurl();
  testsippacket();

  return 0;
}
