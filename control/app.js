

const projectcontrol = require( "projectcontrol" )
const projectqueue = require( "projectqueue" )

projectcontrol.gw = {
  from: {
    user: "1001",
    domain: "omniis.babblevoice.com",
    authsecret: "thecatsatonthemat"
  },
  to: {
    domain: "omniis.babblevoice.com", /*"sipgw.magrathea.net" */
  }
}

/* A soup */
projectcontrol.moh = {
  "loop": true,
  "files":[
    {
      "wav": "uksounds.wav"
    }
  ]
}

// limit the codecs
// projectcontrol.codecs = [ "pcmu" , "2833" ,"pcma" ];

projectcontrol.onreporting = ( call ) =>
{
  console.log( "global hangup" )
  console.log( call.history )
}

queuewav = {}
queuewav.youare = { wav: "queue.wav", start: 400, stop: 1100 }
queuewav.inline = { wav: "queue.wav", start: 1300, stop: 2200 }
queuewav[ "1" ]= { wav: "queue.wav", start: 2600, stop: 3100 }
queuewav[ "2" ]= { wav: "queue.wav", start: 3500, stop: 4100 }
queuewav[ "3" ]= { wav: "queue.wav", start: 4500, stop: 4800 }
queuewav[ "4" ]= { wav: "queue.wav", start: 5400, stop: 5800 }
queuewav[ "5" ]= { wav: "queue.wav", start: 6300, stop: 6600 }
queuewav[ "6" ]= { wav: "queue.wav", start: 7400, stop: 7600 }


projectqueue.onpositionchange = ( call ) =>
{
  var soup = {}
  soup.loop = true
  soup.files = [
    queuewav.youare,
    queuewav[ call.metadata.queue.position ],
    queuewav.inline,
     { wav: "test.wav", start: 3000, stop: 5000 },
  ]

  call.play( soup )
  console.log( "callposchanged" )
  console.log( call.metadata.queue )
}

projectcontrol.onnewcall = ( call ) =>
{
  console.log( "new call inbound call" )
  if( call.haserror ) console.log( call.error )

  if( "1" == call.destination )
  {
    call.onanswer = () =>
    {
      var q = projectqueue.find( "blah" )
      q.queue( call )
    }
    call.answer()

    return;
  }

  if( "2" == call.destination )
  {
    console.log("calling 1001")
    call.newcall( { to: { user: "1001" } } )
    return;
  }

  if( "3" == call.destination )
  {
    console.log("calling 1003")
    call.newcall( { to: { user: "1003" } } )
    return;
  }

  if( "4" == call.destination )
  {
    call.newcall( { to: { user: "somecallrequireingforwarding" }, forward: true } )
  }

  if( "5" == call.destination )
  {
console.log("5")
    call.ring()
    setTimeout( () => { call.answer() }, 2000 )

    call.onanswer = () =>
    {
      console.log( "Answered" )
  
      var soup = {}
      soup.loop = true
      soup.files = []
      soup.files.push( { wav: "test.wav", start: 3000, stop: 5000 } )

      call.play( soup )
      setTimeout( () => { call.hangup(); }, 60000 )
    }
  }

  if( "6" == call.destination )
  {
    call.onanswer = () =>
    {
      var soup = {}
      soup.loop = true
      soup.files = []
      soup.files.push( { wav: "uksounds.wav", start: 1000, stop: 3999, loop: 3 } )
      soup.files.push( { wav: "uksounds.wav", start: 4000, stop: 5499 } )
      soup.loop = true
  
      call.play( soup )
      call.newcall( { to: { user: "1003" } } )
    }
    call.answer()
  }

  if( "7" == call.destination )
  {
    call.onanswer = () =>
    {
      setTimeout( () => { call.hangup(); }, 2000 )
    }
    call.answer()
  }

  if( "8" == call.destination )
  {
    call.onanswer = () =>
    {
      var soup = {}
      soup.loop = true
      soup.files = []
      soup.files.push( { wav: "uksounds.wav", start: 1000, stop: 3999, loop: 3 } )
      soup.files.push( { wav: "uksounds.wav", start: 4000, stop: 5499 } )
      soup.loop = true
  
      call.play( soup )
      var newcall = call.newcall( { to: { user: "1003" } } )
      newcall.onanswer = () =>
      {
        setTimeout( () => { newcall.hangup(); }, 2000 )
      }
    }
    call.answer()
  }

  call.onhangup = () =>
  {
    console.log( "hung up" )
  }
}


/* Our registration handlers */
projectcontrol.onreg = ( reg ) =>
{
  console.log( "onreg" )
  console.log( reg )
}

projectcontrol.ondereg = ( reg ) =>
{
  console.log( "ondereg" )
  console.log( reg )
}

/* Register our user */
projectcontrol.directory( "bling.babblevoice.com",
  [
    { "username": "1003", "secret": "somepassword" },
    { "username": "1000", "secret": "somepassword" },
    { "username": "1001", "secret": "somepassword" }
  ] );

/* Wait for requests */
projectcontrol.run()
