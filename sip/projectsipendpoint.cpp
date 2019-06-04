



#include "projectsipendpoint.h"
#include "projectsipregistrar.h"
#include "projectsipdirectory.h"


projectsipendpoint::projectsipendpoint() :
  errorcode( 0 )
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
std::cout << "attempting resolve" << std::endl;
  if( !this->srvresolver )
  {
    this->srvresolver = projectsipdnssrvresolver::create();
  }
std::cout << "resolver created" << std::endl;
  this->srvresolver->query( std::string( "_sip._udp." ) + pk->gethost().str(),
                            std::bind( &projectsipendpoint::handlesrvresolve,
                              shared_from_this(),
                              std::placeholders::_1
                            ) );

std::cout << "query ran" << std::endl;

#warning TODO - DNS implimentation.
  /* Failing all of the above, we need perform DNS. */
  this->errorcode = 500;
  this->errorreason = "External calls not yet impl";

  /* Delay the release of this object in case we get a response from our DNS resolver */
  //this->locker = us.lock();

  // we can only destroy ourselves once the callback has happened.
  return false;

}

void projectsipendpoint::handlesrvresolve( dnssrvrealm::pointer answer )
{
  std::cout << "handlesrvresolve" << std::endl;
  if( answer )
  {
    std::cout << answer->realm << std::endl;

    dnssrvrecord::pointer record = answer->getbestrecord();
    if( record )
    {
      std::cout << record->host << std::endl;
      std::cout << record->priority << std::endl;
      std::cout << record->weight << std::endl;
      std::cout << record->port << std::endl;
      std::cout << record->ttl << std::endl;
      std::cout << record->port << std::endl;
    }
  }

  /* We should be ok to free us if we are no longer being tracked elsewhere */
  //this->locker.reset();
}
