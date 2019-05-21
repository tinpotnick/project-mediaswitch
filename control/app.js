

const projectcontrol = require( "./projectcontrol/index.js" );

projectcontrol.onhangup = ( call ) =>
{
  console.log( "global hangup" )
}

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

      setTimeout( () =>
      {
        call.hangup();
      }, 2000 );

    } );

  }, 2000 );
}

setTimeout( () =>
{
  var sdp = projectcontrol.sdp( 20518, "127.0.0.1", 54400 );
  projectcontrol.addmedia( sdp, "pcmu" );

  var callobj = {
    from: {
      user: "1003",
      domain: "bling.babblevoice.com"
    },
    to: {
      user: "1003",
      domain: "bling.babblevoice.com"
    },
    cid: {
      number: "1003",
      name: "Nick Knight"
    },
    sdp: sdp
  };

  projectcontrol.newcall( callobj,
    ( call ) =>
    {

      call.onringing = () =>
      {
        console.log( "Call is ringing Yaya" )
      }

      call.onanswer = () =>
      {
        console.log( "outbound call answered :)" )
      }

      call.onhangup = () =>
      {
        console.log( "yay - hung up" );
      }
    },
    ( code, errormessage ) =>
    {
      console.log( code )
      console.log( errormessage )
    } );

}, 1000 );

/* Our registration handlers */
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
