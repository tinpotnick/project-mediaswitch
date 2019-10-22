

#include "projectsippacket.h"
#include "projectsipendpoint.h"


#ifndef PROJECTSIPEVENTSTATE_H
#define PROJECTSIPEVENTSTATE_H

class projectsipeventsubscription :
  public projectsipendpoint
{
public:
  static bool handlesippacket( projectsippacket::pointer pk );
  static void sendok( projectsippacket::pointer pk );
  static void sendnotify( projectsippacket::pointer pk );

  /* Our state */
  std::function <void ( projectsippacket::pointer pk ) > state;

};

#endif /* PROJECTSIPEVENTSTATE_H */