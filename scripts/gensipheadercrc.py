#!/usr/bin/python

# Script to generate CRC values for a C file. Allows for a faster
# parsing of a SIP packet.

import zlib
import string

sipheaders = [ # SIP Headers
          "Authorization",
          "Allow",
          "Alert-Info",
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
          "Remote-Party-ID",
          "Route",
          "Retry-After",
          "Reason",
          "Supported",
          "To",
          "Via",
          "User-Agent",
          "Min-Expires",
          "WWW-Authenticate",
          # SIP Verbs
          "REGISTER",
          "INVITE",
          "ACK",
          "OPTIONS",
          "CANCEL",
          "BYE"
        ]



crcvalues = []
for header in sipheaders:

  lowerheader = header.lower()
  c = hex(zlib.crc32(lowerheader) & 0xffffffff )

  print "    case " + c + ":   /* " + lowerheader + " */"
  print "    {"
  print "      return " + string.replace( header, '-', '_') + ";"
  print "    }"

  if c in crcvalues:
    print "Error duplicate found - need a different CRC"

  crcvalues.append( c )


httpheaders = [
            # HTTP Verbs
          "OPTIONS",
          "GET",
          "HEAD",
          "POST",
          "PUT",
          "DELETE",
          "TRACE",
          "CONNECT",
          # HTTP headers
          # Request
          "Accept",
          "Accept-Charset",
          "Accept-Encoding",
          "Accept-Language",
          "Authorization",
          "Expect",
          "From",
          "Host",
          "If-Match",
          "If-Modified-Since",
          "If-None-Match",
          "If-Range",
          "If-Unmodified-Since",
          "Max-Forwards",
          "Proxy-Authorization",
          "Range",
          "Referer",
          "TE",
          "User-Agent",
          # Response
          "Accept-Ranges",
          "Age",
          "ETag",
          "Location",
          "Proxy-Authenticate",
          "Retry-After",
          "Server",
          "Vary",
          "WWW-Authenticate",
          # Entity
          "Allow",
          "Content-Encoding",
          "Content-Language",
          "Content-Length",
          "Content-Location",
          "Content-MD5",
          "Content-Range",
          "Content-Type",
          "Expires",
          "Last-Modified"
        ]

print "HTTP"
print "================================================================"
crcvalues = []
for header in httpheaders:

  lowerheader = header.lower()
  c = hex(zlib.crc32(lowerheader) & 0xffffffff )

  print "    case " + c + ":   /* " + lowerheader + " */"
  print "    {"
  print "      return " + string.replace( header, '-', '_') + ";"
  print "    }"

  if c in crcvalues:
    print "Error duplicate found - need a different CRC"

  crcvalues.append( c )


print "SDP attributes"
print "================================================================"

sdpa = [
    "sendrecv",
    "recvonly",
    "sendonly",
    "inactive",
    "ptime",
    "maxptime",
    "rtpmap",
    "cat",
    "keywds",
    "tool",
    "orient",
    "type",
    "charset",
    "sdplang",
    "lang",
    "framerate",
    "quality",
    "fmtp",
    # Non standard json index
    "direction"
]

crcvalues = []
for header in sdpa:

  lowerheader = header.lower()
  c = hex(zlib.crc32(lowerheader) & 0xffffffff )

  print "    case " + c + ":   /* " + lowerheader + " */"
  print "    {"
  print "      break;"
  print "    }"

  if c in crcvalues:
    print "Error duplicate found - need a different CRC"

  crcvalues.append( c )