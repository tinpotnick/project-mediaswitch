
const http = require( "http" );
const url = require( "url" );

/***************************************************************************
Function: projectcontrol
Purpose: Object constructor.
***************************************************************************/
var projectcontrol = function()
{
  this.handlers = {};
  this.calls = {};
  var projectcontrolobj = this;

  this.sip = {};
  this.sip.host = "127.0.0.1";
  this.sip.port = 9000;

  this.us = {};
  this.us.host = "127.0.0.1";
  this.us.port = 9001;

  var callobj =
  {
    /***************************************************************************
    Function: answer
    Purpose: Answer a call, call onanswer when confirmed.
    ***************************************************************************/
    answer: function( onanswer )
    {
      var postdata = {
        action: "answer"
      };
    
      this._onanswer = onanswer;
    
      this.postrequest( postdata );
    },
    /***************************************************************************
    Function: ring
    Purpose: Send a ringing signal.
    ***************************************************************************/
    ring: function( alertinfo )
    {
      if( this.s.ring || this.s.answered )
      {
        return;
      }

      var postdata = {};

      if( undefined != alertinfo )
      {
        postdata.alertinfo = alertinfo;
      }

      this.postrequest( "ring", postdata );
    },
    /***************************************************************************
    Function: ring
    Purpose: Send a ringing signal.
    ***************************************************************************/
    busy: function( alertinfo )
    {
      if( this.s.answered )
      {
        return;
      }

      this.postrequest( "busy", {} );
    },
    /***************************************************************************
    Function: hangup
    Purpose: Hang the call up.
    ***************************************************************************/
    hangup: function( reason )
    {
      if( this.s.hangup )
      {
        return;
      }

      var postdata = {
        reason: reason
      };
    
      this.postrequest( "hangup", postdata );
    },
    /***************************************************************************
    Function: onhangup
    Purpose: Set the callback of this call when the call is hung up.
    ***************************************************************************/
    onhangup: function( onhangup )
    {
      this._onhangup = onhangup;
    },
    /***************************************************************************
    Function: postrequest
    Purpose: Post a request to a sip or rtp server.
    ***************************************************************************/
    postrequest: function( action, data )
    {
      data.callid = this.s.callid;
      projectcontrol.sipserver( data, "/dialog/" + action );
    }
  }

  /***************************************************************************
  Purpose: HTTP Handlers
  ***************************************************************************/
  this.handlers[ "invite" ] = function( req, res, body )
  {
    try
    {
      var c = Object.assign( { s: JSON.parse( body ) }, callobj );   
      if( "callid" in c.s )
      {
        if( !( c.s.callid in projectcontrolobj.calls ) )
        {
          projectcontrolobj.calls[ c.s.callid ] = c;
          if( "onnewcallcallback" in projectcontrolobj )
          {
            projectcontrolobj.onnewcallcallback( c );
          }
        }

        var ref = Object.assign( projectcontrolobj.calls[ c.s.callid ], c );

        if( ref.s.answered && "_onanswer" in ref )
        {
          ref._onanswer();
          delete ref._onanswer;

        }

        if( ref.s.hangup )
        {
          if( "_onhangup" in ref )
          {
            ref._onhangup();
          }
          
          delete projectcontrolobj.calls[ ref.callid ];
        }
      }
    }
    catch( e )
    {
      console.log( e );
    }
  }
}

/***************************************************************************
Function: onnewcall
Purpose: Register event handler for a new call.
***************************************************************************/
projectcontrol.prototype.onnewcall = function( callback )
{
  this.onnewcallcallback = callback;
}

/***************************************************************************
Function: sipserver
Purpose: Communicate with sip server.
***************************************************************************/
projectcontrol.prototype.sipserver = function( request, path, method = "POST" )
{
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
projectcontrol.prototype.invite = function( request, callback )
{
  this.sipserver( request, "/dialog/invite" );
}

/***************************************************************************
Function: listen
Purpose: Create a new server and listen.
***************************************************************************/
projectcontrol.prototype.listen = function()
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
      if( path in this.handlers )
      {
        this.handlers[ path ]( req, res, Buffer.concat( body ).toString() );
        res.writeHead( 200, { "Content-Length": "0" } );
        res.end();
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
  "control": 
  { 
    "host": "127.0.0.1", 
    "port": 9001 
  }, 
  "users": 
  [ 
    { "username": "1003", "secret": "mysecret"}
  ]
}

***************************************************************************/
projectcontrol.prototype.directory = function( domain, users )
{
  var request = {};
  request.control = {};
  request.control.host = this.us.host;
  request.control.port = this.us.port;
  request.users = users;

  this.sipserver( request, "/dir/" + domain, "PUT" );
}


/***************************************************************************
Exports.
***************************************************************************/
module.exports = new projectcontrol();

