
#warning TODO - check comments
/*!md
# DNS SRV Resolver

Example:
```
projectsipdnssrvresolver::pointer srvresolver = projectsipdnssrvresolver::create();
srvresolver->query(
    "_sip._udp.bling.babblevoice.com",
    std::bind( &yourcurrentclass::handlesrvresolve,
      shared_from_this(),
      std::placeholders::_1
    ) );
```

This should be used with one instance of the projectsipdnssrvresolvercache class. projectsipdnssrvresolver can be called by others.

Ideas for this came from http://people.samba.org/bzr/jerry/slag/unix/query-srv.c.

TODO
 - [] Retry on timeout
 - [] Work through list of DNS server on no response
 - [] Once worked through all servers ensure we call back as our call may want to release us/memory
*/

#include <iostream>
#include <stdio.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <netinet/in.h>
#include <resolv.h>
#include <string>

/* time() */
#include <ctime>

#include "projectsipdnssrvresolver.h"

extern boost::asio::io_service io_service;
projectsipdnssrvresolvercache dnscache;


dnssrvrecord::dnssrvrecord() :
  baduntil( 0 )
{

}

dnssrvrecord::~dnssrvrecord()
{
  std::cout << "~dnssrvrecord" << std::endl;
}

/*!md
## markasbad
We mark a server bad for a small period of time if we need to do so.
*/
void dnssrvrecord::markasbad( void )
{
  this->baduntil = time( NULL ) + DNSRECORDBADFOR;
}

/*!md
## isgood
As it says. See above function also.
*/
bool dnssrvrecord::isgood( void )
{
  if( this->baduntil > time( NULL ) )
  {
    return false;
  }
  return true;
}

/*!md
# create
*/
dnssrvrecord::pointer dnssrvrecord::create()
{
  return pointer( new dnssrvrecord() );
}

/*!md
# dnssrvrealm
*/
dnssrvrealm::dnssrvrealm() :
  expiretimer( io_service )
{

}

/*!md
# create
*/
dnssrvrealm::pointer dnssrvrealm::create()
{
  return pointer( new dnssrvrealm() );
}

/*!md
## getbestrecord
Search through the list of records we have and return the most appropriate. There is an element of randomness.
*/
dnssrvrecord::pointer dnssrvrealm::getbestrecord( void )
{
  int totalweightforpriority;
  if( 0 == this->records.size() ) return dnssrvrecord::pointer();

  dnssrvrecords::iterator it;
  dnssrvrecord::pointer selection = *this->records.begin();
  totalweightforpriority = selection->weight;

  for( it = this->records.begin(); it != this->records.end(); it++ )
  {
    if( !(*it)->isgood() ) continue;
    double r = ( double ) rand() / ( double ) RAND_MAX; /* 0 => RAND_MAX ~ RAND_MAX / r = 0 => 1 */

    if( (*it)->priority == selection->priority )
    {
      /* we can continue with the current selection. */
      totalweightforpriority += (*it)->priority;
      double prob = ( double ) (*it)->priority / ( double ) totalweightforpriority;

      if( prob > r )
      {
        /* we have won */
        selection = *it;
      }
    }
    else
    {
      /* we start a new selection. */
      selection = *it;
      totalweightforpriority = selection->weight;
    }
  }

  return selection;
}

/*!md
## goexpiretimer
Set a timer to remove us from the cache when we expire
*/
void dnssrvrealm::goexpiretimer( void )
{
  if( this->records.size() > 0 )
  {
    this->expiretimer.cancel();
    /* expire on ttl - plus a little leaway */

    this->expiretimer.expires_after( std::chrono::seconds( ( * this->records.begin() )->ttl + 2 ) );
    this->expiretimer.async_wait( std::bind( &dnssrvrealm::onexpiretimer, shared_from_this(), std::placeholders::_1 ) );
  }
  else
  {
    this->expiretimer.cancel();
    /* expire on a short time to allow recovery from errors */
    this->expiretimer.expires_after( std::chrono::seconds( 60 ) );
    this->expiretimer.async_wait( std::bind( &dnssrvrealm::onexpiretimer, shared_from_this(), std::placeholders::_1 ) );
  }
}

