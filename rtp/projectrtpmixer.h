


#ifndef PROJECTRTPMIXER_H
#define PROJECTRTPMIXER_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

#include <stdint.h>
#include <arpa/inet.h>

#include <atomic>
#include <boost/lockfree/stack.hpp>

#include <vector>

#include <boost/lockfree/stack.hpp>

#include "projectrtpchannel.h"

#warning TODO
/* TODO - what are these numbers */
#define PAYLOADTYPEULAW 0
#define PAYLOADTYPEALAW 8
#define PAYLOADTYPEG722 9
#define PAYLOADTYPEILBC 32

#define WORKERBACKLOGSIZE 500


typedef std::vector<projectrtpchannel::pointer> projectrtpchannels;

/*!md
# projectrtpmixer

This is a base class for different types of mixers. Our base class needs to know how to receive data (from a rtp channel). We will impliment basic functionality in this class including:

* Transcoding
* Recording

Child class then be created for different purposes. For example bridge and conference amongst others.

Write packet comes from a rtpchannel who writes a whole packet to the mixer. Data coming into the mixer may be out of order and have packets missing. So we are responcible in this class to reorder *if required*.

We may not need to reorder or other RTP type of functions as if all we do is pass back onto the network layer untouched then we can just seamlesly pass it through.

This is true if we have CODECS which are bound to a packet and will match any transcoding i.e. PCMA -> PCMU. Other types of CODEC (which I need to check if this is the case) there is not a 1 to 1 mapping so oing from PCMA -> iLBC we will very likley require ensuring order.

Call recording which also happens in this class will also need the packets re-ordering and pottentially transcoding.

We need a way of keeping mixers. RTP channels should reference the mixer they are writing to. A mixer then maintains a reference to a channel it is writing to.

We also have mixers for things like moh, files etc. Music on hold can stay alive whilst files need ot play from the begining for every file played.

Some mixers will get their timing from RTP source, some will get the timing from our own timer (i.e. playing  a file).

How are we going to represent multiple CODECS required by child classes that need more than one client served with different CODECS?
*/
class projectrtpmixer :
  public boost::enable_shared_from_this< projectrtpmixer >
{
public:
  projectrtpmixer( boost::asio::io_service &workerservice );
  virtual ~projectrtpmixer();

  typedef boost::shared_ptr< projectrtpmixer > pointer;
  static pointer create( boost::asio::io_service &workerservice );
  void run( void );
  void stop( void );

  void record( std::string filename );

  void addchannel( projectrtpchannel::pointer );
  void ontimer( const boost::system::error_code& e );

protected:
  /* to be overridden */
  virtual void onorderedrtpdata( uint8_t *pk, size_t length ) {}
  virtual void onrtpdata( uint8_t *pk, size_t length ) {}

  boost::asio::io_service &workerservice;
  boost::asio::steady_timer timer;


private:
  void l16toalaw();
  void alawtol16();
  void l16toulaw();
  void ulawtol16();
  void l16tog722();
  void g722tol16();
  void l16toilbc();
  void ilbctol16();

  /* It will wrap around in about 49 days */
  uint32_t timestamp;
  projectrtpchannels channels;

  int writercount;
  int readercount;

  boost::lockfree::stack< projectrtpchannel::pointer > channelupdates;
  std::atomic_bool active;
};

#endif /* PROJECTRTPMIXER_H */
