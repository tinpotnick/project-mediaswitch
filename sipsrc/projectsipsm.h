

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
  static void handlesippacket( projectsippacketptr pk );

private:
  void handleregister( projectsippacketptr pk );
  void handleoptions( projectsippacketptr pk );
};



#endif  /* PROJECTSIPSM_H */



