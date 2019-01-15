project mediasitch:

There are 3 comonents to this project
SIP
Call control
RTP

Testing

The SIP server can be built with the test flag:

make test

Which will build siptest which will run through a bunch of internal testing
to try and ensure nothing is broken.

In the folder testfiles there are also other test files.

REGISTER_client.xml & .csv.

Which are config files to be used with sipp which can test various
scenarious.

sipp 192.168.0.141 -sf REGISTER_client.xml -inf REGISTER_client.csv -m 1 -l 1 -trace_msg -trace_err   
