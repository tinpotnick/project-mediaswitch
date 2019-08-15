
#include <iostream>

#include "projectrtpcodecx.h"

/*!md
# Project CODECs
This file is responsible for converting different types of rtppackets into different CODECs. It is fixed for now - for efficiency and simplicity. Perhaps in the future we could support more and pluggable CODECs.
*/

/*!md
Pre generate all 711 data.
We can speed up 711 conversion by way of pre calculating values. 128K(ish) of data is not too much to worry about!
*/

static uint8_t _l16topcmu[ 65536 ];
static uint8_t _l16topcma[ 65536 ];
static int16_t _pcmatol16[ 256 ];
static int16_t _pcmutol16[ 256 ];

/* Copied from the CCITT G.711 specification  - taken from spandsp*/
static const uint8_t ulaw_to_alaw_table[256] =
{
     42,  43,  40,  41,  46,  47,  44,  45,  34,  35,  32,  33,  38,  39,  36,  37,
     58,  59,  56,  57,  62,  63,  60,  61,  50,  51,  48,  49,  54,  55,  52,  53,
     10,  11,   8,   9,  14,  15,  12,  13,   2,   3,   0,   1,   6,   7,   4,  26,
     27,  24,  25,  30,  31,  28,  29,  18,  19,  16,  17,  22,  23,  20,  21, 106,
    104, 105, 110, 111, 108, 109,  98,  99,  96,  97, 102, 103, 100, 101, 122, 120,
    126, 127, 124, 125, 114, 115, 112, 113, 118, 119, 116, 117,  75,  73,  79,  77,
     66,  67,  64,  65,  70,  71,  68,  69,  90,  91,  88,  89,  94,  95,  92,  93,
     82,  82,  83,  83,  80,  80,  81,  81,  86,  86,  87,  87,  84,  84,  85,  85,
    170, 171, 168, 169, 174, 175, 172, 173, 162, 163, 160, 161, 166, 167, 164, 165,
    186, 187, 184, 185, 190, 191, 188, 189, 178, 179, 176, 177, 182, 183, 180, 181,
    138, 139, 136, 137, 142, 143, 140, 141, 130, 131, 128, 129, 134, 135, 132, 154,
    155, 152, 153, 158, 159, 156, 157, 146, 147, 144, 145, 150, 151, 148, 149, 234,
    232, 233, 238, 239, 236, 237, 226, 227, 224, 225, 230, 231, 228, 229, 250, 248,
    254, 255, 252, 253, 242, 243, 240, 241, 246, 247, 244, 245, 203, 201, 207, 205,
    194, 195, 192, 193, 198, 199, 196, 197, 218, 219, 216, 217, 222, 223, 220, 221,
    210, 210, 211, 211, 208, 208, 209, 209, 214, 214, 215, 215, 212, 212, 213, 213
};

/* These transcoding tables are copied from the CCITT G.711 specification. To achieve
   optimal results, do not change them. */

static const uint8_t alaw_to_ulaw_table[256] =
{
     42,  43,  40,  41,  46,  47,  44,  45,  34,  35,  32,  33,  38,  39,  36,  37,
     57,  58,  55,  56,  61,  62,  59,  60,  49,  50,  47,  48,  53,  54,  51,  52,
     10,  11,   8,   9,  14,  15,  12,  13,   2,   3,   0,   1,   6,   7,   4,   5,
     26,  27,  24,  25,  30,  31,  28,  29,  18,  19,  16,  17,  22,  23,  20,  21,
     98,  99,  96,  97, 102, 103, 100, 101,  93,  93,  92,  92,  95,  95,  94,  94,
    116, 118, 112, 114, 124, 126, 120, 122, 106, 107, 104, 105, 110, 111, 108, 109,
     72,  73,  70,  71,  76,  77,  74,  75,  64,  65,  63,  63,  68,  69,  66,  67,
     86,  87,  84,  85,  90,  91,  88,  89,  79,  79,  78,  78,  82,  83,  80,  81,
    170, 171, 168, 169, 174, 175, 172, 173, 162, 163, 160, 161, 166, 167, 164, 165,
    185, 186, 183, 184, 189, 190, 187, 188, 177, 178, 175, 176, 181, 182, 179, 180,
    138, 139, 136, 137, 142, 143, 140, 141, 130, 131, 128, 129, 134, 135, 132, 133,
    154, 155, 152, 153, 158, 159, 156, 157, 146, 147, 144, 145, 150, 151, 148, 149,
    226, 227, 224, 225, 230, 231, 228, 229, 221, 221, 220, 220, 223, 223, 222, 222,
    244, 246, 240, 242, 252, 254, 248, 250, 234, 235, 232, 233, 238, 239, 236, 237,
    200, 201, 198, 199, 204, 205, 202, 203, 192, 193, 191, 191, 196, 197, 194, 195,
    214, 215, 212, 213, 218, 219, 216, 217, 207, 207, 206, 206, 210, 211, 208, 209
};

