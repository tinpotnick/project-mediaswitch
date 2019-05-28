

#ifndef PROJECTSIPSM_H
#define PROJECTSIPSM_H

#include "projectsippacket.h"

/*******************************************************************************
Class: projectsipsm
Purpose: Manage our SIP state machine for all connections.
Updated: 17.12.2018
*******************************************************************************/
class projectsipsm
{
public:
  projectsipsm();
  static void handlesippacket( projectsippacket::pointer pk );

private:
  void handleregister( projectsippacket::pointer pk );
  void handleoptions( projectsippacket::pointer pk );
  void handleinvite( projectsippacket::pointer pk );
  void handleresponse( projectsippacket::pointer pk );
};



#endif  /* PROJECTSIPSM_H */
