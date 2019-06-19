
#include <iostream>
#include <boost/bind.hpp>


#include "projectrtpmixer.h"

/*!md
## C'stor
*/
projectrtpmixer::projectrtpmixer( boost::asio::io_service &workerservice ) :
  workerservice( workerservice ),
  timer( workerservice ),
  timestamp( 0 ),
  writercount( 0 ),
  readercount( 0 ),
  channelupdates( WORKERBACKLOGSIZE ),
  active( true )
{
}

/*!md
## Destructor
*/
projectrtpmixer::~projectrtpmixer()
{
  this->timer.cancel();
}

/*!md
# create

*/
projectrtpmixer::pointer projectrtpmixer::create( boost::asio::io_service &workerservice )
{
  return pointer( new projectrtpmixer( workerservice ) );
}

/*
## addchannel
Add a channel to this mixer (aka bridge). On a schedule we fetch data from our RTP channels, mix as appropriate and forward on to other channels in this mixer.

The array is an array of channel uuids.
*/
void projectrtpmixer::addchannel( projectrtpchannel::pointer channel )
{
  this->channelupdates.push( channel );
}

/*!md
## run
Kick off our timer
*/
void projectrtpmixer::run( void )
{
  this->timer.expires_after( boost::asio::chrono::milliseconds( 10 ) );
  this->timer.async_wait( boost::bind(
                    &projectrtpmixer::ontimer,
                    shared_from_this(),
                    boost::asio::placeholders::error ) );
}

/*!md
## stop
Stop our timer - loosing all references to tidy up.
*/
void projectrtpmixer::stop( void )
{
  this->active = false;
}

/*!md
## ontimer
Our interval timer. Check for new data. Mix (if required), send on. This is (very likely) to be called by a seperate thread to that of the control thread (our web server). So we have to ensure wht we access here is thread safe.

We have a window. If we have a channel.

now         6 packets ago
|t=5|t=4|t=3|t=2|t=1|t=0|
 |
 Write in here
  |
  When complete (all writers have received a packet for this slot) we can send on where required
                      |
                      If we require reordering then we pull out from oldest OR complete in order regardless of complete or not.
                      So if slot 6, 5 and 4 are all complete then we can write to any require in order channels.

*/
void projectrtpmixer::ontimer( const boost::system::error_code& e )
{
  if ( e != boost::asio::error::operation_aborted )
  {
    if( !this->active )
    {
      return;
    }

    /*
      this update block may not be the most efficient. research in the future
    */
    projectrtpchannel::pointer chupdate;
    while( this->channelupdates.pop( chupdate ) )
    {
      bool found = false;
      for( projectrtpchannels::iterator check = this->channels.begin();
            check != this->channels.end();
            check++ )
      {
        if( chupdate == *check )
        {
          found = true;
          break;
        }
        check++;
      }

      if( !found )
      {
        this->channels.push_back( chupdate );

        if( chupdate->canwrite() )
        {
          this->writercount++;
        }
        if( chupdate->canread() )
        {
          this->readercount++;
        }
      }
    }


#warning TODO - this is problematic
    /* remove closed channels TODO */
    projectrtpchannels::iterator chit = this->channels.begin();
    while( chit != this->channels.end() )
    {
      if( !( *chit )->isactive() )
      {
        chit = this->channels.erase( chit );
        continue;
      }
      chit++;
    }

    if( 0 == this->channels.size() )
    {
      /* timer will no longer be ticking */
      return;
    }

    /*
      We have completed any update on our channel definition. Now onto the mixing.
    */

    /* different strategies */
    if( 1 == this->writercount && 1 == this->readercount )
    {
      rtppacket *pk = this->channels[ 0 ]->pop();
      if( pk )
      {
        this->channels[ 0 ]->writepacket( pk );
      }
    }

    else if( 2 == this->writercount && 2 == this->readercount )
    {
      /* A simple to channel bridge/relay */
      rtppacket *pka = this->channels[ 0 ]->pop();
      rtppacket *pkb = this->channels[ 1 ]->pop();

      if( pka )
      {
        this->channels[ 1 ]->writepacket( pka );
      }

      if( pkb )
      {
        this->channels[ 0 ]->writepacket( pkb );
      }
    }

    this->timer.expires_at( this->timer.expiry() + boost::asio::chrono::milliseconds( 10 ) );
    timestamp += 10;
    this->timer.async_wait( boost::bind(
                      &projectrtpmixer::ontimer,
                      shared_from_this(),
                      boost::asio::placeholders::error ) );
  }
}


/*!md
## record - record to a filename.
*/
void projectrtpmixer::record( std::string filename )
{

}