void gen711convertdata( void )
{
  std::cout << "Pre generating G711 tables";
  for( int32_t i = 0; i != 65535; i++ )
  {
    int16_t l16val = i - 32768;
    _l16topcmu[ i ] = linear_to_ulaw( l16val );
    _l16topcma[ i ] = linear_to_alaw( l16val );
  }

  for( uint8_t i = 0; i != 255; i++ )
  {
    _pcmatol16[ i ] = alaw_to_linear( i );
    _pcmutol16[ i ] = ulaw_to_linear( i );
  }

  std::cout << " - completed." << std::endl;
}

codecx::codecx() :
  g722encoder( nullptr ),
  g722decoder( nullptr ),
  ilbcencoder( nullptr ),
  ilbcdecoder( nullptr ),
  resamplelastsample( 0 ),
  l168k( nullptr ),
  l1616k( nullptr ),
  l168kallocatedlength( 0 ),
  l1616kallocatedlength( 0 ),
  l168klength( 0 ),
  l1616klength( 0 )
{

}

codecx::~codecx()
{
  this->reset();
}

/*!md
## reset
Full reset - clear out all CODECs
*/
void codecx::reset()
{
  if( nullptr != this->g722decoder )
  {
    g722_decode_free( this->g722decoder );
    this->g722decoder = nullptr;
  }
  if( nullptr != this->g722encoder )
  {
    g722_encode_free( this->g722encoder );
    this->g722encoder = nullptr;
  }

  if( nullptr != this->ilbcdecoder )
  {
    WebRtcIlbcfix_DecoderFree( this->ilbcdecoder );
    this->ilbcdecoder = nullptr;
  }
  if( nullptr != this->ilbcencoder )
  {
    WebRtcIlbcfix_EncoderFree( this->ilbcencoder );
    this->ilbcencoder = nullptr;
  }

  if( 0 != this->l168kallocatedlength && nullptr != this->l168k )
  {
    delete[] this->l168k;
    this->l168kallocatedlength = 0;
    this->l168k = nullptr;
  }

  if( 0 != this->l1616kallocatedlength && nullptr != this->l1616k )
  {
    delete[] this->l1616k;
    this->l1616kallocatedlength = 0;
    this->l1616k = nullptr;
  }

  this->restart();

}

/*!md
## restart
Do enough to manage missing packets.
*/
void codecx::restart( void )
{
  this->lpfilter.reset();
  this->resamplelastsample = 0;
}

/*!md
## xlaw2ylaw

From whichever PCM encoding (u or a) encode to the other without having to do intermediate l16.

TODO - check that using a lookup table instead of a funcion call uses SSE functions in the CPU.
*/
void codecx::xlaw2ylaw( void )
{
  if( 0 == this->in.size() ) return;
  const uint8_t *convert;
  uint8_t *inbufptr, *outbufptr;
  size_t insize = this->in.size();

  if( PCMAPAYLOADTYPE == this->in.getformat() )
  {
    outbufptr = this->pcmuref.c_str();
    this->pcmuref.size( insize );
    convert = alaw_to_ulaw_table;
  }
  else
  {
    outbufptr = this->pcmaref.c_str();
    this->pcmaref.size( insize );
    convert = ulaw_to_alaw_table;
  }
  
  inbufptr = this->in.c_str();
  for( size_t i = 0; i < insize; i++ )
  {
    *outbufptr = convert[ *inbufptr ];

    outbufptr++;
    inbufptr++;
  }
}

/*!md
## allocatel168k
Allocate buffer if needed.
*/
void codecx::allocatel168k( std::size_t len )
{
  if( len > this->l168kallocatedlength )
  {
    if( 0 != this->l168kallocatedlength )
    {
      delete[] this->l168k;
    }

    this->l168k = new int16_t[ len ];
    this->l168kallocatedlength = len;
  }
}

