
const http = require( "http" );
const url = require( "url" );

/*!md
# Project Control
A control server is one which provides the business logic as to how to handle a phone call(s). When a call comes into our SIP server, the SIP server passes this off to the control server and then goes of to do 'other things'. When we have instructions as to what to do with the call in question, we then make a HTTP request on the SIP server with relevent information.

This package provides utilities to handle requests from our SIP server and then provides functions to be able to respond to them.

```
         SIP Server
         |       |
         |       |
  RTP Server----Control Server (this)
```

When the SIP server requires a call to be handled it passes the call off to the control server. This processes any business logic. Then will pass back any information back to the SIP server (via a fresh HTTP request).


This file provides two classes. projectcontrol and call.

## projectcontrol

### onnewcall

Setter to set a callback to indicate a new call.

Example

```
projectcontrol.onnewcall = ( call ) =>
{
  call.ring();
}
```

### sipserver
Make a request to the SIP server (i.e. pass a HTTP request to the SIP server).

### invite
Request the SIP server to perform an INVITE.

### run
After setting up our control server listen for new connections.

```
projectcontrol.run();
```

### directory
The SIP server stores user information in memory. This information is pushed to it by the control server (or other server).

Information is passed to the SIP server via a HTTP request (like all events/traffic) in the following format.
```
HTTP PUT /dir/bling.babblevoice.com
{
  "control": "http://127.0.0.1:9001",
  "users":
  [
    { "username": "1003", "secret": "mysecret"}
  ]
}
```

To pass this information to the SIP server use this function:
```
projectcontrol.directory( "bling.babblevoice.com", [{ "username": "1003", "secret": "1123654789" } ] );
```

## class call
Represents a call which can be controlled.

### onringing
Setter to set a callback to be called when a ringing signal is received in regards to this call.

### onanswer

Setter to set a callback to be called when an answer signal (a call is answered) is received in regards to this call.

### onhangup
Setter to set a callback to be called when a hangup signal is received in regards to this call.

### ringing
Getter to see if the call is ringing.

Example
```
if( call.ringing )
{
  call.answer()
}
```

### answered

### hungup

## TODO

- [] Finish off onhangup

*/

class call
{
  constructor( control, callinfo )
  {
    this.control = control;
    this.callinfo = callinfo;

    this.onringingcallback = [];
    this.onanswercallback = [];
    this.onhangupcallback = [];
    this.onhangupcallback = [];
  }

  set info( callinf )
  {
    this.callinfo = callinf;
  }

  set onringing( cb )
  {
    this.onringingcallback.push( cb );
  }

  set onanswer( cb )
  {
    this.onanswercallback.push( cb );
  }

  set onhangup( cb )
  {
    this.onhangupcallback.push( cb );
  }

  get ringing()
  {
    if( "ring" in this.callinfo )
    {
      return true == this.callinfo.ring;
    }
    return false;
  }

  get answered()
  {
    if( "answered" in this.callinfo )
    {
      return true == this.callinfo.answered;
    }
    return false;
  }

  get hungup()
  {
    if( "hangup" in this.callinfo )
    {
      return true == this.callinfo.hangup;
    }
    return false;
  }

  answer( sdp, onanswer )
  {
    this.onanswercallback.push( onanswer );
    this.postrequest( "answer", { "sdp": sdp } );
  }

  ring( alertinfo )
  {
    if( this.ringing || this.answered )
    {
      return;
    }

    var postdata = {};

    if( undefined != alertinfo )
    {
      postdata.alertinfo = alertinfo;
    }

    this.postrequest( "ring", postdata );
  }

  notfound()
  {
    if( this.hungup )
    {
      return;
    }

    this.postrequest( "hangup", { "reason": "Not found", "code": 404 } );
  }

  paymentrequired()
  {
    if( this.hungup )
    {
      return;
    }

    this.postrequest( "hangup", { "reason": "Payment required", "code": 402 } );
  }

  busy()
  {
    if( this.hungup )
    {
      return;
    }

    this.postrequest( "hangup", { "reason": "Busy here", "code": 486 } );
  }

  hangup()
  {
    if( this.hungup )
    {
      return;
    }

    this.postrequest( "hangup", {} );
  }


  postrequest( action, data )
  {
    data.callid = this.callinfo.callid;
    this.control.sipserver( data, "/dialog/" + action );
  }
}


