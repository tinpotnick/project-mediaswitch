
#ifndef PROJECTSIPDNSSRVRESOLVER_H
#define PROJECTSIPDNSSRVRESOLVER_H


#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <functional>

#include <string>
#include <list>

#define SRVDNSSENDRECEVIEVEBUFFSIZE 1500
#define DNSRECORDBADFOR 20

typedef std::list< std::string > iplist;

/*!md
## dnssrvrecord
Priority and Weight come from the SRV record.
The lower the priority it should be grouped together and the lowest used first.

Weight balances out between them. If one record has a weight of 60 and another 40 then the first must be used 60% of the time.

I have not quite understood why each record has a TTL (hence expires) as they always appear to be the same which makes sense as we will perform a lookup of the whole realm again when any one of them expire.
*/
class dnssrvrecord :
  public boost::enable_shared_from_this< dnssrvrecord >
{
public:
  dnssrvrecord();
  ~dnssrvrecord();

  typedef boost::shared_ptr< dnssrvrecord > pointer;
  static dnssrvrecord::pointer create();

  std::string host;
  int priority;
  int weight;
  short port;
  int ttl;
  /* We have had a problem with this server - so we mark as bad for a while */
  time_t baduntil;

  void markasbad( void );
  bool isgood( void );
};

typedef std::list< dnssrvrecord::pointer > dnssrvrecords;

class dnssrvrealm :
  public boost::enable_shared_from_this< dnssrvrealm >
{
public:
  dnssrvrealm();
  typedef boost::shared_ptr< dnssrvrealm > pointer;
  static dnssrvrealm::pointer create();

  dnssrvrecord::pointer getbestrecord( void );
  void goexpiretimer( void );
  void onexpiretimer( const boost::system::error_code& error );

  std::string realm;
  dnssrvrecords records;
  boost::asio::steady_timer expiretimer;
};


/* tags for multi index */
struct dnssrvrealmrealm{};

/*!md
# dnssrvrealms
Boost multi index set to store our dns lookups.
*/
typedef boost::multi_index::multi_index_container<
  dnssrvrealm::pointer,
  boost::multi_index::indexed_by
  <
    boost::multi_index::hashed_unique
    <
      boost::multi_index::tag< dnssrvrealmrealm >,
      boost::multi_index::member
      <
        dnssrvrealm,
        std::string,
        &dnssrvrealm::realm
      >
    >
  >
> dnssrvrealms;

/*!md
## projectsipdnssrvresolvercache
Loads the system resolver libaries and manages the cache of lookups.
*/
class projectsipdnssrvresolvercache
{
public:
  projectsipdnssrvresolvercache();
  iplist getdnsiplist();

  void addtocache( dnssrvrealm::pointer );
  dnssrvrealm::pointer getfromcache( std::string realm );
  void removefromcache( dnssrvrealm::pointer );
private:
  iplist dnsservers;
  dnssrvrealms cache;
};


/*!md
## projectsipdnssrvresolver
Resolver class for SRV records.
*/
class projectsipdnssrvresolver :
  public boost::enable_shared_from_this< projectsipdnssrvresolver >
{
public:
  projectsipdnssrvresolver();
  ~projectsipdnssrvresolver();

  typedef boost::shared_ptr< projectsipdnssrvresolver > pointer;
  static projectsipdnssrvresolver::pointer create();

  void query( std::string realm, std::function<void ( dnssrvrealm::pointer ) > callback );

private:

  void handlesend( const boost::system::error_code& error,
        std::size_t bytes_transferred );

  void handleanswer( const boost::system::error_code& error,
        std::size_t bytes_transferred );

  void handleresolve (
              boost::system::error_code e,
              boost::asio::ip::udp::resolver::iterator it );

  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::endpoint receiverendpoint;
  boost::asio::ip::udp::endpoint receivedfromendpoint;

  boost::asio::ip::udp::resolver resolver;

  unsigned char querybuffer[ SRVDNSSENDRECEVIEVEBUFFSIZE ];
  int querylength;
  unsigned char answerbuffer[ SRVDNSSENDRECEVIEVEBUFFSIZE ];

  dnssrvrealm::pointer resolved;
  std::function< void ( dnssrvrealm::pointer ) > onresolve;

};


#endif  /* PROJECTSIPDNSSRVRESOLVER_H */