/*!md
## allocatel1616k
Allocate buffer if needed.
*/
void codecx::allocatel1616k( std::size_t len )
{
  if( len > this->l1616kallocatedlength )
  {
    if( 0 != this->l1616kallocatedlength )
    {
      delete[] this->l1616k;
    }

    this->l1616k = new int16_t[ len ];
    this->l1616kallocatedlength = len;
  }
}

/*!md
## g711tol16
As it says.
*/
void codecx::g711tol16( void )
{
  this->allocatel168k( this->in.size() );

  int16_t *convert;
  uint8_t pt = this->in.getformat();

  if( PCMAPAYLOADTYPE == pt )
  {
    convert = _pcmatol16;
  }
  else if( PCMUPAYLOADTYPE == pt )
  {
    convert = _pcmutol16;
  }
  else
  {
    return;
  }

  uint8_t *in;
  int16_t *out;
  in = this->in.c_str();
  out = this->l168k;

  size_t insize = this->in.size();
  for( size_t i = 0; i < insize; i++ )
  {
    *out = convert[ *in ];
    in++;
    out++;
  }

  this->l168klength = insize;
}


/*!md
## l16topcma
*/
void codecx::l16topcma( void )
{
  uint8_t *out = this->pcmaref.c_str();;

  int16_t *in;
  in = this->l168k;

  for( int i = 0; i < this->l168klength; i++ )
  {
    *out = _l16topcma[ ( *in ) + 32768 ];

    in++;
    out++;
  }
}

/*!md
## l16topcmu
*/
void codecx::l16topcmu( void )
{
  uint8_t *out = this->pcmuref.c_str();;

  int16_t *in;
  in = this->l168k;

  for( int i = 0; i < this->l168klength; i++ )
  {
    *out = _l16topcmu[ ( *in ) + 32768 ];

    in++;
    out++;
  }
}

/*!md
## ilbctol16
As it says.
*/
void codecx::ilbctol16( void )
{
  /* roughly compression size with some leg room. */
  this->allocatel168k( this->in.size() * 5 );

  WebRtc_Word16 speechType;

  if( nullptr == this->ilbcdecoder )
  {
    /* Only support 20mS ATM to make the mixing simpler */
    WebRtcIlbcfix_DecoderCreate( &this->ilbcdecoder );
    WebRtcIlbcfix_DecoderInit( this->ilbcdecoder, 20 );
  }

  this->l168klength = WebRtcIlbcfix_Decode( this->ilbcdecoder,
                        ( WebRtc_Word16* ) this->in.c_str(),
                        this->in.size(),
                        this->l168k,
                        &speechType
                      );

  if( -1 == this->l168klength )
  {
    this->l168klength = 0;
  }
}

/*!md
## l16tog722
As it says.
*/
void codecx::l16tog722( void )
{
  if( nullptr == this->l1616k )
  {
    return;
  }

  if( nullptr == this->g722encoder )
  {
    this->g722encoder = g722_encode_init( NULL, 64000, G722_PACKED );
  }

  int len = g722_encode( this->g722encoder, this->g722ref.c_str(), this->l1616k, this->g722ref.size() * 2 );

  if( len > 0 )
  {
    this->g722ref.size( len );
  }
  else
  {
    this->g722ref.size( 0 );
  }
}

/*!md
## l16toilbc
As it says.
I think we can always send as 20mS.
According to RFC 3952 you determin how many frames are in the packet by the frame length. This is also true (although not explicit) that we can send as 20mS if our source is that but also 30mS if that is the case.

As we only support G722, G711 and iLBC (20/30) then we should be able simply encode and send as the matched size.
*/
void codecx::l16toilbc( void )
{
  if( nullptr == this->l168k )
  {
    return;
  }

  if( nullptr == this->ilbcencoder )
  {
    /* Only support 20mS ATM to make the mixing simpler */
    WebRtcIlbcfix_EncoderCreate( &this->ilbcencoder );
    WebRtcIlbcfix_EncoderInit( this->ilbcencoder, 20 );
  }

  WebRtc_Word16 len = WebRtcIlbcfix_Encode( this->ilbcencoder,
                            this->l168k,
                            this->l168klength,
                            ( WebRtc_Word16* ) this->ilbcref.c_str()
                          );
  if ( len > 0 )
  {
    this->ilbcref.size( len );
  }
  else
  {
    this->ilbcref.size( 0 );
  }
}