/*!md
## onexpiretimer
If our timer expires (or errors) then in both csaes we should simply remove us from the cache.
*/
void dnssrvrealm::onexpiretimer( const boost::system::error_code& error )
{
  std::cout << "dnssrvrealm::onexpiretimer" << std::endl;
  dnscache.removefromcache( shared_from_this() );
}

/*!md
## projectsipdnssrvresolvercache
Constructor - load our DNS servers.
*/
projectsipdnssrvresolvercache::projectsipdnssrvresolvercache()
{
  res_init();

  if ( _res.nscount <= 0 )
  {
    /* We need DNS severs */
    return;
  }

  for( int i = 0; i < _res.nscount; i++ )
  {
    std::string ip = std::to_string( (_res.nsaddr_list[i].sin_addr.s_addr & 0xff) >> 0 ) + "." +
      std::to_string( (_res.nsaddr_list[i].sin_addr.s_addr & 0xff00) >> 8 ) + "." +
      std::to_string( (_res.nsaddr_list[i].sin_addr.s_addr & 0xff0000) >> 16 ) + "." +
      std::to_string( (_res.nsaddr_list[i].sin_addr.s_addr & 0xff000000) >> 24 );

    this->dnsservers.push_back( ip );
  }
}

/*!md
## Return the list of IP for DNS servers.
*/
iplist projectsipdnssrvresolvercache::getdnsiplist( void )
{
  return this->dnsservers;
}

/*!md
## addtocache
We maintain a cache of lookups which expire on ttl.

TODO
 - [] Add timer to remove from cache on expiry.
*/
void projectsipdnssrvresolvercache::addtocache( dnssrvrealm::pointer realm )
{
std::cout << "projectsipdnssrvresolvercache::addtocache: " << realm->realm << std::endl;
  this->cache.insert( realm );
  realm->goexpiretimer();
}

/*!md
## getfromcache

Gets from ur cache if it exists otherwise return an empty entry.
*/
dnssrvrealm::pointer projectsipdnssrvresolvercache::getfromcache( std::string realm )
{
  dnssrvrealms::index< dnssrvrealmrealm >::type::iterator it;
  it = this->cache.get< dnssrvrealmrealm >().find( realm );
  if( this->cache.get< dnssrvrealmrealm >().end() != it )
  {
    return *it;
  }

  return dnssrvrealm::pointer();
}

/*!md
## removefromcache
Remove our entry from teh cache.
*/
void projectsipdnssrvresolvercache::removefromcache( dnssrvrealm::pointer r )
{
std::cout << "projectsipdnssrvresolvercache::removefromcache: " << r->realm << std::endl;
  dnssrvrealms::index< dnssrvrealmrealm >::type::iterator it;
  it = this->cache.get< dnssrvrealmrealm >().find( r->realm );
  if( this->cache.get< dnssrvrealmrealm >().end() != it )
  {
    this->cache.erase( it );
  }
}

/*!md
## Constructor

Initialse and grab system DNS sevrers.
*/
projectsipdnssrvresolver::projectsipdnssrvresolver() :
  socket( io_service ),
  resolver( io_service )
{
  this->socket.open( boost::asio::ip::udp::v4() );
}

projectsipdnssrvresolver::~projectsipdnssrvresolver()
{
  this->socket.close();
}

/*!md
## Create

Create a resolver inside an auto pointer.
*/
projectsipdnssrvresolver::pointer projectsipdnssrvresolver::create()
{
  return pointer( new projectsipdnssrvresolver() );
}

