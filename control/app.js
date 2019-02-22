

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

setTimeout( function()
{
  projectcontrol.invite( { realm: "bling.babblevoice.com", to: "1003", from: "1001", maxforwards: 1, callerid: { number: "1001", name: "", private: false } } );
}, 1000 );

projectcontrol.listen( port, hostname );

