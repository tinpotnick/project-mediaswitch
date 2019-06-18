# project mediaswitch

**This project is work in progress - there is nothing to use yet unless you wish to get involved with the ongoing development of this project - we would love to include some collaborators.**

Project mediaswitch is designed to be a scalable VOIP switch. Forget a Swiss-army knife supporting 20 protocols bridging old ISDN routes with H323 or SIP. This project is designed to be slim, efficient and fast using an event-driven, asynchronous architecture. SIP, call control and RTP.

The design is broken down into 3 core components. Each component is primarily a single threaded architecture with I/O completion ports for network (think Nginx over Apache). This is to:

1. Increase reliability by removing the possibility of race conditions
2. Reduce the need to lock threads using mutex's or equivalents
3. Increase the scalability - green threads with I/O completion ports have a track record of much better efficiency

Note re green threads - otherwise known as cooperative multi-threading. The one drawback with using this technique is your software has to cooperate with other threads - i.e. give up processing time. The SIP component uses C++ Boost ASIO and call control uses Node JS - both very good frameworks for this style. Both are high performance frameworks. RTP also uses C++ Boost ASIO.

For most users this means when you are writing Node scripts for your own call control scenarios, you must understand the asynchronous nature of Node and how to properly write asynchronous Javascript. **You can lock up the whole control thread by not giving up processor time.** However, when you get it write, you end up with a very efficient server.

The 3 main components:

* project-control: Call control server
* project-sip: A SIP server
* project-rtp: An RTP server

Each component can run either on the same server or on separate servers. This allows, for example, server 1 running SIP and Call control, then if you heavily transcode which increases the load of the RTP servers, multiple RTP servers.

Events are communicated via a HTTP event mechanism. i.e. if a new SIP call is generated by a client (which sends a SIP INVITE) then project-sip will pass a HTTP request back to the control server. Control server will then communicate back instructions to both the SIP server and the RTP server(s).

The design of this project is designed to work with cloud services so that workloads can be scaled up and down appropriately.

Part of the design is to be in-memory. i.e. the SIP server should no be required to query a database to get directory information. After starting, directory information should be pushed to the SIP server (either by the control server or your web site - a friendly host) - think memcache.

# Interfaces

All three projects are designed to either run on the same physical server or separate servers. This way load balancing between servers can be achieved. Multiple RTP servers can then be run to handle large volumes of transcoding for every SIP and control server.

All services communicate with each other via HTTP. The following section defines the interfaces. In this section all of the examples use curl to get or post data.

## project-control

Like project-sip, control has a http interface. This is simplified by the control library, writing call control scripts become simple:

```js
/* Indicate ringing to the caller - did we really need a comment! */
call.ring();
```

### Node

The library can be included with

```js
const projectcontrol = require( "./projectcontrol/index.js" );
```

(note this will change when released!).

The SIP server requires user information to be uploaded to it.

```js
projectcontrol.directory( "bling.babblevoice.com", [ { "username": "1003", "secret": "1123654789" } ] );
```

We would like to be informed about new calls

```js
projectcontrol.onnewcall = ( call ) =>
{
  console.log( "new call" );
}
```

The 'call' object which is passed in contains internal information to track the call. Information regarding if it is ringing, answered, hungup etc. You can also set callback functions on the call

```js
projectcontrol.onnewcall = ( call ) =>
{
  console.log( "new call" );

  call.onhangup = () =>
  {
    console.log( "hung up" );
  }

  /* Indicate ringing to the caller */
  call.ring();

  /* Answer after 3 seconds */
  setTimeout( () =>
  {
    var sdp = projectcontrol.sdp( 20518, "127.0.0.1", 54400 );
    projectcontrol.addmedia( sdp, "pcmu" );

    call.answer( sdp, () =>
    {
      console.log( "Answered" );
    } );
  }, 3000 );

}
```

In this example, we also create an SDP object with sessionid of 20518, host 127.0.0.1 and port 54400. This is where the caller will send its RTP traffic to. See control/projectcontrol/index.js for more information.