/*!md
## query
Our main client function. Kick off our resolution. The realm should be in the format _sip_._udp.bling.babblevoice.com.
*/
void projectsipdnssrvresolver::query( std::string realm, std::function<void ( dnssrvrealm::pointer ) > callback )
{
  this->onresolve = callback;

  this->resolved = dnscache.getfromcache( realm );
  if( this->resolved )
  {
std::cout << "retreived from cache" << std::endl;
    this->onresolve( this->resolved );
    this->onresolve = nullptr;
    return;
  }

  this->resolved = dnssrvrealm::create();
  this->resolved->realm = realm;

  if ( -1 == ( this->querylength = res_mkquery( QUERY, realm.c_str(), C_IN, ns_t_srv, NULL, 0, NULL, this->querybuffer, sizeof( this->querybuffer ) ) ) )
  {
    /* Inidicate error */
    return;
  }

  std::string dnsip = dnscache.getdnsiplist().front();
  boost::asio::ip::udp::resolver::query query( boost::asio::ip::udp::v4(), dnsip, "domain" );

  /* Run through all DNS on failure */
  this->resolver.async_resolve( query,
      boost::bind( &projectsipdnssrvresolver::handleresolve,
        shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::iterator ) );

}

/*!md
## handleresolve
We have resolved (or not)
*/
void projectsipdnssrvresolver::handleresolve (
            boost::system::error_code e,
            boost::asio::ip::udp::resolver::iterator it )
{
  boost::asio::ip::udp::resolver::iterator end;

  if( it == end )
  {
    /* Failure - inform our callback */
    this->onresolve( dnssrvrealm::pointer() );
    this->onresolve = nullptr;
    return;
  }

  this->receiverendpoint = *it;

  this->socket.async_send_to(
              boost::asio::buffer( this->querybuffer, this->querylength ),
              this->receiverendpoint,
              boost::bind(
                &projectsipdnssrvresolver::handlesend,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred) );

}


/*!md
## handlesend
Once we have sent, wait for response.
*/
void projectsipdnssrvresolver::handlesend(
                    const boost::system::error_code& error,
                    std::size_t bytes_transferred)
{
  if ( !error )
  {
    this->socket.async_receive_from(
      boost::asio::buffer( this->answerbuffer, SRVDNSSENDRECEVIEVEBUFFSIZE ),
        this->receivedfromendpoint,
        boost::bind(
          &projectsipdnssrvresolver::handleanswer,
          shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred ) );
  }
}


/*!md
## handleanswer
Once we have sent, wait for response.

ID of requst is the first 2 bytes in request and answer:
  std::cout << "id: " << +answerbuffer[ 0 ] << +answerbuffer[ 1 ] << std::endl;
*/
void projectsipdnssrvresolver::handleanswer( const boost::system::error_code& error,
      std::size_t bytes_transferred )
{
  if ( error )
  {
    this->onresolve( dnssrvrealm::pointer() );
    this->onresolve = nullptr;
    return;
  }

  ns_msg h;
  if( ns_initparse( this->answerbuffer, bytes_transferred, &h ) )
  {
    /* Failed to parse thrown error */
    this->onresolve( dnssrvrealm::pointer() );
    this->onresolve = nullptr;
    return;
  }

  /* Go through our messages */
  ns_rr rr;
  for ( int i = 0; i < ns_msg_count( h, ns_s_an ); i++ )
  {
    if ( ns_parserr( &h, ns_s_an, i, &rr ) )
    {
      /* report error */
      this->onresolve( dnssrvrealm::pointer() );
      this->onresolve = nullptr;
      return;
    }

    if ( ns_rr_type(rr) == ns_t_srv )
    {
      char name[ 1024 ];
      if( -1 == dn_expand( ns_msg_base( h ), ns_msg_end( h ), ns_rr_rdata( rr ) + 6, name, sizeof( name ) ) )
      {
        this->onresolve( dnssrvrealm::pointer() );
        this->onresolve = nullptr;
        return;
      }

      dnssrvrecord::pointer record = dnssrvrecord::create();

      record->host = name;
      record->ttl = ns_rr_ttl( rr );
      record->priority = ntohs( *( unsigned short* )ns_rr_rdata( rr ) );
      record->weight = ntohs( *( ( unsigned short* )ns_rr_rdata( rr ) + 1 ) );
      record->port = ntohs( *( ( unsigned short* )ns_rr_rdata( rr ) + 2 ) );
      this->resolved->records.push_back( record );
    }
  }

  dnscache.addtocache( this->resolved );
  this->onresolve( this->resolved );
  this->onresolve = nullptr;
}
