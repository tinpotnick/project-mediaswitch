

#include "projectrtpsoundsoup.h"


/*!md
# c'stor
*/
soundsoup::soundsoup( void ) :
  loopcount( -1 ),
  currentfile( 0 )
{
}

/*!md
# d'stor
*/
soundsoup::~soundsoup()
{

}

/*!md
# create
Shared pointer version of us.
*/
soundsoup::pointer soundsoup::create( void )
{
  return pointer( new soundsoup() );
}


/*!md
# getpreferredfilename
Return the preferred filename based on the format we are sending data to.
*/
std::string *soundsoup::getpreferredfilename( JSON::Object &file, int format )
{
  switch( format )
  {
    case PCMUPAYLOADTYPE:
    {
      if( file.has_key( "pcmu" ) )
      {
        return &JSON::as_string( file[ "pcmu" ] );
      }
      if( file.has_key( "pcma" ) )
      {
        return &JSON::as_string( file[ "pcma" ] );
      }
      if( file.has_key( "l168k" ) )
      {
        return &JSON::as_string( file[ "l168k" ] );
      }
      if( file.has_key( "l1616k" ) )
      {
        return &JSON::as_string( file[ "l1616k" ] );
      }
      if( file.has_key( "ilbc" ) )
      {
        return &JSON::as_string( file[ "ilbc" ] );
      }
      if( file.has_key( "g722" ) )
      {
        return &JSON::as_string( file[ "g722" ] );
      }
      if( file.has_key( "wav" ) )
      {
        return &JSON::as_string( file[ "wav" ] );
      }
      break;
    }
    case PCMAPAYLOADTYPE:
    {
      if( file.has_key( "pcma" ) )
      {
        return &JSON::as_string( file[ "pcma" ] );
      }
      if( file.has_key( "pcmu" ) )
      {
        return &JSON::as_string( file[ "pcmu" ] );
      }
      if( file.has_key( "l168k" ) )
      {
        return &JSON::as_string( file[ "l168k" ] );
      }
      if( file.has_key( "l1616k" ) )
      {
        return &JSON::as_string( file[ "l1616k" ] );
      }
      if( file.has_key( "ilbc" ) )
      {
        return &JSON::as_string( file[ "ilbc" ] );
      }
      if( file.has_key( "g722" ) )
      {
        return &JSON::as_string( file[ "g722" ] );
      }
      if( file.has_key( "wav" ) )
      {
        return &JSON::as_string( file[ "wav" ] );
      }
      break;
    }
    case G722PAYLOADTYPE:
    {
      if( file.has_key( "g722" ) )
      {
        return &JSON::as_string( file[ "g722" ] );
      }
      if( file.has_key( "l1616k" ) )
      {
        return &JSON::as_string( file[ "l1616k" ] );
      }
      if( file.has_key( "l168k" ) )
      {
        return &JSON::as_string( file[ "l168k" ] );
      }
      if( file.has_key( "pcma" ) )
      {
        return &JSON::as_string( file[ "pcma" ] );
      }
      if( file.has_key( "pcmu" ) )
      {
        return &JSON::as_string( file[ "pcmu" ] );
      }
      if( file.has_key( "ilbc" ) )
      {
        return &JSON::as_string( file[ "ilbc" ] );
      }
      if( file.has_key( "wav" ) )
      {
        return &JSON::as_string( file[ "wav" ] );
      }
      break;
    }
    case ILBCPAYLOADTYPE:
    {
      if( file.has_key( "ilbc" ) )
      {
        return &JSON::as_string( file[ "ilbc" ] );
      }
      if( file.has_key( "l168k" ) )
      {
        return &JSON::as_string( file[ "l168k" ] );
      }
      if( file.has_key( "pcma" ) )
      {
        return &JSON::as_string( file[ "pcma" ] );
      }
      if( file.has_key( "pcmu" ) )
      {
        return &JSON::as_string( file[ "pcmu" ] );
      }
      if( file.has_key( "l1616k" ) )
      {
        return &JSON::as_string( file[ "l1616k" ] );
      }
      if( file.has_key( "g722" ) )
      {
        return &JSON::as_string( file[ "g722" ] );
      }
      if( file.has_key( "wav" ) )
      {
        return &JSON::as_string( file[ "wav" ] );
      }
      break;
    }
  }

  return nullptr;
}

/*!md
# config
Config (or reconfig) this soup.
*/
void soundsoup::config( JSON::Object &json, int format )
{
  if( json.has_key( "files" ) )
  {
    /* This is the only reason for our existence! */
    JSON::Array rfiles = JSON::as_array( json[ "files" ] );
    this->files.resize( rfiles.values.size() );
    size_t num = 0;

    if( this->currentfile > this->files.size() )
    {
      this->currentfile = 0;
    }

    for( auto it = rfiles.values.begin(); it != rfiles.values.end(); it++ )
    {
      JSON::Object &inref = JSON::as_object( *it );
      soundsoupfile &ref = this->files[ num ];

      std::string *newfilename = this->getpreferredfilename( inref, format );
      if( num == this->currentfile )
      {
        /* If we are currently playing this one, then continue if it is the same */
        if( !ref.sf || *newfilename != ref.sf->geturl() )
        {
          this->currentfile = 0;
          if( newfilename )
          {
            ref.sf = soundfile::create( *newfilename );
          }
          else
          {
            ref.sf = nullptr;
          }
        }
      }
      else
      {
        /* re-create all others */
        if( newfilename )
        {
          ref.sf = soundfile::create( *newfilename );
        }
        else
        {
          ref.sf = nullptr;
        }
      }

      /* Defaults */
      ref.loopcount = -1;
      ref.start = -1;
      ref.stop = -1;

      if( inref.has_key( "loop" ) )
      {
        ref.loopcount = JSON::as_int64( inref[ "loop" ] );
      }

      if( inref.has_key( "start" ) )
      {
        ref.start = JSON::as_int64( inref[ "start" ] );
      }

      if( inref.has_key( "stop" ) )
      {
        ref.stop = JSON::as_int64( inref[ "stop" ] );
      }

      num++;
    }
  }
}

rawsound soundsoup::read( void )
{
  if( 0 == this->files.size() )
  {
    return rawsound();
  }

  if( this->currentfile > this->files.size() )
  {
    this->currentfile = 0;
  }

  soundsoupfile &playing = this->files[ this->currentfile ];

  if( !playing.sf )
  {
    if( -1 == this->loopcount )
    {
      this->currentfile = 0;
    }

    return rawsound();
  }
  else if ( playing.sf->complete() )
  {
    return rawsound();
  }

  return playing.sf->read();
}

/*!md
# c'stor
*/
soundsoupfile::soundsoupfile() :
  start( -1 ),
  stop( -1 ),
  loopcount( -1 ),
  sf( nullptr )
{

}