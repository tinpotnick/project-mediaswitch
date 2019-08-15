

#ifndef PROJECTRTPSOUNDFILE_H
#define PROJECTRTPSOUNDFILE_H

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>


#include <sys/types.h>
#include <sys/stat.h>
#include <aio.h>
#include <errno.h>
#include <fcntl.h>

#include <stdint.h>

#include "projectrtppacket.h"
#include "projectrtpcodecx.h"

/*!md

*/

typedef struct
{
    /* RIFF Header */
    char riff_header[ 4 ]; /* Contains "RIFF" */
    int32_t wav_size; /* Size of the wav portion of the file, which follows the first 8 bytes. File size - 8 */
    char wave_header[ 4 ]; /* Contains "WAVE" */

    /* Format Header */
    char fmt_header[ 4 ]; /* Contains "fmt " (includes trailing space) */
    int32_t fmt_chunk_size; /* Should be 16 for PCM */
    int16_t audio_format; /* Should be 1 for PCM. 3 for IEEE Float */
    int16_t num_channels;
    int32_t sample_rate;
    int32_t byte_rate; /* Number of bytes per second. sample_rate * num_channels * Bytes Per Sample */
    int16_t sample_alignment; /* num_channels * Bytes Per Sample */
    int16_t bit_depth; /* Number of bits per sample */

    /* Data */
    char data_header[ 4 ]; /* Contains "data" */
    /* int32_t data_bytes;  Number of bytes in data. Number of samples * num_channels * sample byte size */
    /* Remainder of wave file is bytes */
} wav_header;

/* Actual value in the wav header */
#define  WAVE_FORMAT_PCM 0x0001
#define  WAVE_FORMAT_ALAW 0x0006
#define  WAVE_FORMAT_MULAW 0x0007
#define  WAVE_FORMAT_POLYCOM_G722 0xA112 /* Polycom - there are other versions */
#define  WAVE_FORMAT_GLOBAL_IP_ILBC 0xA116 /* Global IP */

class soundfile
{
public:
  enum compression { pcm, pcmu, pcma, g722, ilbc };
  enum bandwidth{ narrowband, wideband };
  soundfile( std::string &url );
  ~soundfile();

  typedef boost::shared_ptr< soundfile > pointer;
  static pointer create( std::string &url );

  rawsound read( void );

private:
  int file;
  uint8_t *readbuffer;
  int readbuffercount;
  int currentindex;
  int blocksize;
  aiocb cbwavheader;
  wav_header wavheader;
  aiocb cbwavblock;
};




void wavinfo( const char *file );


#endif /* PROJECTRTPSOUNDFILE_H */