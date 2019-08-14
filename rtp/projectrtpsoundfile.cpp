
#include <iostream>

#include "projectrtpsoundfile.h"
#include "globals.h"


/*!md
## soundfile
Ideas:
This is to be used in an xform in our channel which has a chain of actions. We may have already had MOH placed into our packet in our read function which we simply want to attenuate in this function here. So, how are we going to a) pass that other information in and b) other options like only this xform to save CPU on playing MOH if it is fully attenuated?

This begs the question, do we create a format of sound file, which has the same recording but in different formats in it. Similar to wav for different channels but would need to allow different lengths of recordings in it in different formats.

The mix of compression and bandwidth can be used either in the file.wav or file.wav.g722 (still wav but reference in the filename). The bandwidth is defined by the compression (i.e. pcma, pcmu and ilbc are all narrowband, g722 is wideband, pcm can be both) so

pcm, pcmu, pcma, g722, ilbc
Will be assigned the following file formats

| Format | extension |
|-|-|
| pcm - narrowband | .pcm8 |
| pcm - wideband | .pcm16 |
| pcmu | .pcmu |
| pcma | .pcma |
| g722 | .g722 |
| ilbc | .ilbc |

We prefer the specific filename and will expect it to be in the correct format.
*/
soundfile::soundfile( std::string &filename, compression format, int mode, bandwidth bps ) :
  file( 0 ),
  readbuffer( nullptr ),
  mode( mode ),
  blocksize( G711PAYLOADBYTES )
{
  std::string filenfullpath = mediachroot + filename;
  std::string filenameforcompression;

  this->mode = mode;

  /* allocate a buffer */
  switch( mode )
  {
    case pcm:
    {
      if( wideband == bps )
      {
        filenameforcompression = filenfullpath + ".pcm16";
        this->blocksize = L16WIDEBANDBYTES;
      }
      else
      {
        filenameforcompression = filenfullpath + ".pcm18";
        this->blocksize = L16NARROWBANDBYTES;
      }
      break;
    }
    case pcmu:
    {
      filenameforcompression = filenfullpath + ".pcmu";
      this->blocksize = G711PAYLOADBYTES;
      break;
    }
    case pcma:
    {
      filenameforcompression = filenfullpath + ".pcma";
      this->blocksize = G711PAYLOADBYTES;
      break;
    }
    case g722:
    {
      filenameforcompression = filenfullpath + ".g722";
      this->blocksize = G722PAYLOADBYTES;
      break;
    }
    case ilbc:
    {
      filenameforcompression = filenfullpath + ".ilbc";
      this->blocksize = ILBC20PAYLOADBYTES;
      break;
    }
  }

  this->file = open( filenameforcompression.c_str(), mode | O_NONBLOCK, 0 );

	if ( -1 == this->file )
	{
    this->file = open( filenfullpath.c_str(), mode | O_NONBLOCK, 0 );
    if ( -1 == this->file )
    {
      return;
    }
	}

  this->readbuffer = new uint8_t[ this->blocksize ];

  /* As it is asynchronous - we read wav header + ahead */
	memset( &wavheader, 0, sizeof( aiocb ) );
	wavheader.aio_nbytes = this->blocksize;
	wavheader.aio_fildes = file;
	wavheader.aio_offset = 0;
	wavheader.aio_buf = this->readbuffer;

  memset( &wavblock, 0, sizeof( aiocb ) );
	wavblock.aio_nbytes = this->blocksize;
	wavblock.aio_fildes = file;
	wavblock.aio_offset = 0;
	wavblock.aio_buf = this->readbuffer;

	/* read */
	if ( aio_read( &wavheader ) == -1 )
	{
    /* report error somehow. */
		close( this->file );
    this->file = 0;
    return;
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
}

/*!md
# create
Shared pointer version of us.
*/
soundfile::pointer soundfile::create( std::string &filename, compression format, int mode, bandwidth bps )
{
  return pointer( new soundfile( filename, format, mode, bps ) );
}

/*
## read
Asynchronous read.

Return the number of bytes read. Will read the appropriate amount of bytes for 1 rtp packet for the defined CODEC.
If not ready return -1.
*/
int soundfile::read( rtppacket *pk )
{
  if( aio_error( &this->wavblock ) == EINPROGRESS )
  {
    return -1;
  }

  /* success? */
	int numbytes = aio_return( &this->wavblock );

  if( -1 == numbytes )
  {
    return -1;
  }

  memcpy( pk->pk, (const void *)this->wavblock.aio_buf, this->wavblock.aio_nbytes );

  /* read again */
  if ( aio_read( &this->wavblock ) == -1 )
  {
    /* report error somehow. */
    close( this->file );
    this->file = 0;
    return -1;
  }

  return numbytes;
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
  std::cout << "BPS: " << hd.bit_depth << std::endl;
  std::cout << "Wav size: " << hd.wav_size << std::endl;

done:
  close( fd );
}
