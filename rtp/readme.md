# RTP Server

Some of my thoughts regarding the design of our RTP server to help me with the design!

The design of most aspects of Project Media Switch is single threaded asynchronous. This is to improve both reliability and performance. Telecom's software is well suited to asynchronous design as we receive a packet we process and move onto the next. It seems a bit pointless to give up working on this packet and going to another as we should not spend long on each packet. So having a thread for 'every' connection is a bit redundant.

For the SIP stack this is very appropriate. REGISTER = store the devices location and inform our upper layers in case they wish to act on it. Simple.

The big shortfall with this design is when we have multi core processor systems. Using our SIP stack as an example, this is not much of an issue as we should be able to handle a large number of clients on one core. If we need to go to another core then we are already potentially handling 1000s of clients on the first core. We can do something like separate by domain and publish SRV records (each core's SIP process could run on its own IP address). We can also spread across multiple SIP cores by having multiple SRV records with equal weight. The SIP stack won't support that for a little while yet as it would be required to somehow share information across each process.

It is a bit more tricky with the RTP stack. Although the RTP in terms of state machine in considerably simpler (although it may have more complex tasks such as mixing video) these tasks become more CPU intensive. When we accept a connection we have to ensure that it is somehow farmed out to the relevant resource.

Whichever way we work, the design for RTP is many processes on a one or many machines. Each control server (of which there could be many) would know about one or many RTP servers and farm out calls appropriately. We could use this design by simply running multiple processes of the RTP server on the same machine (one for each core) and use the control server to decide how it farms it out. Or we could add a small amount of complexity into the RTP server which can use multiple cores.

The largest workload we will have is transcoding. We have to ensure we farm this out appropriately to the most appropriate server and on each server the most appropriate core.

## Control Server

Each control server can farm out RTP sessions to any number of RTP servers it knows about. It should have control about which server it uses and also have the ability to spin up extra resources if required in cloud environments.

Some of the challenges which is bothering me at the moment:

* If we decide to shut down some of our RTP resource but one server has a call still hanging on what do we do?
  * We could re-invite that call to use a different server but that would cause problems if a resource was being used for that call, for example, the call is being recorded to a local disk before upload. This may mean we have to limit this type of function to mountable shared disk resources.
* Each RTP server should report back how stressed it is. This information can be used in the decision making of where to place a call to.

Each channel needs a control URL so it can send information back to the control server if required. For example, DTMF, or file finished playing and so on.

## RTP Server

Do we follow the same design pattern as the SIP server which is very much a single core solution. Or do we have a multiple worker threads solution for each server?

Using pthreads (probably via std::threads) you can call

```C++
int main(int argc, const char** argv)
{
  constexpr unsigned num_threads = 4;
  // A mutex ensures orderly access to std::cout from multiple threads.
  std::mutex iomutex;
  std::vector<std::thread> threads(num_threads);
  for (unsigned i = 0; i < num_threads; ++i)
  {
    threads[i] = std::thread([&iomutex, i]
      {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      while (1)
      {
        {
          // Use a lexical scope and lock_guard to safely lock the mutex only
          // for the duration of std::cout usage.
          std::lock_guard<std::mutex> iolock(iomutex);
          std::cout << "Thread #" << i << ": on CPU " << sched_getcpu() << "\n";
        }

        // Simulate important work done by the tread by sleeping for a bit...
        std::this_thread::sleep_for(std::chrono::milliseconds(900));
      }
    });

    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(i, &cpuset);
    int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
  }

  for (auto& t : threads) {
    t.join();
  }
  return 0;
}
```

This sets the affinity of a thread to a specific CPU. (Thank you https://eli.thegreenplace.net/2016/c11-threads-affinity-and-hyperthreading/).

So we can easily tie a thread to a CPU. So what will be our strategy?

#### Idea

We have our normal I/O worker thread. This allows us to be mutex free on all of our control structures - the HTTP server can trigger events events as required, send and receive RTP UDP data. We can then pass off data which requires heavy loads to other threads which are then passed back.

Main I/O thread receives RTP G711 packet. Its channel has a mark showing it is also required in G722 format. It passes this packet off to a worker thread for transcoding and is redelivered to the channel as G722 by the worker thread. The I/O thread can now perform it's magic as to where to send it etc.

The question then becomes - do we need to tie each thread to a specific CPU?

#### Measurement

How are we going to measure it. It - the workload. Using any strategy it would be impossible to balance workload across all CPUs evenly. In fact - a multi thread application would probably be more successful at this than this strategy. We probably need to report back to our control server a number indicative of if we are at capacity or not.

### Automation

It is probably wise to have some automation built into the RTP server. For example, we may decide that we need to play multiple audio files to the client. It would be prudent to have some form of macro (JSON?) to do this instead of having to revert back to the control server.

i.e.

```json
{
  "macro": [
    { "play": "moh", "duration": 30 },
    { "play": "file", "filename": "you are.mp3" },
    { "play": "tts", "first" },
    { "play": "file", "filename": "in the queue" }
  ],
  "loop": true
}
```
