

const projectcontrol = require( "./projectcontrol/index.js" );

const hostname = "127.0.0.1";
const port = 9001;

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
  console.log(call)

  setTimeout( function()
  {
    console.log( "Indicating ringing" )
    call.ring();
    setTimeout( function()
    {
      console.log( "now answering" )
      call.answer( onanswer );
    }, 1100 );
  }, 2000 );

} );

setTimeout( function()
{
  //projectcontrol.invite( { realm: "bling.babblevoice.com", to: "1003", from: "1001", maxforwards: 1, callerid: { number: "1001", name: "", private: false } } );
}, 1000 );

projectcontrol.directory( "bling.babblevoice.com", [{ "username": "1003", "secret": "1123654789" } ] );

projectcontrol.listen();

