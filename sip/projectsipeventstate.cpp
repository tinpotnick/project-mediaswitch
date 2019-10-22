

#include "projectsipeventstate.h"
#include "projectsipconfig.h"

/*
   Watcher             Server                 PUA
      | F1 SUBSCRIBE      |                    |
      |------------------>|                    |
      | F2 200 OK         |                    |
      |<------------------|                    |
      | F3 NOTIFY         |                    |
      |<------------------|                    |
      | F4 200 OK         |                    |
      |------------------>|                    |
      |                   |                    |
      |                   |   Update presence  |
      |                   |<------------------ |
      |                   |                    |
      | F5 NOTIFY         |                    |
      |<------------------|                    |
      | F6 200 OK         |                    |
      |------------------>|                    |

Although, I have seen references to responding with 202 (accepted) rather than 200 ok. The above is in RFC 3856 and 3903
*/


bool projectsipeventsubscription::handlesippacket( projectsippacket::pointer pk )
{
  switch( pk->getmethod() )
  {
    case projectsippacket::SUBSCRIBE:
    {
      projectsipeventsubscription::sendok( pk );
      projectsipeventsubscription::sendnotify( pk );
      return true;
    }
    case projectsippacket::NOTIFY:
    {
      break;
    }
    case projectsippacket::PUBLISH:
    {
      break;
    }
  }
  projectsippacket notallowed;

  notallowed.setstatusline( 405, "Method Not Allowed" );
  notallowed.addcommonheaders();
  notallowed.addviaheader( projectsipconfig::gethostipsipport(), pk );

  notallowed.addheader( projectsippacket::To,
              pk->getheader( projectsippacket::To ) );
  notallowed.addheader( projectsippacket::From,
              pk->getheader( projectsippacket::From ) );
  notallowed.addheader( projectsippacket::Call_ID,
              pk->getheader( projectsippacket::Call_ID ) );
  notallowed.addheader( projectsippacket::CSeq,
              pk->getheader( projectsippacket::CSeq ) );
  notallowed.addheader( projectsippacket::Contact,
              projectsippacket::contact( pk->getuser().strptr(),
              stringptr( new std::string( projectsipconfig::gethostipsipport() ) ) ) );

  notallowed.addheader( projectsippacket::Content_Length,
                      "0" );

  pk->respond( notallowed.strptr() );

  return true;
}


void projectsipeventsubscription::sendok( projectsippacket::pointer pk )
{
  projectsippacket ok;

  ok.setstatusline( 200, "OK" );
  ok.addcommonheaders();
  ok.addviaheader( projectsipconfig::gethostipsipport(), pk );

  ok.addheader( projectsippacket::To,
              pk->getheader( projectsippacket::To ) );
  ok.addheader( projectsippacket::From,
              pk->getheader( projectsippacket::From ) );
  ok.addheader( projectsippacket::Call_ID,
              pk->getheader( projectsippacket::Call_ID ) );
  ok.addheader( projectsippacket::CSeq,
              pk->getheader( projectsippacket::CSeq ) );
  ok.addheader( projectsippacket::Contact,
              projectsippacket::contact( pk->getuser().strptr(),
              stringptr( new std::string( projectsipconfig::gethostipsipport() ) ) ) );

  ok.addheader( projectsippacket::Content_Length,
                      "0" );

  pk->respond( ok.strptr() );
}

void projectsipeventsubscription::sendnotify( projectsippacket::pointer pk )
{
  projectsippacket notify;

  notify.setrequestline( projectsippacket::NOTIFY, pk->getrequesturi().str() );
  notify.addcommonheaders();

  notify.addviaheader( projectsipconfig::gethostipsipport(), pk );

  notify.addheader( projectsippacket::To,
              pk->getheader( projectsippacket::To ) );
  notify.addheader( projectsippacket::From,
              pk->getheader( projectsippacket::From ) );
  notify.addheader( projectsippacket::Call_ID,
              pk->getheader( projectsippacket::Call_ID ) );
  notify.addheader( projectsippacket::CSeq,
              "1 NOTIFY" );
  notify.addheader( projectsippacket::Contact,
              projectsippacket::contact( pk->getuser().strptr(),
              stringptr( new std::string( projectsipconfig::gethostipsipport() ) ) ) );

  notify.addheader( projectsippacket::Content_Length,
                      "0" );

  pk->respond( notify.strptr() );
}