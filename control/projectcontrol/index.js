
const http = require( "http" );
const url = require( "url" );

/***************************************************************************
Class: call
Purpose: Represents a call which can be controlled.
***************************************************************************/
class call
{
  constructor( control, callinfo )
  {
    this.control = control;
    this.callinfo = callinfo;
  }

  /***************************************************************************
  Set: onringing
  Purpose: Sets a callback when this call receives a ringing.
  ***************************************************************************/
  set onringing( cb )
  {
    this.onringingcallback = cb
  }

  /***************************************************************************
  Set: onanswer
  Purpose: As it says.
  ***************************************************************************/
  set onanswer( cb )
  {
    this.onanswercallback = cb
  }

  /***************************************************************************
  Set: onhangup
  Purpose: As it says.
  ***************************************************************************/
  set onhangup( cb )
  {
    this.onhangupcallback = cb
  }

  /***************************************************************************
  Get: ringing
  Purpose: Test if call is ringing or not.
  ***************************************************************************/
  get ringing()
  {
    if( "ring" in this.callinfo )
    {
      return true == this.callinfo.ring;
    }
    return false;
  }

  /***************************************************************************
  Get: answered
  Purpose: Test if call is answered or not.
  ***************************************************************************/
  get answered()
  {
    if( "answered" in this.callinfo )
    {
      return true == this.callinfo.answered;
    }
    return false;
  }

  /***************************************************************************
  Get: hungup
  Purpose: Test if call is hung up or not.
  ***************************************************************************/
  get hungup()
  {
    if( "hangup" in this.callinfo )
    {
      return true == this.callinfo.hangup;
    }
    return false;
  }

  /***************************************************************************
  Function: answer
  Purpose: Answer a call, call onanswer when confirmed.
  ***************************************************************************/
  answer( onanswer )
  {
    var postdata = {
      action: "answer"
    };
  
    this._onanswer = onanswer;
    this.postrequest( postdata );
  }

  /***************************************************************************
  Function: ring
  Purpose: Send a ringing signal.
  ***************************************************************************/
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

  /***************************************************************************
  Function: hangup
  Purpose: Hang the call up.
  ***************************************************************************/
  hangup( reason )
  {
    if( this.s.hangup )
    {
      return;
    }

    var postdata = {
      reason: reason
    };
  
    this.postrequest( "hangup", postdata );
  }

  /***************************************************************************
  Function: onhangup
  Purpose: Set the callback of this call when the call is hung up.
  ***************************************************************************/
  onhangup( onhangup )
  {
    this._onhangup = onhangup;
  }

  /***************************************************************************
  Function: postrequest
  Purpose: Post a request to a sip or rtp server.
  ***************************************************************************/
  postrequest( action, data )
  {
    data.callid = this.callinfo.callid;
    this.control.sipserver( data, "/dialog/" + action );
  }
}

/***************************************************************************
Function: projectcontrol
Purpose: Object constructor.
***************************************************************************/
class projectcontrol
{
  constructor()
  {
    this.handlers = {}
    this.calls = {}
  
    this.sip = {};
    this.sip.host = "127.0.0.1";
    this.sip.port = 9000;
  
    this.us = {};
    this.us.host = "127.0.0.1";
    this.us.port = 9001;

    /***************************************************************************
      Purpose: HTTP Handlers
    ***************************************************************************/
    this.handlers.invite = ( pathparts, req, res, body ) =>
    {
      try
      {
        var c = new call( this, JSON.parse( body ) );
        if( "onnewcallcallback" in this )
        {
          this.onnewcallcallback( c );
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
      }
      catch( e )
      {
        console.log( e );
      }

      res.writeHead( 200, { "Content-Length": "0" } );
      res.end();
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

      res.writeHead( 200, { "Content-Length": "0" } );
      res.end();
    }
  }

  /***************************************************************************
  Function: onnewcall
  Purpose: Register event handler for a new call.
  ***************************************************************************/
  onnewcall( callback )
  {
    this.onnewcallcallback = callback;
  }

  /***************************************************************************
  Function: sipserver
  Purpose: Communicate with sip server.
  ***************************************************************************/
  sipserver( request, path, method = "PUT" )
  {
    console.log( "Asking SIP to do something." );
    try
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
      post_req.write( data );
      post_req.end();
    }
    catch( error )
    {
      console.log( "Error communicating with SIP server." )
    }
  }

  /***************************************************************************
  Function: invite
  Purpose: New call.
  ***************************************************************************/
  invite( request, callback )
  {
    this.sipserver( request, "/dialog/invite" );
  }

  /***************************************************************************
  Function: listen
  Purpose: Create a new server and listen.
  ***************************************************************************/
  listen()
  {
    this.httpserver = http.createServer( ( req, res ) => 
    {
      /*********************************************
       Gather our body.
      ********************************************/
      let body = [];
      req.on( "data", ( chunk ) => 
      {
        body.push( chunk );
      } );
      
      req.on( "end", () => 
      {
        var urlparts = url.parse( req.url );
        /* Remove the leading '/' */
        var path = urlparts.path.substr( 1 );
        var pathparts = path.split( '/' );
        if( pathparts[ 0 ] in this.handlers )
        {
          this.handlers[ pathparts[ 0 ] ]( pathparts, req, res, Buffer.concat( body ).toString() );
        }
        else
        {
          res.writeHead( 404, { "Content-Length": "0" } );
          res.end();
        }
      } );

    } );

    this.httpserver.listen( this.us.port, this.us.host, () => 
    {
      console.log( `Project Control Server is running on ${this.us.host} port ${this.us.port}` );
    } );
  }

  /***************************************************************************
  Function: directory
  Purpose: Register users with our SIP server and let it know we are the 
  control server for them.
  HTTP PUT /dir/bling.babblevoice.com
  { 
    "control": "http://127.0.0.1:9001",
    "users": 
    [ 
      { "username": "1003", "secret": "mysecret"}
    ]
  }

  ***************************************************************************/
  directory( domain, users )
  {
    var request = {};
    request.control = "http://" + this.us.host + ":" + this.us.port;
    request.users = users;

    this.sipserver( request, "/dir/" + domain );
  }
}


/***************************************************************************
Exports.
***************************************************************************/
module.exports = new projectcontrol();

