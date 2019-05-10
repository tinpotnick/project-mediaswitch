

const projectcontrol = require( "./projectcontrol/index.js" );

projectcontrol.onnewcall = ( call ) =>
{
  call.onhangup = () =>
  {
    console.log( "hung up" );
  }

  console.log( "new call" );
  call.ring();
}

/* Register our user */
projectcontrol.directory( "bling.babblevoice.com", [ { "username": "1003", "secret": "1123654789" } ] );

/* Wait for requests */
projectcontrol.run();
