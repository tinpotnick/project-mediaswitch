

#include "projectsipsm.h"


/*******************************************************************************
Function: handlesippacket
Purpose: Handle a SIP packet.
Updated: 17.12.2018
*******************************************************************************/
void projectsipsm::handlesippacket( projectsippacket &pk )
{
  switch( pk.getmethod() )
  {
    case projectsippacket::REGISTER:
    {
      this->handleregister( pk );
      break;
    }
    case projectsippacket::INVITE:
    {
      break;
    }
    case projectsippacket::OPTIONS:
    {
      break;
    }
    case projectsippacket::ACK:
    {
      break;
    }
    case projectsippacket::CANCEL:
    {
      break;
    }
    case projectsippacket::BYE:
    {
      break;
    }
    case projectsippacket::RESPONCE:
    {
      break;
    }
  }
}

/*******************************************************************************
Function: handleregister
Purpose: As it says. RFC 3261 section 10.
Updated: 17.12.2018
*******************************************************************************/
void projectsipsm::handleregister( projectsippacket &pk )
{
  /* Required headers */
  if( false == pk.hasheader( projectsippacket::To ) ||
      false == pk.hasheader( projectsippacket::From ) ||
      false == pk.hasheader( projectsippacket::Call_ID ) ||
      false == pk.hasheader( projectsippacket::CSeq ) ||
      false == pk.hasheader( projectsippacket::Contact ) )
  {
    return;
  }

  stringptr uri = pk.getrequesturi();
  if( !uri || uri->size() < 4 )
  {
    return;
  }

  if( std::string::npos != uri->find( '@' ) )
  {
    return;
  }

}


