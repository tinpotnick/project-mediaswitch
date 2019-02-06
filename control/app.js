

const http = require( "http" );
const url = require( "url" );

const hostname = "127.0.0.1";
const port = 3000;

function postrequest( data )
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

function answercall( request )
{
  var postdata = JSON.stringify( {
    callid: request.callid,
    action: "answer"
  } );

  postrequest( postdata );
}

function ringingcall( request )
{
  var post_data = JSON.stringify( {
    callid: request.callid,
    action: "ringing"
  } );
  postrequest( postdata );
}

var calls = {}
function handlenewcall( request )
{
  console.log( "handlenewcall" );

  calls[ request.callid ] = request;
  answercall( request );
}


var handlers = {};
handlers[ "/dir" ] = function( req, res, body )
{
  var dir = [
    { "u": "1000", "s": "112233" }
  ]
  res.statusCode = 200;
  res.setHeader( "Content-Type", "application/json" );
  res.end( JSON.stringify( dir ) );
  console.log( "Sending DIR" );

  return;
}

handlers[ "/call" ] = function( req, res, body )
{
  var parsed = JSON.parse( body );
  if( "callid" in parsed )
  {
    if( !( calls[ "callid" ] in calls ) )
    {
      handlenewcall( parsed );
    }
  }
}

const server = http.createServer( ( req, res ) => 
{
  /*********************************************
   Gather our body.
   ********************************************/
  let body = [];
  req.on( "data", ( chunk ) => {

    body.push( chunk );

  } ).on( "end", () => {


    var urlparts = url.parse( req.url );
    if( urlparts.path in handlers )
    {
      handlers[ urlparts.path ]( req, res, Buffer.concat( body ).toString() );
    }
  } );
} );

server.listen( port, hostname, () => 
{
  console.log( `Server running at http://${hostname}:${port}/` );
} );
