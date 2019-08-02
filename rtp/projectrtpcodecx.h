

#ifndef PROJECTRTPCODECX_H
#define PROJECTRTPCODECX_H


#include <ilbc.h>
#include <spandsp.h>

#include "projectrtppacket.h"
#include "projectrtppacket.h"
#include "globals.h"
#include "firfilter.h"


class codecx
{
public:
  codecx();
  ~codecx();

  void reset( void );
  void restart( void );

  friend codecx& operator << ( codecx&, rtppacket& );
  friend rtppacket& operator << ( rtppacket&, codecx& );

private:
  void xlaw2ylaw( rtppacket &dst );
  void g711tol16( void );
  void ilbctol16( void );
  void g722tol16( void );
  void l16lowtowideband( void);
  void l16widetonarrowband( void );
  void l16tog711( rtppacket &dst );
  void l16tog722( rtppacket &dst );
  void l16toilbc( rtppacket &dst );

  /* CODECs  */
  g722_encode_state_t *g722encoder;
  g722_decode_state_t *g722decoder;

  iLBC_encinst_t *ilbcencoder;
  iLBC_decinst_t *ilbcdecoder;

  /* If we require downsampling */
  lowpass3_4k16k lpfilter;
  /* When we up sample we need to interpolate so need last sample */
  int16_t resamplelastsample;

  int16_t l168k[ RTPMAXLENGTH ]; /* narrow band */
  int16_t l1616k[ L16MAXLENGTH ]; /* wideband */

  int16_t l168klength;
  int16_t l1616klength;

  rtppacket *in;
  rtppacket *pcmaref;
  rtppacket *pcmuref;
  rtppacket *g722ref;
  rtppacket *ilbcref;


};


codecx& operator << ( codecx&, rtppacket& );
rtppacket& operator << ( rtppacket&, codecx& );

/* Functions */
void gen711convertdata( void );


#endif /* PROJECTRTPCODECX_H */