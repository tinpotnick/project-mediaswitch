
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
projectcontrol.listen();
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
  }

  set info( callinf )
  {
    this.callinfo = callinf;
  }

  set onringing( cb )
  {
    this.onringingcallback = cb
  }

  set onanswer( cb )
  {
    this.onanswercallback = cb
  }

  set onhangup( cb )
  {
    this.onhangupcallback = cb
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

  answer( onanswer )
  {
    var postdata = {
      action: "answer"
    };

    this._onanswer = onanswer;
    this.postrequest( postdata );
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

  hangup( reason )
  {
    if( this.hungup )
    {
      return;
    }

    var postdata =
    {
      reason: reason
    };

    this.postrequest( "hangup", postdata );
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
    this.handlers = {}

    /* Store by call-id */
    this.calls = {}

    this.sip = {};
    this.sip.host = "127.0.0.1";
    this.sip.port = 9000;

    this.us = {};
    this.us.host = "127.0.0.1";
    this.us.port = 9001;

    this.handlers.invite = ( pathparts, req, res, body ) =>
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

          if( "onnewcallcallback" in this )
          {
            this.onnewcallcallback( c );
          }
        }
        else
        {
          c = this.calls[ callinfo.callid ];
          c.info = callinfo;
        }

        if( c.ringing && "onringingcallback" in c )
        {
          c.onringingcallback();
          delete c.onringingcallback;
        }

        if( c.answered && "onanswercallback" in c )
        {
          c.onanswercallback();
          delete c.onanswercallback;
        }

        if( c.hungup )
        {
          if( "onhangupcallback" in c )
          {
            c.onhangupcallback();
            delete c.onhangupcallback;
          }
          delete this.calls[ callinfo.callid ];
        }
      }
      catch( e )
      {
        console.log( e );
        console.log( "Body: " + body );
      }
    }

    this.handlers.reg = ( pathparts, req, res, body ) =>
    {
      try
      {
        console.log( req.method );
        console.log( "reg" );
        console.log( pathparts )
        console.log( body );
      }
      catch( e )
      {
        console.log( e );
      }
    }
  }

  set onnewcall( callback )
  {
    this.onnewcallcallback = callback;
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

        if( pathparts[ 0 ] in this.handlers )
        {
          res.writeHead( 200, { "Content-Length": "0" } );
          this.handlers[ pathparts[ 0 ] ]( pathparts, req, res, Buffer.concat( this.collatedbody ).toString() );
          this.collatedbody = [];
        }
        else
        {
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
}


module.exports = new projectcontrol();
