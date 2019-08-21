
"use strict";


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

```js
projectcontrol.onnewcall = ( call ) =>
{
  call.ring();
}
```

As the '=>' syntax alters the this pointer we pass in the call object. We also support:

```js
projectcontrol.onnewcall = function()
{
  this.ring();
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

### error
If an error occures then an error object will be set on the call object

## TODO

- [] Finish off onhangup

*/

class call
{
  constructor( control, callinfo )
  {
    this.control = control;
    this.callinfo = callinfo;
    this.metadata = {};

    /* A call's family. It can only have 1 parent but can have multiple children */
    this.metadata.family = {};
    this.metadata.family.children = [];

    /* Our callback handlers */
    this.onnewcallcallback = [];
    this.onringingcallback = [];
    this.onanswercallback = [];
    this.onhangupcallback = [];


    /* Some default handlers */
    this.onhangup = () =>
    {
      console.log("hanging up from call onhangup")
      for ( let child of this.metadata.family.children )
      {
        child.hangup();
      }

      if( "parent" in this.metadata.family )
      {
        /* we only hangup the parent, if the parent only has us as a child */
        var filtered = this.metadata.family.parent.metadata.family.children.filter( ( value ) =>
        {
          return value != this;
        } );
        this.metadata.family.parent.metadata.family.children = filtered;

        /* We may want to alter this. There may be occasions where we wish further processing of the originating call after any child may hang up. The reason could be passed back into hanup of the parent who then makes the decision. */
        if( 0 == filtered.length ) this.metadata.family.parent.hangup();
      }

      if( "channel" in this.metadata )
      {
        this.control.server( {}, "/channel/" + this.metadata.channel.uuid, "DELETE", this.metadata.channel.control, ( response ) =>
        {
          delete this.control.channels[ this.metadata.channel.uuid ];
        } );
      }
    }

    this.onanswer = () =>
    {
      /*
        them = their sdp - so information like where we send RTP to.
        us = our sdp - where rtp should be sent to. this was issued from information from our RTP server so the RTP server does not need updating.
      */
      if( !( "sdp" in this.callinfo ) || !( "them" in this.callinfo.sdp ) )
      {
        return;
      }

      var request = {};

      if( "c" in this.callinfo.sdp.them && Array.isArray( this.callinfo.sdp.them.c ) )
      {
        request.ip = this.callinfo.sdp.them.c[ 0 ].address;
      }

      for( let m of this.callinfo.sdp.them.m )
      {
        if( "audio" == m.media )
        {
          request.port = m.port;
          request.audio = {};
          request.audio.payloads = m.payloads;
          break;
        }
      }

      /* Only include payloads we find acceptable ourself */
      var ourpayloads = [];
      for( let m of this.callinfo.sdp.us.m )
      {
        if( "audio" == m.media )
        {
          ourpayloads = m.payloads;
          break;
        }
      }

      request.audio.payloads = request.audio.payloads.filter( payload => ourpayloads.includes( payload ) );

      this.control.server( request, "/channel/" + this.metadata.channel.uuid, "PUT", this.metadata.channel.control, ( response ) =>
      {
        if( "parent" in this.metadata.family )
        {
          this.metadata.family.parent.onanswer = () => 
          {
            if( "channel" in this.metadata.family.parent.metadata )
            {
              var url = "/channel/";
              url += this.metadata.channel.uuid;
              url += "/mix/";
              url += this.metadata.family.parent.metadata.channel.uuid;
    
              this.control.server( {}, url , "PUT", this.metadata.channel.control, ( response ) =>
              {
                
              } );
            }
          }
          console.log( "Answering parent" )
          this.metadata.family.parent.answer();
        }

      } );
    }

    this.onringing = () =>
    {
      if( "parent" in this.metadata.family )
      {
        this.metadata.family.parent.ring();
      }
    }
  }

  set info( callinf )
  {
    this.callinfo = callinf;
  }

  get info()
  {
    return this.callinfo;
  }

  set onnewcall( cb )
  {
    this.onnewcallcallback.push( cb );
    return this;
  }

  set onringing( cb )
  {
    this.onringingcallback.push( cb );
    return this;
  }

  set onanswer( cb )
  {
    this.onanswercallback.push( cb );
    return this;
  }

  set onhangup( cb )
  {
    this.onhangupcallback.push( cb );
    return this;
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

/*!md
### destination
Get the destination the user has dialled.
*/
  get destination()
  {
    return this.callinfo.to;
  }

/*!md
### originator
Did we originate the call?
*/
  get originator()
  {
    if( "originator" in this.callinfo )
    {
      return true == this.callinfo.originator;
    }
    return false;
  }

  get haserror()
  {
    return "error" in this;
  }

/*!md
### answer
Request is similar to new call. As the call is already (partly) established all we can have is optionally the list of codecs.
request = { codecs: [ 'pcmu' ] }
*/
  answer( request )
  {
    if( this.answered ) 
    {
      /* If a call has already been answered, we schedule running these callbacks as they will have been recently added. */
      for( let cb of this.onanswercallback )
      {
        setTimeout( () => { cb.call( this, this ) }, 0 );
      }
      this.onanswercallback = [];
      return;
    }

    if( undefined == request ) request = {};

    this.control.createchannel( this, request, ( channel ) =>
    {
      if ( undefined == channel )
      {
        this.hangup();
        return;
      }

      this.metadata.channel = channel;

      this.postrequest( "answer", { "sdp": request.sdp } );
    } );
  }

  ring( alertinfo )
  {
    if( this.ringing || this.answered ) 
    {
      for( let cb of this.onringingcallback )
      {
        setTimeout( () => { cb.call( this, this ) }, 0 );
      }
      this.onringingcallback = [];

      for( let cb of this.onanswercallback )
      {
        setTimeout( () => { cb.call( this, this ) }, 0 );
      }
      this.onanswercallback = [];
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
    if( this.hungup ) return;

    this.postrequest( "hangup", { "reason": "Not found", "code": 404 } );
  }

  paymentrequired()
  {
    if( this.hungup ) return;

    this.postrequest( "hangup", { "reason": "Payment required", "code": 402 } );
  }

  busy()
  {
    if( this.hungup ) return;

    this.postrequest( "hangup", { "reason": "Busy here", "code": 486 } );
  }

  hangup()
  {
    if( this.hungup ) return;

    this.postrequest( "hangup", {} );
  }

/*!md
### newcall

Similar to the newcall method on the control object except this method calls 'from' an exsisting call. The caller only needs to set the desitnation. This method sets up the call, then if sucsessful will bridge RTP.

forward = false means it will pass this call off to our SIP server (local), forward = true we will still use SIP but use one of our Gateways.

request is a request object as defined in projectsipcontrol.newcall.

TODO
- [] Improve CID
*/
  newcall( request )
  {
    if( !( "to" in request ) || !( "user" in request.to ) )
    {
      return;
    }

    if( !( "maxforwards" in request ) )
    {
      if( "maxforwards" in this.callinfo )
      {
        request.maxforwards = this.callinfo.maxforwards - 1;
      }
      else
      {
        request.maxforwards = 70;
      }
    }

    if( !( "from" in request ) )
    {
      request.from = {};
      request.from.domain = this.callinfo.domain;
      request.from.user = this.callinfo.from;
    }

    if( !( "cid" in request ) ) request.cid = {};
    if( !( "number" in request.cid ) ) request.cid.number = request.from.user;
    if( !( "name" in request.cid ) ) request.cid.name = request.from.user;

    var call = this.control.newcall( request );
    call.metadata.family.parent = this;
    this.metadata.family.children.push( call );

    return call;
  }

/*!md
### postrequest
Generic purpose fuction to post data to a SIP server.
*/
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
    this.onhangupcallbacks = [];

    this.handlers = { 'PUT': {}, 'POST': {}, 'DELETE': {}, 'GET': {} };

    /* Store by call-id */
    this.calls = {}

    /* Reverse channel uuid to call id */
    this.channels = {}

    /* TODO - work out if we need more and how to add. */
    this.sip = {};
    this.sip.host = "127.0.0.1";
    this.sip.port = 9000;

    this.us = {};
    this.us.host = "127.0.0.1";
    this.us.port = 9001;

    this.rtp = {};
    this.rtp.host = "127.0.0.1";
    this.rtp.port = 9002;

    this.codecs = [ "pcma", "pcmu", "722", "ilbc",  "2833" ];
    this.sessionidcounter = 1;

    this.gateways = [];

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

          for( let cb of this.onnewcallcallbacks )
          {
            cb.call( c, c );
          }
        }
        else
        {
          c = this.calls[ callinfo.callid ];
          c.info = callinfo;
        }

        if( c.ringing && c.onringingcallback.length > 0 )
        {
          for( let cb of c.onringingcallback )
          {
            cb.call( c, c );
          }
          c.onringingcallback = [];
        }

        if( c.answered && c.onanswercallback.length > 0 )
        {
          for( let cb of c.onanswercallback )
          {
            cb.call( c, c );
          }
          c.onanswercallback = [];
        }

        if( c.hungup )
        {
          this.runhangups( c );
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

  runhangups( call )
  {
    for( let cb of this.onhangupcallbacks )
    {
      cb.call( call, call );
    }

    for( let cb of call.onhangupcallback )
    {
      cb.call( call, call );
    }
    call.onhangupcallback = [];

    if( "callinfo" in call && "callid" in call.callinfo )
    {
      delete this.calls[ call.callinfo.callid ];
    }
  }

  runnewcalls( call )
  {
    /* global new call handlers */
    for( let cb of this.onnewcallcallbacks )
    {
      cb.call( call, call );
    }

    /* call specific handlers */
    for( let cb of call.onnewcallcallback )
    {
      cb.call( call, call );
    }
    call.onnewcallcallback = [];
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

  set onhangup( callback )
  {
    this.onhangupcallbacks.push( callback );
  }

/*!md
# sipserver
Send a request to our SIP server.
*/
  sipserver( request, path, method = "PUT", callback )
  {
    return this.server( request, path, method, this.sip, callback );
  }
/*!md
# rtpserver
Send a request to our rtp server. This is work in progress. Simple for now but the goal is to add intelligence to send it out to 1 of many servers and choose which one.
*/
  rtpserver( request, path, method = "PUT", callback )
  {
    return this.server( request, path, method, this.rtp, callback );
  }

  server( request, path, method, server, callback )
  {
    var data = JSON.stringify( request );
    var post_options = {
      "host": server.host,
      "port": server.port,
      "path": path,
      "method": method,
      "headers": {
        "Content-Type": "text/json",
        "Content-Length": Buffer.byteLength( data )
      }
    };

    var post_req = http.request( post_options );

    post_req.on( "error", ( e ) =>
    {
      if( callback )
      {
        callback( { code: 500, message: `Problem with request: ${e.message}, for ${method} ${path}` } );
      }
    } );


    post_req.on( "response", ( req ) =>
    {
      req.on( "data", ( chunk ) =>
      {
        if( !( "collatedbody" in this ) ) this.collatedbody = [];
        this.collatedbody.push( chunk );
      } );

      req.on( "end", () =>
      {
        var body = {};
        if( Array.isArray( this.collatedbody ) )
        {
          try
          {
            if( this.collatedbody.length > 0 )
            {
              body = JSON.parse( Buffer.concat( this.collatedbody ).toString() );
            }
          }
          catch( e )
          {
            console.log( e );
          }
        }
        this.collatedbody = [];
        if( callback )
        {
          callback( { code: req.statusCode, message: req.statusMessage, json: body } );
        }
      } );
    } );

    post_req.write( data );
    post_req.end();
  }

/*!md
# newcall

request object
```json
{
  from: {
    domain: "",
    user: ""
  },
  to: {
    domain: "optional",
    user: ""
  },
  maxforwards: 70,
  cid: {
    name: "",
    number: "",
    private: false
  },
  codecs[ 'pcmu' ],
  forward = false
}
```

A gateway object which we will take settings from if the forward param is set looks like
```json
{
  from: {
    user: "omniis",
    domain: "omniis.babblevoice.com",
    authsecret: "thecatsatonthemat"
  },
  to: {
    domain: "omniis.babblevoice.com",
  }
}
```

The follow list is what we will be taken from a gateway (if it exsists)

* cid
* codecs
* from ( user, domain and authsecret )
* to ( domain )
*/
  newcall( request )
  {
    var c = new call( this, { originator: true } );

    if( !( "maxforwards" in request ) ) request.maxforwards = 70;

    if( "forward" in request && request.forward )
    {
      var gw = this.gw;
      if( "from" in gw )
      {
        if( !( "from" in request ) ) request.from = {};
        if( "user" in gw.from ) request.from.user = gw.from.user;
        if( "domain" in gw.from ) request.from.domain = gw.from.domain;
        if( "authsecret" in gw.from ) request.from.authsecret = gw.from.authsecret;
      }
      if( "to" in gw )
      {
        if( !( "to" in request ) ) request.to = {};
        if( "domain" in gw.to ) request.to.domain = gw.to.domain;
      }
      if( "cid" in gw )
      {
        if( !( "cid" in request ) ) request.cid = {};
        if( "name" in gw.cid ) request.cid.name = gw.cid.name;
        if( "number" in gw.cid ) request.cid.name = gw.cid.number;
        if( "private" in gw.cid ) request.cid.name = gw.cid.private;
      }
      if( "codecs" in gw ) request.codecs = gw.codecs;
    }

    this.createchannel( c, request, ( channel ) =>
    {
      if( !( "sdp" in request ) )
      {
        c.error = { code: 480, message: "Unable to create channel" };
        for( var i = 0; i < c.onnewcallcallback.length; i++ )
        {
          c.onnewcallcallback[ i ].call( c, c );
        }
        return;
      }

      c.metadata.channel = channel;

      this.sipserver( request, "/dialog/invite", "POST", ( response ) =>
      {
        if( 200 == response.code )
        {
          c.callinfo.callid = response.json.callid;

          this.calls[ c.callinfo.callid ] = c;
          this.runnewcalls( c );
        }
        else
        {
          this.runhangups( c );
        }
      } );
    } );

    return c;
  }

/*!md
# createchannel
Negotiates a channel with an RTP server then creates the corrosponding SDP object with CODECS requested. The SDP object is added to the request (as it is required in the SIP INVITE) and then we pass in the channel object to the callback. Currently our RTP servers return:
{
  ip: "",
  port: 10000,
}
*/
  createchannel( call, request, callback )
  {
    request.control = "http://" + this.us.host;
    if( 80 != this.us.port )
    {
      request.control += ":" + this.us.port;
    }

    this.server( request, "/channel/", "POST", this.rtp, ( response ) =>
    {
      if( 200 == response.code )
      {
        var ch = {};
        ch.ip = response.json.ip;
        ch.port = response.json.port;
        ch.control = this.rtp;
        ch.uuid = response.json.uuid;

        /* reverse map from channel uuid to call */
        this.channels[ response.json.uuid ] = call;

        request.sdp = {
          v: 0,
          t: { start: 0, stop: 0 },
          o: {
            username: "-",
            sessionid: this.sessionidcounter,
            sessionversion: 0,
            nettype: "IN",
            ipver: 4,
            address: response.json.ip
          },
          s: " ",
          c: [
            {
              nettype: "IN",
              ipver: 4,
              address: response.json.ip
            } ],
          m: [
              {
                media: "audio",
                port: response.json.port,
                proto: "RTP/AVP",
                direction: "sendrecv",
                payloads: [],
                rtpmap: {},
              }
            ]
        };

        if( !( "codecs" in request ) ) request.codecs = this.codecs;

        ch.codecs = request.codecs;

        if( Array.isArray( request.codecs ) )
        {
          for( var i = 0; i < request.codecs.length; i++ )
          {
            this.addmedia( request.sdp, request.codecs[ i ] );
          }
        }

        this.sessionidcounter = ( this.sessionidcounter + 1 ) % 4294967296;
      }

      callback( ch );
    } );
  }

/*!md
# addmedia
Add a CODEC to the SDP object.
*/
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
      case "pcma":
      {
        sdp.m[ 0 ].payloads.push( 8 );
        sdp.m[ 0 ].rtpmap[ "8" ] = { encoding: "PCMA", clock: "8000" };
        break;
      }
      case "722":
      {
        sdp.m[ 0 ].payloads.push( 9 );
        break;
      }
      case "ilbc":
      {
        sdp.m[ 0 ].payloads.push( 97 );
        sdp.m[ 0 ].rtpmap[ "97" ] = { encoding: "iLBC", clock: "8000" };
        if( !( "fmtp" in sdp.m[ 0 ] ) ) sdp.m[ 0 ].fmtp = {};
        sdp.m[ 0 ].fmtp[ "97" ] = "mode=20";
        break;
      }
      /* rfc 2833 - DTMF*/
      case "2833":
      {
        sdp.m[ 0 ].payloads.push( 101 );
        sdp.m[ 0 ].rtpmap[ "101" ] = { encoding: "telephone-event", clock: "8000" };
        if( !( "fmtp" in sdp.m[ 0 ] ) ) sdp.m[ 0 ].fmtp = {};
        sdp.m[ 0 ].fmtp[ "101" ] = "0-16";
        break;
      }
    }
  }

  directory( domain, users )
  {
    var request = {};
    request.control = "http://" + this.us.host + ":" + this.us.port;
    request.users = users;

    this.sipserver( request, "/dir/" + domain );
  }

  /* We have a list of gateways. This is for when we indicate we wish to forward a call.  */
  set gw( gw )
  {
    this.gateways.push( gw );
  }

  get gw()
  {
    return this.gateways[ 0 ];
  }



/*!md
# run
Our main event loop. Listen for HTTP control events.
*/
  run()
  {
    this.httpserver = http.createServer( ( req, res ) =>
    {
      /*
       Gather our body.
      */
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
}


module.exports = new projectcontrol();
