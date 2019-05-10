

#include "projectsipconfig.h"
#include "projectsipsm.h"
#include "projectsipregistrar.h"
#include "projectsipdialog.h"
#include "projectsipdirectory.h"

/* Our one instalce. */
static projectsipsm sipsm;


/*******************************************************************************
Function: projectsipsm
Purpose: Constructor.
Updated: 08.01.2019
*******************************************************************************/
projectsipsm::projectsipsm()
{
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
    case projectsippacket::CANCEL:
    case projectsippacket::INVITE:
    {
      sipsm.handleinvite( pk );
      break;
    }
    case projectsippacket::OPTIONS:
    {
      sipsm.handleoptions( pk );
      break;
    }
    case projectsippacket::BYE:
    case projectsippacket::RESPONSE:
    case projectsippacket::ACK:
    {
      sipsm.handleresponse( pk );
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
  response.addviaheader( projectsipconfig::gethostname(), pk.get() );

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
Purpose: As it says. RFC 3261 section 10. We do not support anonymous
registrations.
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

  if ( 0 >= pk->getheader( projectsippacket::Max_Forwards ).toint() )
  {
    return;
  }

  projectsipregistration::registrarsippacket( pk );
}


/*******************************************************************************
Function: handleinvite
Purpose: Handle INVITE
Updated: 23.12.2018
*******************************************************************************/
void projectsipsm::handleinvite( projectsippacketptr pk )
{
  /* Required headers */
  if( false == pk->hasheader( projectsippacket::To ) ||
      false == pk->hasheader( projectsippacket::From ) ||
      false == pk->hasheader( projectsippacket::Call_ID ) ||
      false == pk->hasheader( projectsippacket::CSeq ) ||
      /*false == pk->hasheader( projectsippacket::Contact ) || - not sure if we can do this, but zoiper does not send this on cancel*/
      false == pk->hasheader( projectsippacket::Via ) )
  {
    return;
  }

  if ( 0 >= pk->getheader( projectsippacket::Max_Forwards ).toint() )
  {
    return;
  }

  projectsipdialog::invitesippacket( pk );
}

/*******************************************************************************
Function: handleresponse
Purpose: Handle responses to our requests.
Updated: 23.12.2018
*******************************************************************************/
void projectsipsm::handleresponse( projectsippacketptr pk )
{
  if( false == projectsipdialog::invitesippacket( pk ) )
  {
    projectsipregistration::registrarsippacket( pk );
  }
}
