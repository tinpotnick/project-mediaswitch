

const projectcontrol = require( "./projectcontrol/index.js" );

projectcontrol.onnewcall( function( call )
{
  call.ring();
} );

/* Register our user */
projectcontrol.directory( "bling.babblevoice.com", [{ "username": "1003", "secret": "1123654789" } ] );

/* Wait for requests */
projectcontrol.listen();

