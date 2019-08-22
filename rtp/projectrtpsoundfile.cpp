
#include <iostream>

#include "projectrtpsoundfile.h"
#include "globals.h"

#include "projectwebdocument.h"


/*!md
## soundfile
Time to simplify. We will read wav files - all should be in pcm format - either wideband or narrow band. Anything else we will throw out. In the future we may support pre-encoded - but for now...

We need to support
* read and write (play and record)
* whole read and store in memory (maybe)
* looping a file playback (this is effectively moh)
* multiple readers (of looped files only - equivalent of moh)
* virtual files (i.e. think tone://) ( think tone_stream in FS - work on real first)
* Need to review: https://www.itu.int/dms_pub/itu-t/opb/sp/T-SP-E.180-2010-PDF-E.pdf
* Also: TGML - https://freeswitch.org/confluence/display/FREESWITCH/TGML I think a simpler version may be possible
*/
soundfile::soundfile( std::string &url ) :
  file( -1 ),
  url( url ),
  readbuffercount( 2 ),
  currentindex( 0 ),
  opened (false ),
  badheader( false )
{

  httpuri uriparts( substring( url.c_str() ) );
  if( uriparts.protocol == "file" ) 
  {
    int mode = O_RDONLY;
    std::string filenfullpath = mediachroot + uriparts.host.str();

    this->file = open( filenfullpath.c_str(), mode | O_NONBLOCK, 0 );
    if ( -1 == this->file )
    {
      /* Not much more we can do */
      return;
    }

    /* 
      Soundfile blindly reads the format and passes to the codec - so it must be in a format we support - or there will be silence.

      Our macro player (to be written) will choose the most appropriate file to play based on the codec of the channel.
    */
    this->readbuffer = new uint8_t[ L16WIDEBANDBYTES * this->readbuffercount ];

    /* As it is asynchronous - we read wav header + ahead */
    memset( &this->cbwavheader, 0, sizeof( aiocb ) );
    this->cbwavheader.aio_nbytes = sizeof( wav_header );
    this->cbwavheader.aio_fildes = file;
    this->cbwavheader.aio_offset = 0;
    this->cbwavheader.aio_buf = &this->wavheader;

    memset( &this->cbwavblock, 0, sizeof( aiocb ) );
    this->cbwavblock.aio_nbytes = L16WIDEBANDBYTES;
    this->cbwavblock.aio_fildes = file;
    this->cbwavblock.aio_offset = sizeof( wav_header );
    this->cbwavblock.aio_buf = this->readbuffer;

    /* read */
    if ( aio_read( &this->cbwavheader ) == -1 )
    {
      /* report error somehow. */
      close( this->file );
      this->file = -1;
      return;
    }

    if ( aio_read( &this->cbwavblock ) == -1 )
    {
      /* report error somehow. */
      close( this->file );
      this->file = -1;
      return;
    }
  }

	return;

}

/*!md
# d-stor
Clean up.
*/
soundfile::~soundfile()
{
  if( nullptr != this->readbuffer )
  {
    delete[] this->readbuffer;
  }

  if ( -1 != this->file )
  {
    close( this->file );
  }
}

/*!md
# create
Shared pointer version of us.
*/
soundfile::pointer soundfile::create( std::string &url )
{
  return pointer( new soundfile( url ) );
}

