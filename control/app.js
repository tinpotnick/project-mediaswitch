

const projectcontrol = require( "./projectcontrol/index.js" );

const hostname = "127.0.0.1";
const port = 3000;

var onhangup = function()
{
  console.log( "onhangup" );
  console.log( this );
}

var onanswer = function()
{
  console.log( "onanswer" );
}

projectcontrol.onnewcall( function( call )
{
  call.onhangup( onhangup );

  setTimeout( function()
  {
    call.ring();
    setTimeout( function()
    {
      call.answer( onanswer );
    }, 1100 );
  }, 2000 );

} );

projectcontrol.listen( port, hostname );

