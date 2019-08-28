# RTP Server

The RTP server offers functionality to process RTP data streams and mix them. Channels will be able to be mixed with other channels or local functions like recording or file playback.

## Dependencies

* ilbc-devel
* spandsp-devel

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


### Transcoding

For some scenarios we may not need to transcode. However we may need to transcode to more than one or more other CODECs. For example, we receive PCMA, which is part of a conference which there are 2 other clients. One asking for G722 and the other PCMU. This may also be the case in the future if we handle video.

As this is the case, the sender channel should always be responsible for transcoding. This way as the sender will know about it's multiple receivers it can transcode for all receivers. If there are 2 requiring the same CODEC we only need to transcode once. We can also keep the intermediate L16 for the different CODECs.

### Measurement

How are we going to measure it. It - the workload. Using any strategy it would be impossible to balance workload across all CPUs evenly. In fact - a multi thread application would probably be more successful at this than this strategy. We probably need to report back to our control server a number indicative of if we are at capacity or not.

## Control Server

Each control server can farm out RTP sessions to any number of RTP servers it knows about. It should have control about which server it uses and also have the ability to spin up extra resources if required in cloud environments.

Some of the challenges which is bothering me at the moment:

* If we decide to shut down some of our RTP resource but one server has a call still hanging on what do we do?
  * We could re-invite that call to use a different server but that would cause problems if a resource was being used for that call, for example, the call is being recorded to a local disk before upload. This may mean we have to limit this type of function to mountable shared disk resources.
* Each RTP server should report back how stressed it is. This information can be used in the decision making of where to place a call to.

Each channel needs a control URL so it can send information back to the control server if required. For example, DTMF, or file finished playing and so on.

## Utils

### Tone generation

In order to make the RTP as scalable as possible, we will not support on the fly tone generation. Currently disk space is much cheaper than CPU resources. 1S of wav data sampled at 8K is 16K. Using soundsoup we can provide wav files for each supported codec and easily loop which requires very little CPU overhead.

What we need to provide is a utility to generate wav files which will generate tones for use in telecoms (i.e. ringing, DTMF etc).

project-rtp --tone 350+440*0.75:1000 dialtone.wav

The format attempts to closely follow the format in https://www.itu.int/dms_pub/itu-t/opb/sp/T-SP-E.180-2010-PDF-E.pdf - although that standard is not the clearest in some respects.

We currently support

* '+' adds 1 or more frequencies together
* '*' reduces the amplitude (i.e. aptitude multiply by this value, 0.75 is 0.75 x max value)
* ':' values after the colon are cadences, i.e. duration before the tone moves onto the next tone
* '~' start~end, for example this can be used in frequency or amplitude, 350+440*0.5~0:100 would create a tone starting at amp 0.5 reducing to 0 through the course of the 1000mS period

Examples

#### UK Dial tone:
project-rtp --tone 350+440*0.75:1000 dialtone.wav

#### UK Ringing
project-rtp --tone 400+450*0.5/0/400+450*0.5/0:400/200/400/2000 ringing.wav

#### DTMF

||1209Hz|1336Hz|1477Hz|1633Hz|
|---|---|---|---|---|
|697Hz|1|2|3|A|
|770Hz|4|5|6|B|
|852Hz|7|8|9|C|
|941Hz|*|0|#|D|

Example, 1 would mix 1209Hz and 697Hz

project-rtp --tone 697+1209*0.5:400 dtmf1.wav

project-rtp --tone 697+1209*0.5/0/697+1336*0.5/0/697+1477*0.5/0/:400/100 dtmf1-3.wav
