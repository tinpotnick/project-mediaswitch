#!/usr/bin/python

# Script to generate CRC values for a C file. Allows for a faster
# parsing of a SIP packet.

import zlib
import string

headers = [ "Authorization",
          "Call-ID",
          "Content-Length",
          "CSeq",
          "Contact",
          "Content-Type",
          "Expires",
          "From",
          "Max-Forwards",
          "Proxy-Authenticate",
          "Proxy-Authorization",
          "Record-Route",
          "Route",
          "Retry-After",
          "Supported",
          "To",
          "Via",
          "User-Agent",
          "WWW-Authenticate" ]


crcvalues = []
for header in headers:

  lowerheader = header.lower()
  c = hex(zlib.crc32(lowerheader) & 0xffffffff )

  print "    case " + c + ":   /* " + lowerheader + " */"
  print "    {"
  print "      return " + string.replace( header, '-', '_') + ";"
  print "    }"

  if c in crcvalues:
    print "Error duplicate found - need a different CRC"

  crcvalues.append( c )


