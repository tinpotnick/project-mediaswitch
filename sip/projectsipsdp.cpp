

#include <boost/lexical_cast.hpp>
#include <boost/crc.hpp>

#include <iostream>


#include "projectsippacket.h"
#include "projectsipsdp.h"


/*******************************************************************************
Function: sdptojson
Purpose: Search for the next end of line and return a count to that point from 
in.
Updated: 31.01.2019
*******************************************************************************/
inline size_t findendofline( const char * in, size_t maxlength )
{
  const char* it = in;
  size_t count = 0;
  while( '\n' != *it )
  {
    count++;
    it++;
    if( count > maxlength )
    {
      return maxlength;
    }
  }

  if( '\r' == *( it - 1 ) )
  {
    return count - 1;
  }
  return count;
}

/*******************************************************************************
Function: toseconds
Purpose: Convert:
d - days (86400 seconds)
h - hours (3600 seconds)
m - minutes (60 seconds)
s - seconds (allowed for completeness)
To seconds.
str = 7d, we sould return 604800
Updated: 31.01.2019
*******************************************************************************/
inline JSON::Integer toseconds( std::string &str )
{
  try
  {
    int muliplier = 1;
    switch( str[ str.length() - 1 ] )
    {
      case 'd':
        str.pop_back();
        muliplier = 3600 * 24;
        break;
      case 'h':
        str.pop_back();
        muliplier = 3600;
        break;
      case 'm':
        str.pop_back();
        muliplier = 60;
        break;
      case 's':
        str.pop_back();
      default:
        break;
    }
    return ( JSON::Integer ) boost::lexical_cast< int >( str.c_str() ) * muliplier;
  }
  catch( boost::bad_lexical_cast &e )
  {
  }

  return 0;
}

