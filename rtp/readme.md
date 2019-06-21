# RTP Server

The RTP server offers functionality to process RTP data streams and mix them. Channels will be able to be mixed with other channels or local functions like recording or file playback.

## API

### Channels
*POST /channel*

Channel functions which can create and destroy channels. Channels are pre-allocated at start-up and when a channel is requested by our control server is pulled from the pool and put to use.

This call returns a JSON object:

```json
{
  "channel": "<uuid>",
  "port": 10000,
  "ip": "8.8.8.8"
}
```

*DELETE /channel/<uuid>*

As it says - returns 200 ok on completion.

*PUT /channel/target/<uuid>*

This sets the remote address - where we transmit RTP UDP data to. It is not always needed - as if we receive UDP data from our client we will use that address in preference anyway.

```json
{
  "port": 12000,
  "ip": "8.8.4.4"
}
```

### Mix

*PUT /channel/<uuid>/mix/<uuid>*

Configures both channels to mix together.

## Control Server

Each control server can farm out RTP sessions to any number of RTP servers it knows about. It should have control about which server it uses and also have the ability to spin up extra resources if required in cloud environments.

Some of the challenges which is bothering me at the moment:

* If we decide to shut down some of our RTP resource but one server has a call still hanging on what do we do?
  * We could re-invite that call to use a different server but that would cause problems if a resource was being used for that call, for example, the call is being recorded to a local disk before upload. This may mean we have to limit this type of function to mountable shared disk resources.
* Each RTP server should report back how stressed it is. This information can be used in the decision making of where to place a call to.

Each channel needs a control URL so it can send information back to the control server if required. For example, DTMF, or file finished playing and so on.


### Transcoding

For some scenarios we may not need to transcode. However we may need to transcode to more than one or more other CODECs. For example, we receive PCMA, which is part of a conference which there are 2 other clients. One asking for G722 and the other PCMU. This may also be the case in the future if we handle video.

As this is the case, the sender channel should always be responsible for transcoding. This way as the sender will know about it's multiple receivers it can transcode for all receivers. If there are 2 requiring the same CODEC we only need to transcode once. We can also keep the intermediate L16 for the different CODECs.

### Mixers

I am not sure what to call these yet. So far we have channels. A channel is an RTP & RTCP stream. A Channel is bridged to another channel.

A is a phone at one end, B a phone at the other. Within our RTP server we have 2 channels, channel 1 & 2.

A ---> RTP --->  project channel 1 ---> project channel 2 ---> RTP ---> B

We may need transcoding so we may use another thread to take workload away from our main I/O thread.

We also have other scenarios, a conference room is one example.

A ---> RTP --->  project channel 1 ---> project channel 2 ---> RTP ---> B
                                   ---> project channel 3 ---> RTP ---> C

In both scenarios reverse paths may or may not exist. For example, we may have applications which can listen in on a channel (without sending). Or our conference may allow only 1 speaker at once.

Putting this functionality into a channel seems inappropriate. Mixers all have slightly different functions so we need a base generic mixer with child mixers for different purposes (bridge/conference etc). Generic mixers should also be perhaps where transcoding happens.

Call recording should perhaps happen in the base/generic mixer. All function will have the ability to record a call. Different mixer may want to do it differently. Although, we may want to record on the basis of an aleg though. There are a few challenges with call recording.

The reason it should be in a mixer is transcoding may needs to have happened - i.e. if we receive SOMEBIZARECODECYETTOBESUPPORTED and store in a wav this my not be possible. So transcoding has to happen.

#### Mix

This is simple, a calls b: a is recorded.

A problem arises a calls b, a transfers to c and hangs up. The recording should continue. In this scenario, both b and c are b legs. In terms of our RTP server this is simply the ability to re-open a file. Our control server will need to make the link. I need to verify how this works with both blind and attended xfer.

#### Conference

Arguably, the conference call should be recorded - which would suggest that as soon as there is 1 caller in a conference, the recording should start. When it drops back down to zero then it should stop. This presents problems like, when browsing from traditional call records and select the call the conference recording will span an entirely different timespan. It ill also be referenced by an identify for the conference rather than by a call which entered the conference.

### Measurement

How are we going to measure it. It - the workload. Using any strategy it would be impossible to balance workload across all CPUs evenly. In fact - a multi thread application would probably be more successful at this than this strategy. We probably need to report back to our control server a number indicative of if we are at capacity or not.

### Automation

It is probably wise to have some automation built into the RTP server. For example, we may decide that we need to play multiple audio files to the client. It would be prudent to have some form of macro (JSON?) to do this instead of having to revert back to the control server.

i.e.

```json
{
  "macro": [
    { "play": "moh", "duration": 30 },
    { "play": "file", "filename": "youare.mp3" },
    { "play": "tts", "first" },
    { "play": "file", "filename": "inthequeue.mp3" }
  ],
  "loop": true
}
```

Then there is the question - should we support mp3 - this adds a stupid non-scalable workload to a CPU. If we enforce only working with appropriate files then encoding would only ever be one once.

# TODO

 -[] If you use echo (or any other dumb mixing) the timestamp and the sequence number will need to be managed if the application switches.