Once projectcontrol has been configured run needs calling which places it all in its event loop.

```js
projectcontrol.run();
```

List of setters for call backs in projectcontrol:

* onringing
* onanswer
* onhangup

These can be called multiple times and the callbacks will be stacked and all called.

Getters

* ringing
* answered
* hungup

Methods

* ring
* answer
* hangup (which takes the param reason, if left it is normal hangup, 'busy' is currently supported)

### HTTP

#### POST http://control/invite

Similar to the sip server invite this call will originate a new call - but it will go through the call processing first (compared to the SIP interface which will blindly call the SIP endpoint).


#### PUT http://control/reg

Notify the control server of a SIP registration. Generated by the SIP server and sent to the control server configured using the directory interface.

**PUT http://127.0.0.1:9001/reg/bling.babblevoice.com/1003**
```json
{
  "host": "127.0.0.1",
  "port": 45646,
  "agent": "Z 5.2.28 rv2.8.114"
}
```

The host and port are the client network (where the request came from) and the agent is the agent string reported by the SIP client.

Example:
curl -X POST --data-raw '{ "domain": "bling.babblevoice.com", "user": "1000" }' -H "Content-Type:application/json" http://control/reg

#### DELETE http://control/reg

Generated by the SIP server and sent to the control address against the domain configured using the directory interface. Notify the control server of a SIP de-registration.

**DELETE http://127.0.0.1:9001/reg/bling.babblevoice.com/1003**

Example using curl:
curl -X DELETE --data-raw '{ "domain": "bling.babblevoice.com", "user": "1000" }' -H "Content-Type:application/json" http://control/reg

## project-sip

### PUT http://sip/dir/domain

This interface is used to add directory information to the SIP server.

**PUT http://127.0.0.1:9000/dir/bling.babblevoice.com**
```json
{
  "control": "http://127.0.0.1:9001",
  "users":
  [
    {
      "username": "1003",
      "secret": "1123654789"
    }
  ]
}
```

Returns 201 on success.

Example using curl:
```
curl -X PUT --data-raw '{ "control": "http://127.0.0.1:9001", "users": [ { "username": "1003", "secret": "1123654789"}]}' -H "Content-Type:application/json" http://127.0.0.1/dir/bling.babblevoice.com
```

* domain: the name of the sip domain
* control: a structure of host and port of the control server responsible for this domain
* users: an array of user structures containing username and secret

### PUT http://sip/dir/domain/username

This is synonymous with PATCH.

### PATCH http://sip/dir/domain/username

This will replace the user only. When PUT the domain, this replace the whole domain object.

**PUT http://127.0.0.1:9000/dir/bling.babblevoice.com/1003**
```json
{
  "secret": "1123654789"
}
```

curl -X PUT --data-raw '{ "secret": "1123654789" }' -H "Content-Type:application/json" http://127.0.0.1/dir/bling.babblevoice.com/1003

### GET http://sip/dir/bling.babblevoice.com

Returns JSON listing this domains entry in the directory.

### DELETE http://sip/dir/bling.babblevoice.com

Remove the entry in the directory. The user can also be specified - /dir/bling.babblevoice.com/1003.

### GET http://sip/reg

Returns the number of registered clients.

The example:

**GET http://127.0.0.1:9000/reg/bling.babblevoice.com**


or to filter for a specific user

**GET http://127.0.0.1:9000/reg/bling.babblevoice.com/1003**

Returns 200 with the body:
```json
{
  "domain": "bling.babblevoice.com",
  "count": 3,
  "registered": 1,
  "users": {
    "1000": {
      "registered": false
    },
    "1001": {
      "registered": false
    },
    "1003": {
      "registered": true,
      "outstandingping": 0,
      "remote": {
        "host": "127.0.0.1",
        "port": 42068,
        "agent": "Z 5.2.28 rv2.8.114"
      },
      "epochs": {
        "registered": 1552507958
      }
    }
  }
}
```

