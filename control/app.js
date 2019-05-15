

const projectcontrol = require( "./projectcontrol/index.js" );

projectcontrol.onnewcall = ( call ) =>
{
  console.log( "new call" );
  //console.log( call.callinfo.sdp )

  call.onhangup = () =>
  {
    console.log( "hung up" );
  }

  call.ring();

  setTimeout( () =>
  {
    var sdp = projectcontrol.sdp( 20518, "127.0.0.1", 54400 );
    projectcontrol.addmedia( sdp, "pcmu" );

    call.answer( sdp, () =>
    {
      console.log( "Answered" );
    } );

    setTimeout( () =>
    {
      call.hangup( "busy" );
    }, 3000 );
  }, 5000 );
}

projectcontrol.onreg = ( reg ) =>
{
  console.log( "onreg" );
  console.log( reg );
}

projectcontrol.ondereg = ( reg ) =>
{
  console.log( "ondereg" );
  console.log( reg );
}

/* Register our user */
projectcontrol.directory( "bling.babblevoice.com", [ { "username": "1003", "secret": "1123654789" } ] );

/* Wait for requests */
projectcontrol.run();
