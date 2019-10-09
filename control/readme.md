# Call Control Server

Documentation for the objects and interfaces provided in node.

## Projectcontrol object


```JS
const projectcontrol = require( "projectcontrol" )
```

## Call object

A call object can be created by making a call or receiving a call. When you receive a call it looks something like this.

```JS
const projectcontrol = require( "projectcontrol" )

projectcontrol.onnewcall = ( call ) =>
{

}
```

The call object is now available.

```JSON
{
  "control": [ Object ], /* reference to the projectcontrol object handling the call */ 
  "callinfo":
  {
    "originator": true,  /* Is this the originating leg of the call */
    "callid": [ String ],   /* unique id for the call */
    "domain": "bling.babblevoice.com",
    "to": "6",
    "from": "1000",
    "contact": "<sip:1000@192.168.0.114:62001;transport=UDP>",
    "maxforwards": 70,
    "authenticated": true,
    "ring": false,
    "answered": false,
    "hold": false,
    "originator": false,
    "hangup": false,
    "error": { "code": 0, "status": "" },
    "remote": { "host": "192.168.0.114", "port": 62001 },
    "refer": { "to": [ String ], "referedby": [ Object ], "replaces", "callid" },  /* referedby is a reference to the call object which performed the xfer */
    "sdp":
    {
      "them":
      {
        "v": 0, 
        "o": [ Object ], 
        "s": "Z", 
        "c": [ Array ], 
        "t": [ Object ], 
        "m": [ Array ] 
      },
      "us": {} 
    },
    "epochs":
    {
      "start": 1570015248, 
      "ring": 0, 
      "answer": 0, 
      "end": 0 
    }
  },
  "metadata":
  {
    "family": 
    {
      "children": [ Object, Object ], /* array of call objects which have been created as a result of this call */
      "parent": [ Object ] /* we are the child call to this parent call object */
    },
    "channel":
    {
      "ip": "192.168.0.141",
      "port": 10106,
      "control": [ Object ],
      "uuid": [ String ],  /* String referencing the channel on the RTP server */
      "codecs": [ Array ]  /* Array of strings listing the codecs i.e. [ "pcmu", "pcma", "722", "ilbc", 2833" ] */
    }
  }
}
```

The call object also then has getters and setters. Access to the object should use these where possible instead of direct access to the above object.

Metadata is the home to all other data which needs to follow this call. Already take is family and channel. For modules wihch need to tag information, such as a queue module and information regarding the queue would be added in metadata.

### Getters

* info: returns the callinfo object
* moh: returns the music on hold soup. If one hasn't been set for this cal, then will return the default one
* ringing: returns true if the call is ringing
* answered: returns true if the call has been answered
* hungup: returns true if the call has been hungup
* hold: returns true if the call has been placed on hold
* destination: returns the destination of the call (what has been dialled)
* haserror: returns true if an error has occured
* other: returns the other channel this call is linked to (if more than one i.e. multiple children then returns the first), returns false if this call is linked to no others
* others: returns array of other channels we are linked to

### Setters

* onnewcall: add a callback when a new call happens - we have created and have received indication it has happened
* onringing: add a callback when ringing happens
* onanswer: 
* onhold:
* onoffhold:
* onhangup:
* moh: Set a music on hold sound soup

### Methods

* answer
* ring
* notfound
* paymentrequired
* busy
* hangup
* newcall
* play: play a sound soup

NB: There are some functions omitted from this page as they are intended for internal use only (at the moment).
