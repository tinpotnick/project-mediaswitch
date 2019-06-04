



#include "projectsipendpoint.h"
#include "projectsipregistrar.h"
#include "projectsipdirectory.h"
#include "globals.h"

projectsipendpoint::projectsipendpoint() :
  errorcode( 0 ),
  resolver( io_service )
{

}

projectsipendpoint::~projectsipendpoint()
{
}

/*!md
# sendpk

Flow:
If we have received a packet from a client then we respond to that client. We prefer authenticate packets where we have recieved them.

If we haven't received a packet, then if it is a local user then we send through the registrar.

If it is not a local user, then we perform DNS lookups.

Even though a registrar also could be an end point, this would be a) circular dependancy (which is respolvable but untify) and b) there a couple of differences. If we end up supporting outbound registrations, then we may convert the registrar to endpoint and resolve the circular dependancy as it will be worth it for that.

This is also where we will add sending to the contact/via destination if we decide to allow rport to be diabled.

Q: how am I going to pass in the to user to this layer. It probbaly should come from the packet?
*/
bool projectsipendpoint::sendpk( projectsippacket::pointer pk )
{
  if( this->lastauthenticatedpk )
  {
    this->lastauthenticatedpk->respond( pk->strptr() );
    return true;
  }

  if( this->lastreceivedpk )
  {
    this->lastreceivedpk->respond( pk->strptr() );
    return true;
  }

  projectsipdirdomain::pointer todom = projectsipdirdomain::lookupdomain( pk->gethost() );
  if( todom )
  {
    std::string user = pk->getuser().str();

    if( todom->userexist( user ) )
    {
      std::string userhost = pk->getuserhost().str();
      if( projectsipregistration::sendtoregisteredclient( userhost, pk ) )
      {
        return true;
      }
      this->errorcode = 480;
      this->errorreason = "Temporarily Unavailable";
      return false;
    }
    this->errorcode = 404;
    this->errorreason = "To user not found";
    return false;
  }

  if( !this->srvresolver )
  {
    this->srvresolver = projectsipdnssrvresolver::create();
  }

  this->tosendpk = pk;

  this->srvresolver->query( std::string( "_sip._udp." ) + pk->gethost().str(),
                            std::bind( &projectsipendpoint::handlesrvresolve,
                              shared_from_this(),
                              std::placeholders::_1
                            ) );

  /* we can only destroy ourselves once the callback has happened. */
  return true;

}

/*!md
## handlesrvresolve
For now we pick out a SRV record and then go onto resolve that host. In the future we may (if no SRV records are returned) try and resolve the host on its own. I don't think that is the most correct thing to do though.
*/
void projectsipendpoint::handlesrvresolve( dnssrvrealm::pointer answer )
{
  if( answer )
  {
    this->srvrecord = answer->getbestrecord();
    if( this->srvrecord )
    {
      /*std::cout << answer->realm << std::endl;
      std::cout << this->srvrecord->host << std::endl;
      std::cout << this->srvrecord->priority << std::endl;
      std::cout << this->srvrecord->weight << std::endl;
      std::cout << this->srvrecord->port << std::endl;
      std::cout << this->srvrecord->ttl << std::endl;
      std::cout << this->srvrecord->port << std::endl;*/

      boost::asio::ip::udp::resolver::query query( boost::asio::ip::udp::v4(), this->srvrecord->host , std::to_string( this->srvrecord->port ) );
      this->resolver.async_resolve( query,
          boost::bind( &projectsipendpoint::handleresolve,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::iterator ) );
    }
  }
}

/*!md
## handleresolve
Resolve our host name (or not).
*/
void projectsipendpoint::handleresolve(
          boost::system::error_code e,
          boost::asio::ip::udp::resolver::iterator it )
{
  boost::asio::ip::udp::resolver::iterator end;
  if( it == end )
  {
    /* What to do? For now, allow the upper object to time out */
    return;
  }

  this->receiverendpoint = *it;

  sipserver->getsocket()->async_send_to(
              boost::asio::buffer( *this->tosendpk->strptr() ),
              this->receiverendpoint,
              boost::bind(
                &projectsipendpoint::handlesipendpointsend,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred) );
}

/*!md
## handlesipendpointsend
Once we have sent, wait for response.
*/
void projectsipendpoint::handlesipendpointsend(
                    const boost::system::error_code& error,
                    std::size_t bytes_transferred)
{
  /* Great! */
}
