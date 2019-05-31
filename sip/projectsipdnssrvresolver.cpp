
#warning TODO - check comments
/*!md
# DNS SRV Resolver

This should be used with one instance of the projectsipdnssrvresolverinfo class. projectsipdnssrvresolver can be called by others.

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
projectsipdnssrvresolverinfo dnscache;

projectsipdnssrvresolverinfo::projectsipdnssrvresolverinfo()
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
iplist projectsipdnssrvresolverinfo::getdnsiplist( void )
{
  return this->dnsservers;
}

/*!md
## addtocache
We maintain a cache of lookups which expire on ttl.
*/
void projectsipdnssrvresolverinfo::addtocache( dnssrvrealm::pointer )
{

}

dnssrvrealm::pointer projectsipdnssrvresolverinfo::getfromcache( std::string realm )
{
  return dnssrvrealm::pointer();
}


/*!md
# create
*/
dnssrvrealm::pointer dnssrvrealm::create()
{
  return pointer( new dnssrvrealm() );
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
Kick off our resolution. The realm should be in the format _sip_._udp.bling.babblevoice.com.
*/
void projectsipdnssrvresolver::query( std::string realm, std::function<void ( dnssrvrealm::pointer ) > callback )
{
  this->onresolve = callback;

  this->resolved = dnscache.getfromcache( realm );
  if( this->resolved )
  {
    this->onresolve( this->resolved );
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
    return;
  }

  ns_msg h;
  if( ns_initparse( this->answerbuffer, bytes_transferred, &h ) )
  {
    /* Failed to parse thrown error */
    this->onresolve( dnssrvrealm::pointer() );
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
      return;
    }

    if ( ns_rr_type(rr) == ns_t_srv )
    {
      char name[ 1024 ];
      if( -1 == dn_expand( ns_msg_base( h ), ns_msg_end( h ), ns_rr_rdata( rr ) + 6, name, sizeof( name ) ) )
      {
        this->onresolve( dnssrvrealm::pointer() );
        return;
      }

      dnssrvrecord record;

      record.host = name;
      record.expires = time( NULL ) + ns_rr_ttl( rr );
      record.priority = ntohs( *( unsigned short* )ns_rr_rdata( rr ) );
      record.weight = ntohs( *( ( unsigned short* )ns_rr_rdata( rr ) + 1 ) );
      record.port = ntohs( *( ( unsigned short* )ns_rr_rdata( rr ) + 2 ) );
      this->resolved->records.push_back( record );
    }
  }

  dnscache.addtocache( this->resolved );
  this->onresolve( this->resolved );
}