/*!md
## g722tol16
As it says.
*/
void codecx::g722tol16( void )
{
  this->allocatel1616k( this->in.size() * 2 );

  if( nullptr == this->g722decoder )
  {
    this->g722decoder = g722_decode_init( NULL, 64000, G722_PACKED );
  }

  this->l1616klength = g722_decode( g722decoder, this->l1616k, this->in.c_str(), this->in.size() );
}

/*!md
## l16lowtowideband
Upsample from narrow to wideband. Take each point and interpolate between them. We require the final sample from the last packet to continue the interpolating.
*/
void codecx::l16lowtowideband( void )
{
  if( nullptr == this->l168k )
  {
    return;
  }

  this->allocatel1616k( this->l168klength * 2 );

  int16_t *in = this->l168k;
  int16_t *out = this->l1616k;

  for( int i = 0; i < this->l168klength; i++ )
  {
    *out = ( ( *in - this->resamplelastsample ) / 2 ) + this->resamplelastsample;
    this->resamplelastsample = *in;
    out++;

    *out = *in;

    out++;
    in++;
  }
  this->l1616klength = this->l168klength * 2;
}

/*!md
##  l16widetolowband
Downsample our L16 wideband samples to 8K. Pass through filter then grab every other sample.
*/
void codecx::l16widetonarrowband( void )
{
  /* if odd need ceil instead of floor (the + 1 bit) */
  this->allocatel168k( ( this->l1616klength + 1 ) / 2 );

  int16_t *out = this->l168k;
  int16_t *in = this->l1616k;

  int16_t filteredval;
  for( int i = 0; i < this->l1616klength; i++ )
  {
    filteredval = lpfilter.execute( *in );
    in++;

    if( 0x01 == ( i & 0x01 ) )
    {
      *out = filteredval;
      out++;
    }
  }

  this->l168klength = this->l1616klength / 2;
}

/*!md
Try to simplify the code. Use the operator << to take in data and take out data.

codecx << rtpacket places data into our codec.

rtppacket << codecx takes data out.

We pass a packet in, then we can take multiple out - i.e. we may want different destinations with different (or the same) CODECs.

Have a think about if this is where we want to mix audio data.
*/
codecx& operator << ( codecx& c, rtppacket& pk )
{
  c.in = rawsound( pk );

  c.pcmaref = rawsound();
  c.pcmuref = rawsound();
  c.g722ref = rawsound();
  c.ilbcref = rawsound();

  c.l1616klength = 0;
  c.l168klength = 0;

  return c;
}

codecx& operator << ( codecx& c, rawsound& raw )
{
  c.in = raw;

  c.pcmaref = rawsound();
  c.pcmuref = rawsound();
  c.g722ref = rawsound();
  c.ilbcref = rawsound();

  c.l1616klength = 0;
  c.l168klength = 0;

  return c;
}