class projectcontrol
{
  constructor()
  {
    this.onregcallbacks = [];
    this.onderegcallbacks = [];
    this.onnewcallcallbacks = [];

    this.handlers = { 'PUT': {}, 'POST': {}, 'DELETE': {}, 'GET': {} };

    /* Store by call-id */
    this.calls = {}

    this.sip = {};
    this.sip.host = "127.0.0.1";
    this.sip.port = 9000;

    this.us = {};
    this.us.host = "127.0.0.1";
    this.us.port = 9001;

    this.handlers.PUT.dialog = ( pathparts, req, res, body ) =>
    {
      /*
        We add a call to our call dictionary when we get a call we do not know about (i.e new),
        we remove it when the call has been marked as hungup (we should not hear anything
        else from it).
      */
      try
      {
        var callinfo = JSON.parse( body );
        var c;
        if( !( callinfo.callid in this.calls ) )
        {
          c = new call( this, callinfo );
          this.calls[ callinfo.callid ] = c;

          for( var i = 0; i < this.onnewcallcallbacks.length; i ++ )
          {
            this.onnewcallcallbacks[ i ]( c );
          }
        }
        else
        {
          c = this.calls[ callinfo.callid ];
          c.info = callinfo;
        }

        if( c.ringing && c.onringingcallback.length > 0 )
        {
          for( var i = 0; i < c.onringingcallback.length; i ++ )
          {
            c.onringingcallback[ i ]();
          }
          c.onringingcallback = [];
        }

        if( c.answered && c.onanswercallback.length > 0 )
        {
          for( var i = 0; i < c.onanswercallback.length; i ++ )
          {
            c.onanswercallback[ i ]();
          }
          c.onanswercallback = [];
        }

        if( c.hungup )
        {
          for( var i = 0; i < c.onhangupcallback.length; i ++ )
          {
            c.onhangupcallback[ i ]();
          }
          c.onhangupcallback = [];
          delete this.calls[ callinfo.callid ];
        }
      }
      catch( e )
      {
        console.log( e );
        console.log( "Body: " + body );
      }
    }

    this.handlers.PUT.reg = ( pathparts, req, res, body ) =>
    {
      try
      {
        for( var i = 0; i < this.onregcallbacks.length; i++ )
        {
          this.onregcallbacks[ i ]( body );
        }
      }
      catch( e )
      {
        console.log( e );
      }
    }

    this.handlers.DELETE.reg = ( pathparts, req, res, body ) =>
    {
      try
      {
        for( var i = 0; i < this.onderegcallbacks.length; i++ )
        {
          this.onderegcallbacks[ i ]( body );
        }
      }
      catch( e )
      {
        console.log( e );
      }
    }
  }

  set onreg( callback )
  {
    this.onregcallbacks.push( callback );
  }

  set ondereg( callback )
  {
    this.onderegcallbacks.push( callback );
  }

  set onnewcall( callback )
  {
    this.onnewcallcallbacks.push( callback );
  }

  sipserver( request, path, method = "PUT" )
  {
    var data = JSON.stringify( request );
    var post_options = {
      "host": this.sip.host,
      "port": this.sip.port,
      "path": path,
      "method": method,
      "headers": {
        "Content-Type": "text/json",
        "Content-Length": Buffer.byteLength( data )
      }
    };

    var post_req = http.request( post_options );

    post_req.on( "error", (e) =>
    {
      console.error( `Problem with request: ${e.message}, for ${method} ${path}` );
    } );

    post_req.write( data );
    post_req.end();
  }

  invite( request, callback )
  {
    this.sipserver( request, "/dialog/invite" );
  }

  run()
  {
    this.httpserver = http.createServer( ( req, res ) =>
    {
      /*********************************************
       Gather our body.
      ********************************************/
      req.on( "data", ( chunk ) =>
      {
        if( !( "collatedbody" in this ) )
        {
          this.collatedbody = [];
        }
        this.collatedbody.push( chunk );
      } );

      req.on( "end", () =>
      {
        var urlparts = url.parse( req.url );
        /* Remove the leading '/' */
        var path = urlparts.path.substr( 1 );
        var pathparts = path.split( '/' );

        if( req.method in this.handlers && pathparts[ 0 ] in this.handlers[ req.method ] )
        {
          res.writeHead( 200, { "Content-Length": "0" } );
          this.handlers[ req.method ][ pathparts[ 0 ] ]( pathparts, req, res, Buffer.concat( this.collatedbody ).toString() );
          this.collatedbody = [];
        }
        else
        {
          console.log( "Unknown method " + req.method + ":" + url );
          res.writeHead( 404, { "Content-Length": "0" } );
        }
        res.end();
      } );

    } );

    this.httpserver.listen( this.us.port, this.us.host, () =>
    {
      console.log( `Project Control Server is running on ${this.us.host} port ${this.us.port}` );
    } );
  }

  directory( domain, users )
  {
    var request = {};
    request.control = "http://" + this.us.host + ":" + this.us.port;
    request.users = users;

    this.sipserver( request, "/dir/" + domain );
  }

  /*
    Create a new SDP object.
  */
  sdp( sessionid, ip, port )
  {
    var sdp = {
      v: 0,
      t: { start: 0, stop: 0 },
      o: {
        username: "-",
        sessionid: sessionid,
        sessionversion: 0,
        nettype: "IN",
        ipver: 4,
        address: ip
      },
      s: " ",
      c: [
        {
          nettype: "IN",
          ipver: 4,
          address: ip
        } ],
      m: [
          {
            media: "audio",
            port: port,
            proto: "RTP/AVP",
            ptime: 20,
            direction: "sendrecv",
            payloads: [],
            rtpmap: {},
          }
        ]
    };

    return sdp;
  }

  addmedia( sdp, codec )
  {
    switch( codec )
    {
      case "pcmu":
      {
        sdp.m[ 0 ].payloads.push( 0 );
        sdp.m[ 0 ].rtpmap[ "0" ] = { encoding: "PCMU", clock: "8000" };
        break;
      }
    }
  }
}


module.exports = new projectcontrol();