When a request is for a specific domain the fields are:

* count - number of users that exist for this domain
* registered - the number of users that are registered
* users - array listing the state of the each user, most of which are self explanatory, outstandingping is the current OPTIONS failure count.


**GET http://127.0.0.1:9000/reg/**

Returns 200 with the body:
```json
{
  "count": 1255
}
```

This is a complete count of all registrations on this SIP server.


### POST http://sip/dialog/invite

Originate a new call.

curl -X POST --data-raw '[{ "domain": "bling.babblevoice.com", "to: "", "from": "", "maxforwards": 70, "callerid": { "number": "123", "name": "123", "private": false }, "control": { "host": "127.0.0.1", "port": 9001 } }]' -H "Content-Type:application/json" http://127.0.0.1/invite

The control option is optional. If it is in this is the server which will receive updates regarding the call flow. If not, the default one listed in the "to" field will be used. If not this, then no updates will be sent.

### POST http://sip/dialog/ring

Example:
curl -X  POST --data-raw '{ "callid": "<callid>", "alertinfo": "somealertinfo" }' -H "Content-Type:application/json" http://sip/dir

If the call is not in a ringing or answered state it will send a 180 ringing along with alert info if sent.


### GET http://sip/dialog

## project-rtp

### POST http://rtp/

Posting a blank document will create a new channel.

Example:
curl -X  POST --data-raw '{}' -H "Content-Type:application/json" http://rtp/

The server will return a JSON document. Including stats regarding the workload of the server so that the control server can make decisions based on workload as well as routing.

# RFCs used in this project

* [RFC 2616](https://tools.ietf.org/html/rfc2616): Hypertext Transfer Protocol -- HTTP/1.1
* [RFC 2617](https://tools.ietf.org/html/rfc2617): HTTP Authentication: Basic and Digest Access Authentication
* [RFC 3261](https://tools.ietf.org/html/rfc3261): SIP: Session Initiation Protocol
* [RFC 3881](https://tools.ietf.org/html/rfc3881): An Extension to the Session Initiation Protocol (SIP) for Symmetric Response Routing
* [RFC 4028](https://tools.ietf.org/html/rfc4028): Session Timers in the Session Initiation Protocol (SIP)
* [RFC 4566](https://tools.ietf.org/html/rfc4566): SDP: Session Description Protocol
* [RFC 4317](https://tools.ietf.org/html/rfc4317): Session Description Protocol (SDP) Offer/Answer Examples
* [RFC 3550](https://tools.ietf.org/html/rfc3550): RTP: A Transport Protocol for Real-Time Applications
* [RFC 3551](https://tools.ietf.org/html/rfc3551): RTP Profile for Audio and Video Conferences with Minimal Control

Note, I have included RFC 4028 here for possible future work.

# Testing

The SIP server can be run with the test flag:

project-sip --test

In the folder testfiles there are also other test files.

The default ports for the server are 9000 for the web server and 5060 for the SIP server.

## Memory

Some notes on using valgrind for memory testing.

### Running memory checking

valgrind --tool=massif project-rtp --fg

After running, this will create a massif file in the directry you run valgrind from. i.e. massif.out.3823
You can use ms_print to pretify th econtents of this file:

ms_print massif.out.3823

### Leak detection

valgrind --leak-check=yes project-rtp --fg

## Registry

registerclient.xml & .csv.

Which are config files to be used with sipp which can test various scenarios.

sipp 127.0.0.1:9997 -sf registerclient.xml -inf registerclient.csv -m 1 -l 1 -trace_msg -trace_err

or without the logging files.

sipp 127.0.0.1:9997 -sf registerclient.xml -inf registerclient.csv -m 1 -l 1

To upload test data to the sip server use

## Invite

sipp 127.0.0.1 -sf uaclateoffer.xml -m 1 -l 1

# TODO

- [ ] Impliment session timers (RFC 4028)