/*!md
## rtppacket << codecx
Take the data out and transcode if necessary. Keep reference to any packet we have transcoded as we may need to use it again.
*/
rtppacket& operator << ( rtppacket& pk, codecx& c )
{
  /* This shouldn't happen. */
  if( 0 == c.in.size() )
  {
    pk.length = 0;
    return pk;
  }

  int inpayloadtype = c.in.getformat();
  int outpayloadtype = pk.getpayloadtype();

  if( inpayloadtype == outpayloadtype )
  {
    pk.copy( c.in.c_str(), c.in.size() );
    return pk;
  }

  /* If we have already converted this packet... */
  if( PCMAPAYLOADTYPE == outpayloadtype &&  0 != c.pcmaref.size() )
  {
    pk.copy( c.pcmaref.c_str(), c.pcmaref.size() );
    return pk;
  }

  if( PCMUPAYLOADTYPE == outpayloadtype && 0 != c.pcmuref.size() )
  {
    pk.copy( c.pcmuref.c_str(), c.pcmuref.size() );
    return pk;
  }

  if( ILBCPAYLOADTYPE == outpayloadtype && 0 != c.ilbcref.size() )
  {
    pk.copy( c.ilbcref.c_str(), c.ilbcref.size() );
    return pk;
  }

  if( G722PAYLOADTYPE == outpayloadtype && 0 != c.g722ref.size() )
  {
    pk.copy( c.g722ref.c_str(), c.g722ref.size() );
    return pk;
  }

  /*
    Two stage, convert input to L16 (if required (or special case for PCMU(A) ) ), second stage then l16 to target
  */
  switch( inpayloadtype )
  {
    case PCMAPAYLOADTYPE:
    case PCMUPAYLOADTYPE:
    {
      switch( outpayloadtype )
      {
        /* special case */
        case PCMAPAYLOADTYPE:
        {
          c.xlaw2ylaw();
          pk.copy( c.pcmaref.c_str(), c.pcmaref.size() );
          return pk;
        }
        case PCMUPAYLOADTYPE:
        {
          c.xlaw2ylaw();
          pk.copy( c.pcmuref.c_str(), c.pcmuref.size() );
          return pk;
        }
        default:
        {
          if( 0 == c.l168klength )
          {
            c.g711tol16();
          }
          break;
        }
      }
      break;
    }
    case ILBCPAYLOADTYPE:
    {
      if( 0 == c.l168klength )
      {
        c.ilbctol16();
      }
      break;
    }
    case G722PAYLOADTYPE:
    {
      if( 0 == c.l1616klength )
      {
        c.g722tol16();
      }
      break;
    }
    case L16PAYLOADTYPE:
    {
      size_t insize = c.in.size();
      if( 0 != insize )
      {
        if( 8000 == c.in.getsamplerate() )
        {
          c.allocatel168k( c.in.size() );
          memcpy( c.l168k, c.in.c_str(), c.in.size() );
          c.l168klength = c.in.size() / 2;
        }
        else if( 16000 == c.in.getsamplerate() )
        {
          c.allocatel1616k( c.in.size() );
          memcpy( c.l1616k, c.in.c_str(), c.in.size() );
          c.l1616klength = c.in.size() / 2;
        }
      }

      break;
    }
    default:
    {
      /* should not get here */
      return pk;
    }
  }

  /* If we get here we may have L16 but at the wrong sample rate so check and resample - then convert */
  /* narrowband targets */
  switch( outpayloadtype )
  {
    case ILBCPAYLOADTYPE:
    {
      if( 0 == c.l168klength && 0 != c.l1616klength )
      {
        c.l16widetonarrowband();
      }
      c.ilbcref = rawsound( pk );
      c.l16toilbc();
      pk.setpayloadlength( c.ilbcref.size() );
      break;
    }
    case G722PAYLOADTYPE:
    {
      if( 0 == c.l1616klength && 0 != c.l168klength )
      {
        c.l16lowtowideband();
      }
      c.g722ref = rawsound( pk );
      c.l16tog722();
      pk.setpayloadlength( c.g722ref.size() );
      break;
    }
    case PCMAPAYLOADTYPE:
    {
      if( 0 == c.l168klength && 0 != c.l1616klength )
      {
        c.l16widetonarrowband();
      }
      c.pcmaref = rawsound( pk );
      c.l16topcma();
      pk.setpayloadlength( c.pcmaref.size() );
      break;
    }
    case PCMUPAYLOADTYPE:
    {
      if( 0 == c.l168klength && 0 != c.l1616klength )
      {
        c.l16widetonarrowband();
      }
      c.pcmuref = rawsound( pk );
      c.l16topcmu();
      pk.setpayloadlength( c.pcmuref.size() );
      break;
    }
  }

  return pk;
}


/*!md
# rawsound
An object representing raw data for sound - which can be in any format (supported). We maintain a pointer to the raw data and will not clean it up.
*/
rawsound::rawsound() :
  data( nullptr ),
  length( 0 ),
  format( 0 ),
  samplerate( 0 )
{
}

/*!md
# rawsound
*/
rawsound::rawsound( uint8_t *ptr, std::size_t length, int format, uint16_t samplerate ) :
  data( ptr ),
  length( length ),
  format( format ),
  samplerate( samplerate )
{
}

/*!md
## rawsound
Construct from an rtp packet
*/
rawsound::rawsound( rtppacket& pk )
{
  this->data = pk.getpayload();
  this->length = pk.getpayloadlength();
  this->format = pk.getpayloadtype();

  switch( pk.getpayloadtype() )
  {
    case PCMUPAYLOADTYPE:
    case PCMAPAYLOADTYPE:
    {
      this->samplerate = 8000;
      break;
    }
    case G722PAYLOADTYPE:
    {
      this->samplerate = 16000;
      break;
    }
    case ILBCPAYLOADTYPE:
    {
      this->samplerate = 8000;
      break;
    }
  }
}
