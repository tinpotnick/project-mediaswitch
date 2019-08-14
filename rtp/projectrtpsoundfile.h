
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

/*!md
Read and write sound file formats.

For playback efficiency we should only support raw sound file formats. Space is cheap, CPU is expensive. Playback should find the correct file, or the closest. We may handle this in different layer - to be decided.
We support:

L16 8K
L16 16K
PCMA
PCMU
G722
iLBC
wav

We have to provide a util to convert files to these formats. Playing files must be in mono(?) but we can record in sterio (is there any possible use of playing in sterio?).

file.l168k
file.l1616k
file.pcma
file.pcmu
file.g722
file.ilbc
file.wav

When the filename is l168 then this is raw data which is the most efficient. Channels can easily swap between formats.
We will only support the formats above in a wav file. So wav containing pcm, pcma, pcmu, 722, ilbc are all acceptable.

As well as the above file formats, we will support file.playlist - the is designed for music on hold. We should also support file1.wav,file2.wav as the filename which produces a playlist on the fly.

Recording files probably should be in wav file format?
http://soundfile.sapp.org/doc/WaveFormat/
https://www.daniweb.com/programming/software-development/threads/372867/how-to-write-wav-files

For a reference on how to store ulaw and alaw formats this document references them.
http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

Wav files appear ot be able to store L16 (PCM), G722, iLBC, PCMU and PCMA, ref:
http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Docs/Pages%20from%20mmreg.h.pdf

*/

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
  soundfile( std::string &filename, compression format = pcm, int mode = O_RDONLY, bandwidth bps = narrowband );
  ~soundfile();

  typedef boost::shared_ptr< soundfile > pointer;
  static pointer create( std::string &filename, compression format = pcm, int mode = O_RDONLY, bandwidth bps = narrowband );

  int read( rtppacket *pk );

private:
  int file;
  uint8_t *readbuffer;
  int mode;
  int blocksize;

  aiocb wavheader;
  aiocb wavblock;

};


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




void wavinfo( const char *file );
