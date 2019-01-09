
/* gethostname */
#include <unistd.h>

#include "projectsipsm.h"

static projectsipsm sipsm;


/*******************************************************************************
Function: projectsipsm
Purpose: Constructor.
Updated: 08.01.2019
*******************************************************************************/
projectsipsm::projectsipsm()
{
  char b[ 200 ];
  if( -1 == gethostname( b, sizeof( b ) ) )
  {
    this->hostname = "unknown";
    return;
  }
  this->hostname = b;
}

/*******************************************************************************
Function: handlesippacket
Purpose: Handle a SIP packet.
Updated: 17.12.2018
*******************************************************************************/
void projectsipsm::handlesippacket( projectsippacketptr pk )
{
  switch( pk->getmethod() )
  {
    case projectsippacket::REGISTER:
    {
      sipsm.handleregister( pk );
      break;
    }
    case projectsippacket::INVITE:
    {
      break;
    }
    case projectsippacket::OPTIONS:
    {
      sipsm.handleoptions( pk );
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
    case projectsippacket::RESPONSE:
    {
      break;
    }
  }
}

/*******************************************************************************
Function: handleoptions
Purpose: As it says. RFC 3261 section 11.
Updated: 03.01.2019
*******************************************************************************/
void projectsipsm::handleoptions( projectsippacketptr pk )
{
    /* Required headers - section 8.1.1 */
  if( false == pk->hasheader( projectsippacket::To ) ||
      false == pk->hasheader( projectsippacket::From ) ||
      false == pk->hasheader( projectsippacket::CSeq ) ||
      false == pk->hasheader( projectsippacket::Call_ID ) ||
      false == pk->hasheader( projectsippacket::Max_Forwards ) ||
      false == pk->hasheader( projectsippacket::Via ) )
  {
    return;
  }

  if ( 0 >= pk->getheader( projectsippacket::Max_Forwards ).toint() )
  {
    return;
  }

  projectsippacket response;

  response.setstatusline( 200, "OK" );
  response.addviaheader( this->hostname.c_str(), pk.get() );

  response.addheader( projectsippacket::To,
                      pk->getheader( projectsippacket::To ) );
  response.addheader( projectsippacket::From,
                      pk->getheader( projectsippacket::From ) );
  response.addheader( projectsippacket::Call_ID,
                      pk->getheader( projectsippacket::Call_ID ) );
  response.addheader( projectsippacket::CSeq,
                      pk->getheader( projectsippacket::CSeq ) );
  response.addheader( projectsippacket::Contact,
                      pk->getheader( projectsippacket::Contact ) );
  response.addheader( projectsippacket::Allow,
                      "INVITE, ACK, CANCEL, OPTIONS, BYE" );
  response.addheader( projectsippacket::Content_Type,
                      "application/sdp" );
  response.addheader( projectsippacket::Content_Length,
                      "0" );

  pk->respond( response.strptr() );
}

/*******************************************************************************
Function: handleregister
Purpose: As it says. RFC 3261 section 10.
Updated: 17.12.2018
*******************************************************************************/
void projectsipsm::handleregister( projectsippacketptr pk )
{
  /* Required headers */
  if( false == pk->hasheader( projectsippacket::To ) ||
      false == pk->hasheader( projectsippacket::From ) ||
      false == pk->hasheader( projectsippacket::Call_ID ) ||
      false == pk->hasheader( projectsippacket::CSeq ) ||
      false == pk->hasheader( projectsippacket::Contact ) ||
      false == pk->hasheader( projectsippacket::Via ) )
  {
    return;
  }

  if( false == pk->hasheader( projectsippacket::Authorization ) )
  {
    response.setstatusline( 401, "Unauthorized" );
    response.addviaheader( this->hostname.c_str(), pk.get() );

    response.addheader( projectsippacket::To,
                        pk->getheader( projectsippacket::To ) );
    response.addheader( projectsippacket::From,
                        pk->getheader( projectsippacket::From ) );
    response.addheader( projectsippacket::Call_ID,
                        pk->getheader( projectsippacket::Call_ID ) );
    response.addheader( projectsippacket::CSeq,
                        pk->getheader( projectsippacket::CSeq ) );
    response.addheader( projectsippacket::Contact,
                        pk->getheader( projectsippacket::Contact ) );
    response.addheader( projectsippacket::Allow,
                        "INVITE, ACK, CANCEL, OPTIONS, BYE" );
    response.addheader( projectsippacket::Content_Type,
                        "application/sdp" );
    response.addheader( projectsippacket::Content_Length,
                        "0" );

    pk->respond( response.strptr() );
    return;
  }

  stringptr uri = pk->getrequesturi();
  if( !uri || uri->size() < 4 )
  {
    return;
  }

  int expires = DEFAULTSIPEXPIRES;
  substring ex;
  if( pk->hasheader( projectsippacket::Expires ) )
  {
    ex = pk->getheader( projectsippacket::Expires );
  }
  else
  {
    ex = pk->getheaderparam( projectsippacket::Contact, "expires" );
  }

  if( 0 != ex.end() )
  {
    expires = ex.toint();
    if( -1 == expires )
    {
      expires = DEFAULTSIPEXPIRES;
    }
  }
}


