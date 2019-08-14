#ifndef PROJECTRTPGLOBALS_H
#define PROJECTRTPGLOBALS_H

#include <boost/asio.hpp>
#include <string>

/* The number of bytes in a packet ( these figure are less some overhead G711 = 172*/
#define G711PAYLOADBYTES 160
#define G722PAYLOADBYTES 160
#define L16NARROWBANDBYTES 160
#define L16WIDEBANDBYTES 320
#define ILBC20PAYLOADBYTES 38
#define ILBC30PAYLOADBYTES 50

#define PCMUPAYLOADTYPE 0
#define PCMAPAYLOADTYPE 8
#define G722PAYLOADTYPE 9
#define ILBCPAYLOADTYPE 97
/* RFC says this is 44k sampling, however it also seems a little ambiguous on this - so we generally are going to be using this so lets see where it goes! */
#define L16PAYLOADTYPE 11

/* Need to double check max RTP length with variable length header - there could be a larger length withour CODECs */
/* this maybe breached if a stupid number of csrc count is high */
#define RTPMAXLENGTH 200
#define L16MAXLENGTH ( RTPMAXLENGTH * 2 )
#define RTCPMAXLENGTH 200


/* Try not to use too many globals. Keep track of ones we do here */
/* ioservice is our main control thread - used for http control information, workerservice is the workload - rtp service */
extern boost::asio::io_service ioservice;
extern boost::asio::io_service workerservice;

extern std::string mediachroot;

#endif /* PROJECTRTPGLOBALS_H */
