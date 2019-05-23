

const projectcontrol = require( "./projectcontrol/index.js" );

projectcontrol.onhangup = ( call ) =>
{
  console.log( "global hangup" )
}

projectcontrol.onnewcall = ( call ) =>
{
  console.log( "new call" );

  if( call.originator ) return;

  call.onhangup = () =>
  {
    console.log( "hung up" );
  }

  call.onanswer = () =>
  {
    console.log( "Answered" );
    setTimeout( () => { call.hangup(); }, 2000 );
  }

  call.ring();
  setTimeout( () => { call.answer(); }, 2000 );
}

setTimeout( () =>
{

  var callobj = {
    from: {
      user: "1003",
      domain: "bling.babblevoice.com"
    },
    to: {
      user: "1003",
    },
    cid: {
      number: "1003",
      name: "Nick Knight"
    }
  };

  var call = projectcontrol.newcall( callobj )
  call.onnewcall = ( call ) =>
  {
    if( call.haserror )
    {
      console.log( call.error )
      return;
    }

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
  }

  console.log( "Call originated" );

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