/*
## read
Asynchronous read.

Return the number of bytes read. Will read the appropriate amount of bytes for 1 rtp packet for the defined CODEC.
If not ready return -1.

We only support 1 channel. Anything else we need to look at.
*/
rawsound soundfile::read( void )
{
  if( aio_error( &this->cbwavblock ) == EINPROGRESS )
  {
    return rawsound();
  }

  /* success? */
	int numbytes = aio_return( &this->cbwavblock );

  if( -1 == numbytes || 0 == numbytes )
  {
    return rawsound();
  }

  this->opened = true;
  if( 'W' != this->wavheader.wave_header[ 0 ] )
  {
    this->badheader = true;
  }
  this->badheader = false;


  int ploadtype = L16PAYLOADTYPE;
  int blocksize = L16WIDEBANDBYTES;
  switch( this->wavheader.audio_format )
  {
    case WAVE_FORMAT_PCM:
    {
      if( 8000 == this->wavheader.sample_rate )
      {
        blocksize = L16NARROWBANDBYTES;
      }
      break;
    }
    case WAVE_FORMAT_ALAW:
    {
      ploadtype = PCMAPAYLOADTYPE;
      blocksize = G711PAYLOADBYTES;
      break;
    }
    case WAVE_FORMAT_MULAW:
    {
      ploadtype = PCMUPAYLOADTYPE;
      blocksize = G711PAYLOADBYTES;
      break;
    }
    case WAVE_FORMAT_POLYCOM_G722:
    {
      ploadtype = G722PAYLOADTYPE;
      blocksize = G722PAYLOADBYTES;
      break;
    }
    case WAVE_FORMAT_GLOBAL_IP_ILBC:
    {
      ploadtype = ILBCPAYLOADTYPE;
      blocksize = ILBC20PAYLOADBYTES;
      break;
    }
  }
  
  uint8_t *current = ( uint8_t * ) this->cbwavblock.aio_buf;

  this->currentindex = ( this->currentindex + 1 ) % readbuffercount;
  this->cbwavblock.aio_buf = this->readbuffer + ( blocksize * this->currentindex );
  this->cbwavblock.aio_offset += blocksize;
  this->cbwavblock.aio_nbytes = blocksize;

  if( this->cbwavblock.aio_offset > ( __off_t ) ( this->wavheader.wav_size + sizeof( wav_header ) ) )
  {
    this->cbwavblock.aio_offset = sizeof( wav_header );
  }

  /* read next block */
  if ( aio_read( &this->cbwavblock ) == -1 )
  {
    /* report error somehow. */
    close( this->file );
    this->file = -1;
    return rawsound();
  }

  if ( nullptr == current )
  {
    return rawsound();
  }

  return rawsound( current, blocksize, ploadtype, this->wavheader.sample_rate );
}

/*!md
# setposition and getposition
Gets and sets the position in terms of mS.
*/
void soundfile::setposition( long mseconds )
{
  int newposition = sizeof( wav_header );
  newposition += ( this->wavheader.bit_depth / 8 ) * ( this->wavheader.sample_rate / 1000 ) * mseconds; /* bytes per sample */
  this->cbwavblock.aio_offset = newposition;
}

long soundfile::getposition( void )
{
  if( this->cbwavblock.aio_offset <= ( __off_t ) sizeof( wav_header ) )
  {
    return 0;
  }

  long position = this->cbwavblock.aio_offset -= sizeof( wav_header );
  return position / ( this->wavheader.bit_depth / 8 ) * ( this->wavheader.sample_rate / 1000 );
}

/*!md
# complete
Have we completed reading the file.
*/
bool soundfile::complete( void )
{
  if( false == this->opened )
  {
    return false;
  }

  return this->cbwavblock.aio_offset > this->wavheader.wav_size;
}


/*!md
## wavinfo
For test purposes only. Read and display the wav file header info.
*/
void wavinfo( const char *file )
{
  wav_header hd;
  int fd = open( file, O_RDONLY, 0 );
  if( -1 == fd )
  {
    std::cerr << "Couldn't open file " << file << std::endl;
    return;
  }

  read( fd, &hd, sizeof( wav_header ) );

  if( 'R' != hd.riff_header[ 0 ] ||
      'I' != hd.riff_header[ 1 ] ||
      'F' != hd.riff_header[ 2 ] ||
      'F' != hd.riff_header[ 3 ] )
  {
    std::cout << "Bad riff" << std::endl;
    goto done;
  }

  if( 'W' != hd.wave_header[ 0 ] ||
      'A' != hd.wave_header[ 1 ] ||
      'V' != hd.wave_header[ 2 ] ||
      'E' != hd.wave_header[ 3 ] )
  {
    std::cout << "Bad wav" << std::endl;
    goto done;
  }

  if( 'f' != hd.fmt_header[ 0 ] ||
      'm' != hd.fmt_header[ 1 ] ||
      't' != hd.fmt_header[ 2 ] ||
      ' ' != hd.fmt_header[ 3 ] )
  {
    std::cout << "Bad format" << std::endl;
    goto done;
  }

  if( 'd' != hd.data_header[ 0 ] ||
      'a' != hd.data_header[ 1 ] ||
      't' != hd.data_header[ 2 ] ||
      'a' != hd.data_header[ 3 ] )
  {
    std::cout << "Bad data header" << std::endl;
    goto done;
  }

  std::cout << "Audio format: " << hd.audio_format << std::endl;
  std::cout << "Channel count: " << hd.num_channels << std::endl;
  std::cout << "Sample rate: " << hd.sample_rate << std::endl;
  std::cout << "Byte rate: " << hd.byte_rate << std::endl;
  std::cout << "Bit depth: " << hd.bit_depth << std::endl;
  std::cout << "Wav size: " << hd.wav_size << std::endl;

done:
  close( fd );
}
