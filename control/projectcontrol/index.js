
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

  var callobj =
  {
    /***************************************************************************
    Function: answer
    Purpose: Answer a call, call onanswer when confirmed.
    ***************************************************************************/
    answer: function( onanswer )
    {
      var postdata = JSON.stringify( {
        callid: this.s.callid,
        action: "answer"
      } );
    
      this._onanswer = onanswer;
    
      postrequest( postdata );
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

      var postdata = JSON.stringify( {
        callid: this.s.callid,
        action: "ring"
      } );

      if( undefined != alertinfo )
      {
        postdata.alertinfo = alertinfo;
      }
      postrequest( postdata );
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

      var postdata = JSON.stringify( {
        callid: this.s.callid,
        action: "hangup",
        reason: reason
      } );
    
      postrequest( postdata );
    },
    /***************************************************************************
    Function: onhangup
    Purpose: Set the callback of this call when the call is hung up.
    ***************************************************************************/
    onhangup: function( onhangup )
    {
      this._onhangup = onhangup;
    }
  }

  /***************************************************************************
  Function: onhangup
  Purpose: Set the callback of this call when the call is hung up.
  ***************************************************************************/
  this.handlers[ "call" ] = function( req, res, body )
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
Function: postrequest
Purpose: Post a request to a sip or rtp server.
***************************************************************************/
var postrequest = function( data )
{
  var post_options = {
    host: "127.0.0.1",
    port: "8080",
    path: "/dialog",
    method: "POST",
    headers: {
        "Content-Type": "text/json",
        "Content-Length": Buffer.byteLength( data )
    }
  };

  var post_req = http.request( post_options );
  post_req.write( data );
  post_req.end();
}

/***************************************************************************
Function: listen
Purpose: Create a new server and listen.
***************************************************************************/
projectcontrol.prototype.listen = function( port, hostname )
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
      }
    } );

  } );

  this.httpserver.listen( port, hostname, () => 
  {
    console.log( `Server running at http://${hostname}:${port}/` );
  } );
}


/***************************************************************************
Exports.
***************************************************************************/
module.exports = new projectcontrol();