/*******************************************************************************
Function: sdptojson
Purpose: Convert the SDP to a JSON document so we can pass up the tree in 
a more usable format.

      Session description
         v=  (protocol version)
         o=  (originator and session identifier)
         s=  (session name)
         i=* (session information)
         u=* (URI of description)
         e=* (email address)
         p=* (phone number)
         c=* (connection information -- not required if included in
              all media)
         b=* (zero or more bandwidth information lines)
         One or more time descriptions ("t=" and "r=" lines; see below)
         z=* (time zone adjustments)
         k=* (encryption key)
         a=* (zero or more session attribute lines)
         Zero or more media descriptions

      Time description
         t=  (time the session is active)
         r=* (zero or more repeat times)

      Media description, if present
         m=  (media name and transport address)
         i=* (media title)
         c=* (connection information -- optional if included at
              session level)
         b=* (zero or more bandwidth information lines)
         k=* (encryption key)
         a=* (zero or more media attribute lines)

Updated: 31.01.2019
*******************************************************************************/
bool sdptojson( substring in, JSON::Value &out )
{
#warning
  /* Currently no support for timezone. */
  JSON::Object sdp;

  JSON::Object a;
  JSON::Object *atributes = &a;

  const char *it = in.c_str();

  for( size_t i = in.start(); i < in.end(); )
  {
    char type = *it;
    char typestr[ 2 ];
    typestr[ 1 ] = 0;
    typestr[ 0 ] = type;

    size_t startofline = i;

    if( '=' != *( it + 1 ) )
    {
      return false;
    }

    size_t endofline = findendofline( it, in.end() - i );
    i += endofline + 2;
    it += endofline + 2;

    substring value = substring( in, startofline + 2, i - 2 );
    switch( type )
    {
      case 'v':
      {
        sdp[ typestr ] = ( JSON::Integer ) value.toint();
        break;
      }
      case 'o':
      {
        stringvector parts = splitstring( value.str(), ' ' );
        if( 6 == parts.size() )
        {
          try
          {
            JSON::Object origin;
            origin[ "username" ] = parts[ 0 ];
            origin[ "sessionid" ] = (JSON::Integer ) boost::lexical_cast< int >( parts[ 1 ].c_str() );
            origin[ "sessionversion" ] = (JSON::Integer ) boost::lexical_cast< int >( parts[ 2 ].c_str() );
            origin[ "nettype" ] = parts[ 3 ];
            if( parts[ 4 ] == "IP4" )
            {
              origin[ "ipver" ] = (JSON::Integer ) 4;
            }
            else if( parts[ 4 ] == "IP6" )
            {
              origin[ "ipver" ] = (JSON::Integer ) 6;
            }
            origin[ "address" ] = parts[ 5 ];
            sdp[ typestr ] = origin;
          }
          catch( boost::bad_lexical_cast &e )
          {
          }
        }
        break;
      }
      case 's':
      case 'i':
      case 'e':
      case 'p':
      case 'u':
      {
        sdp[ typestr ] = value.str();
        break;
      }
      case 'c':
      {
        if( !sdp.has_key( typestr ) )
        {
          JSON::Array a;
          sdp[ typestr ] = a;
        }

        stringvector parts = splitstring( value.str(), ' ' );
        if( 3 == parts.size() )
        {
          try
          {
            JSON::Object connection;
            connection[ "nettype" ] = parts[ 0 ];
            if( parts[ 1 ] == "IP4" )
            {
              connection[ "ipver" ] = (JSON::Integer ) 4;
            }
            else if( parts[ 1 ] == "IP6" )
            {
              connection[ "ipver" ] = (JSON::Integer ) 6;
            }
            connection[ "address" ] = parts[ 2 ];
            JSON::as_array( sdp[ typestr ] ).values.push_back( connection );
          }
          catch( boost::bad_lexical_cast &e )
          {
          }
        }
        break;
      }
      case 'b':
      {
        try
        {
          stringvector parts = splitstring( value.str(), ':' );
          JSON::Object bandwidth;
          bandwidth[ "bwtype" ] = parts[ 0 ];
          bandwidth[ "bandwidth" ] = ( JSON::Integer ) boost::lexical_cast< int >( parts[ 1 ].c_str() );
          sdp[ typestr ] = bandwidth;
        }
        catch( boost::bad_lexical_cast &e )
        {
        }
        break;
      }
      case 'z':
      {
        break;
      }
      case 'k':
      {
          stringvector parts = splitstring( value.str(), ':' );
          JSON::Object enc;
          enc[ "method" ] = parts[ 0 ];
          if( parts.size() > 1 )
          {
            enc[ "key" ] = parts[ 1 ];
          }
          sdp[ typestr ] = enc;
        break;
      }
      case 't':
      {
        stringvector parts = splitstring( value.str(), ' ' );
        if( 2 == parts.size() )
        {
          try
          {
            JSON::Object timing;
            timing[ "start" ] = ( JSON::Integer ) boost::lexical_cast< int >( parts[ 0 ].c_str() );
            timing[ "stop" ] = ( JSON::Integer ) boost::lexical_cast< int >( parts[ 1 ].c_str() );
            sdp[ typestr ] = timing;
          }
          catch( boost::bad_lexical_cast &e )
          {
          }
        }
        break;
      }
      case 'r':
      {
        stringvector parts = splitstring( value.str(), ' ' );
        if( parts.size() > 2 )
        {
          JSON::Object repeat;
          repeat[ "interval" ] = toseconds( parts[ 0 ] );
          repeat[ "duration" ] = toseconds( parts[ 1 ] );
          JSON::Array a;
          repeat[ "offsets" ] = a;

          for( size_t i = 2; i < parts.size(); i ++ )
          {
            JSON::as_array( repeat[ "offsets" ] ).values.push_back( toseconds( parts[ i ] ) );
          }
          sdp[ typestr ] = repeat;
        }
        break;
      }
      case 'm':
      {
        stringvector parts = splitstring( value.str(), ' ' );
        if( parts.size() >= 3 )
        {
          try
          {
            JSON::Object media;
            media[ "media" ] = parts[ 0 ];

            stringvector portparts = splitstring( parts[ 1 ], '/' );
            media[ "port" ] = ( JSON::Integer ) boost::lexical_cast< int >( portparts[ 0 ].c_str() );
            if( portparts.size() > 1 )
            {
              media[ "numports"] = ( JSON::Integer ) boost::lexical_cast< int >( portparts[ 1 ].c_str() );
            }
            media[ "proto" ] = parts[ 2 ];

            JSON::Array a;
            media[ "payloads" ] = a;

            for( size_t i = 3; i < parts.size(); i ++ )
            {
              JSON::as_array( media[ "payloads" ] ).values.push_back( toseconds( parts[ i ] ) );
            }

            if( !sdp.has_key( typestr ) )
            {
              JSON::Array a;
              sdp[ typestr ] = a;
            }

            JSON::as_array( sdp[ typestr ] ).values.push_back( media );
            atributes = &JSON::as_object( JSON::as_array( sdp[ typestr ] ).values[ JSON::as_array( sdp[ typestr ] ).values.size() - 1 ] );
          }
          catch( boost::bad_lexical_cast &e )
          {
          }
        }
        break;
      }
      case 'a':
      {
        JSON::Object &at = *atributes;
        std::string strtosplit = value.str();
        stringvector parts = splitstring( strtosplit, ':' );

        boost::crc_32_type crccheck;
        for( std::string::iterator crcit = parts[ 0 ].begin(); 
              crcit != parts[ 0 ].end();
              crcit++ )
        {
          crccheck.process_byte( std::tolower( *crcit ) );
        }

        switch( crccheck.checksum() )
        {
          case 0xb8a3184e:   /* sendrecv */
          {
            at[ "direction" ] = "sendrecv";
            break;
          }
          case 0x6bb6ae11:   /* recvonly */
          {
            at[ "direction" ] = "recvonly";
            break;
          }
          case 0x1a279b3:   /* sendonly */
          {
            at[ "direction" ] = "sendonly";
            break;
          }
          case 0x8bd30ee0:   /* inactive */
          {
            at[ "direction" ] = "inactive";
            break;
          }
          case 0x7cb20b10:   /* quality */
          case 0x7120508b:   /* ptime */
          case 0x537e0568:   /* maxptime */
          {
            try
            {
              if( parts.size() == 2 )
              {
                at[ parts[ 0 ] ] = ( JSON::Integer ) boost::lexical_cast< int >( parts[ 1 ].c_str() );
              }
            }
            catch( boost::bad_lexical_cast &e )
            {
            }
            break;
          }
          case 0xd37b9e1:   /* keywds */
          case 0x9e5e43a8:   /* cat */
          case 0x20f33ed1:   /* tool */
          case 0x6f56edcd:   /* orient */
          case 0x8cde5729:   /* type (conference type) */
          case 0xbfa4ce15:   /* charset */
          case 0x3a16e335:   /* sdplang */
          case 0x31098462:   /* lang */
          default:
          {
            if( parts.size() == 2 )
            {
              at[ parts[ 0 ] ] = parts[ 1 ];
            }
            break;
          }
          case 0x11d439a0:   /* framerate */
          {
            try
            {
              if( parts.size() == 2 )
              {
                at[ "framerate" ] = ( JSON::Double ) boost::lexical_cast< double >( parts[ 1 ].c_str() );
              }
            }
            catch( boost::bad_lexical_cast &e )
            {
            }
            break;
          }
          case 0x6450e27e:   /* fmtp */
          {
            stringvector fm = splitstring( parts[ 1 ], ' ' );
            std::string format = fm[ 0 ];
            fm.erase( fm.begin() );
            std::string fmparams = joinstring( fm, ' ' );

            if( !at.has_key( "fmtp" ) )
            {
              JSON::Object a;
              at[ "fmtp" ] = a;
            }
            JSON::as_object( at[ "fmtp" ] )[ format ] = fmparams;
            break;
          }
          case 0xfca99853:   /* rtpmap */
          {
            stringvector payload = splitstring( parts[ 1 ], ' ' );
            std::string payloadtype = payload[ 0 ];

            if( !at.has_key( "rtpmap" ) )
            {
              JSON::Object a;
              at[ "rtpmap" ] = a;
            }

            JSON::Object us;

            if( payload.size() > 1 )
            {
              stringvector enc = splitstring( payload[ 1 ], '/' );
              us[ "encoding" ] = enc[ 0 ];

              if( enc.size() > 1 )
              {
                try
                {
                  us[ "clock" ] = ( JSON::Integer ) boost::lexical_cast< int >( enc[ 1 ].c_str() );
                }
                catch( boost::bad_lexical_cast &e )
                {
                }
              }
            }
            if( payload.size() > 2 )
            {
              us[ "params" ] = payload[ 2 ];
            }

            JSON::as_object( at[ "rtpmap" ] )[ payloadtype ] = us;
            break;
          }
        }
        break;
      }
    }
  }

  if( a.values.size() > 0 )
  {
    sdp[ "a" ] = a;
  }
  
  out = sdp;
  return true;
}


static inline void appendattributes( std::string &out, JSON::Object &a )
{
  JSON::Object::values_t::iterator it;
  for( it = a.values.begin();
        it != a.values.end();
        it++ )
  {
    boost::crc_32_type crccheck;
    std::string attr = it->first;

    for( std::string::iterator crcit = attr.begin(); 
          crcit != attr.end();
          crcit++ )
    {
      crccheck.process_byte( std::tolower( *crcit ) );
    }
    
    switch( crccheck.checksum() )
    {
      case 0x3e4ad1b3:   /* direction - instead of all of the send/recv/inactive */
      {
        out += "a=";
        out += JSON::as_string( it->second );
        out += "\r\n";
        break;
      }
      case 0xfca99853:   /* rtpmap */
      {
        JSON::Object::values_t::iterator mapit;
        for( mapit = JSON::as_object( it->second ).values.begin();
              mapit != JSON::as_object( it->second ).values.end();
              mapit++ )
        {
          out += "a=";
          // rtpmap:<payload type> <encoding name>/<clock rate> [/<encoding parameters>]
          out += "rtpmap:";
          out += mapit->first;
          out += " ";

          JSON::Object &ref = JSON::as_object( mapit->second );
          out += JSON::as_string( ref[ "encoding" ] );
          if( ref.has_key( "clock" ) )
          {
            out += "/";
            out += JSON::to_string( ref[ "clock" ] );
          }
          if( ref.has_key( "params" ) )
          {
            out += "/";
            out += JSON::as_string( ref[ "params" ] );
          }
          out += "\r\n";
        }
        break;
      }
      case 0x6450e27e:   /* fmtp */
      {
        out += "a=";
        // fmtp:<format> <format specific parameters>
        out += "fmtp:";
        JSON::Object obj = JSON::as_object( it->second );
        out += JSON::as_string( obj.values[ 0 ].first );
        out += " ";
        out += JSON::as_string( obj.values[ 0 ].second );
        out += "\r\n";
        break;
      }
      case 0x7cb20b10:   /* quality */
      case 0x11d439a0:   /* framerate */
      case 0x31098462:   /* lang */
      case 0x3a16e335:   /* sdplang */
      case 0xbfa4ce15:   /* charset */
      case 0x8cde5729:   /* type */
      case 0x6f56edcd:   /* orient */
      case 0x20f33ed1:   /* tool */
      case 0xd37b9e1:   /* keywds */
      case 0x9e5e43a8:   /* cat */
      case 0x7120508b:   /* ptime */
      case 0x537e0568:   /* maxptime */
      {
        out += "a=";
        out += JSON::as_string( it->first );
        out += ":";
        out += JSON::to_string( it->second );
        out += "\r\n";
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

/*******************************************************************************
Function: jsontosdp
Purpose: Convert the JSON to a string to be sent to a client. We only support
version 0. 

{"v":0,
  "o":{"username":"-","sessionid":20518,"sessionversion":0,"nettype":"IN", "ipver": 4, "address":"203.0.113.1"},
  "s":" ",
  "t":{"start":0,"stop":2},
  "c":[{"nettype":"IN","ipver":4,"address":"203.0.113.1"}],
  "m":[
    {"media":"audio","port":54400,"proto":"RTP/SAVPF","payloads":[0,96],
      "rtpmap":
        {
          "0":{"encoding":"PCMU","clock":"8000"},
          "96":{"encoding":"opus","clock":"48000"}
        },
        "ptime":20,"direction":"sendrecv","candidate":"1 2 UDP 2113667326 203.0.113.1 54401 typ host"},
    {"media":"video","port":55400,"proto":"RTP/SAVPF","payloads":[97,98],
    "rtpmap":
    {
      "97":{"encoding":"H264","clock":"90000"},
      "98":{"encoding":"VP8","clock":"90000"}
    },
    "fmtp":
    {
      "97":"profile-level-id=4d0028;packetization-mode=1"
    },
    "direction":"sendrecv",
    "candidate":"1 2 UDP 2113667326 203.0.113.1 55401 typ host"}],
  "r":{"interval":604800,"duration":3600,"offsets":[0,90000]},
  "a":{"ice-ufrag":"F7gI","ice-pwd":"x9cml/YzichV2+XlhiMu8g"}
}

Updated: 01.02.2019
*******************************************************************************/
stringptr jsontosdp( JSON::Object &in )
{
  stringptr sdp = stringptr( new std::string() );
  sdp->reserve( MASSIPPACKETLENGTH );

  std::string &ref = *sdp;

  ref = "v=0\r\n";

  try
  {
    if( in.has_key( "o" ) )
    {
      ref += "o=";
      ref += JSON::as_string( JSON::as_object( in[ "o" ] )[ "username" ] );
      ref += " ";
      ref += std::to_string( JSON::as_int64( JSON::as_object( in[ "o" ] )[ "sessionid" ] ) );
      ref += " ";
      ref += std::to_string( JSON::as_int64( JSON::as_object( in[ "o" ] )[ "sessionversion" ] ) );
      ref += " ";
      ref += JSON::as_string( JSON::as_object( in[ "o" ] )[ "nettype" ] );

      if( 6 == JSON::as_int64( JSON::as_object( in[ "o" ] )[ "ipver" ] ))
      {
        ref += " IP6 ";
      }
      else
      {
        ref += " IP4 ";
      }
      ref += JSON::as_string( JSON::as_object( in[ "o" ] )[ "address" ] );
      ref += "\r\n";
    }

    if( in.has_key( "s" ) )
    {
      ref += "s=";
      ref += JSON::as_string( in[ "s" ] );
      ref += "\r\n";
    }

    if( in.has_key( "i" ) )
    {
      ref += "i=";
      ref += JSON::as_string( in[ "i" ] );
      ref += "\r\n";
    }

    if( in.has_key( "u" ) )
    {
      ref += "u=";
      ref += JSON::as_string( in[ "u" ] );
      ref += "\r\n";
    }

    if( in.has_key( "e" ) )
    {
      ref += "e=";
      ref += JSON::as_string( in[ "e" ] );
      ref += "\r\n";
    }

    if( in.has_key( "p" ) )
    {
      ref += "p=";
      ref += JSON::as_string( in[ "p" ] );
      ref += "\r\n";
    }

    if( in.has_key( "c" ) )
    {
      JSON::Array::values_t::iterator it;
      for( it = JSON::as_array( in[ "c" ] ).values.begin();
            it != JSON::as_array( in[ "c" ] ).values.end();
            it++ )
      {
        ref += "c=";
        ref+= JSON::as_string( JSON::as_object( *it )[ "nettype" ] );
        if( 6 == JSON::as_int64( JSON::as_object( *it )[ "ipver" ] ) )
        {
          ref += " IP6 ";
        }
        else
        {
          ref += " IP4 ";
        }
        ref+= JSON::as_string( JSON::as_object( *it )[ "address" ] );
        ref += "\r\n";
      }
    }

    if( in.has_key( "b" ) )
    {
      ref += "b=";
      ref += JSON::as_string( JSON::as_object( in[ "b" ] )[ "bwtype" ] );
      ref += ":";
      ref += JSON::as_string( JSON::as_object( in[ "b" ] )[ "bandwidth" ] );
      ref += "\r\n";
    }

    if( in.has_key( "t" ) )
    {
      ref += "t=";
      ref += std::to_string( JSON::as_int64( JSON::as_object( in[ "t" ] )[ "start" ] ) );
      ref += " ";
      ref += std::to_string( JSON::as_int64( JSON::as_object( in[ "t" ] )[ "stop" ] ) );
      ref += "\r\n";
    }

    if( in.has_key( "r" ) )
    {
      // interval duration [ offsets ]
      ref += "r=";
      ref += std::to_string( JSON::as_int64( JSON::as_object( in[ "r" ] )[ "interval" ] ) );
      ref += " ";
      ref += std::to_string( JSON::as_int64( JSON::as_object( in[ "r" ] )[ "duration" ] ) );

      if( JSON::as_object( in[ "r" ] ).has_key( "offsets" ) )
      {
        JSON::Array::values_t::iterator it;
        for( it = JSON::as_array( JSON::as_object( in[ "r" ] )[ "offsets" ] ).values.begin();
              it != JSON::as_array( JSON::as_object( in[ "r" ] )[ "offsets" ] ).values.end();
              it++ )
        {
          ref += " ";
          ref += std::to_string( JSON::as_int64( *it ) );
        }
      }

      ref += "\r\n";
    }

    if( in.has_key( "z" ) )
    {
    }

    if( in.has_key( "k" ) )
    {
      ref += "k=";
      ref += JSON::as_string( JSON::as_object( in[ "k" ] )[ "method" ] );

      if( JSON::as_object( in[ "a" ] ).has_key( "key" ) )
      {
        ref += " ";
        ref += JSON::as_string( JSON::as_object( in[ "k" ] )[ "key" ] );
      }
      ref += "\r\n";
    }

    if( in.has_key( "a" ) )
    {
      appendattributes( ref, JSON::as_object( in[ "a" ] ) );
    }


    if( in.has_key( "m" ) )
    {
      JSON::Array::values_t::iterator it;
      for( it = JSON::as_array( in[ "m" ] ).values.begin();
            it != JSON::as_array( in[ "m" ] ).values.end();
            it++ )
      {
        JSON::Object &mref = JSON::as_object( *it );
        ref += "m=";
        ref += JSON::as_string( mref[ "media" ] );
        ref += " ";
        ref += std::to_string( JSON::as_int64( mref[ "port" ] ) );

        if( mref.has_key( "numports" ) )
        {
          ref += "/";
          ref += std::to_string( JSON::as_int64( mref[ "numports" ] ) );
        }
        ref += " ";
        ref += JSON::as_string( mref[ "proto" ] );

        JSON::Array::values_t::iterator pit;
        for( pit = JSON::as_array( mref[ "payloads" ] ).values.begin();
              pit != JSON::as_array( mref[ "payloads" ] ).values.end();
              pit++ )
        {
          ref += " ";
          ref += std::to_string( JSON::as_int64( *pit ) );
        }

        ref += "\r\n";
        appendattributes( ref, mref );
      }
    }
  }
  catch( const boost::bad_get &e )
  {

  }

  return sdp;
}


