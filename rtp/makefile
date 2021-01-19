CCOPTS:=-Wall -g -O0 -fstack-protector-all -std=c++17
LL:=-lboost_system -lpthread -lrt -L../Debug/ -lspandsp -lilbc

COMPILER:=g++
LINKER:=g++

CPUCOUNT=$(shell grep -c   "^processor" /proc/cpuinfo)

ifeq (, $(shell which ccache))
$(error "No ccache in $(PATH), use dnf install ccache or apt-get install ccache")
endif

# Some speed options
## 1. Use ccache (note above warning if it is not avaiable)
CCACHE_EXISTS := $(shell ccache -V)
COMPILER:=ccache $(COMPILER)
LINKER:=ccache $(LINKER)


## 2. Have a multi flag to re-run with multi jobs
.PHONY: multi
multi:
	$(MAKE) -j$(CPUCOUNT) default

default: ../Debug/project-rtp

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)

# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o)

../Debug/project-rtp: $(ALLOBJS)
	$(LINKER) $(ALLOBJS) -o ../Debug/project-rtp $(LL)

clean:
	rm $(ALLOBJS) -f
	rm ../Debug/project-rtp
	ccache -C

debug:
	mkdir -p ../Debug

../Debug/%.o: %.cpp
	$(COMPILER) -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@


# All uk sounds in the same wav file.
# 0 - 1S Dial tone
# 1S - 4S Ringing
# 4S - 12S DTMF tones working across the DTMF Table (i.e. 400mS on 100mS off 1, 2, 3, A, 4, 5, 6, B, 7, 8, 9, C, *, 0, #, D)
# 12S - 13S Unobtainable
# 13S - 13.75S Busy tone
# 13.75S - 15.25S Congestion tone
# 15.25S - 15.5S Pay tone
uk-sounds: ../Debug/project-rtp
	rm ../Debug/uksounds.wav
	../Debug/project-rtp --tone 350+440*0.5:1000 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 400+450*0.5/0/400+450*0.5/0:400/200/400/2000 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 697+1209*0.5/0/697+1336*0.5/0/697+1477*0.5/0/697+1633*0.5/0:400/100 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 770+1209*0.5/0/770+1336*0.5/0/770+1477*0.5/0/770+1633*0.5/0:400/100 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 852+1209*0.5/0/852+1336*0.5/0/852+1477*0.5/0/852+1633*0.5/0:400/100 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 941+1209*0.5/0/941+1336*0.5/0/941+1477*0.5/0/941+1633*0.5/0:400/100 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 440:1000 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 440/0:375/375 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 440/0:400/350/225/525 ../Debug/uksounds.wav
	../Debug/project-rtp --tone 440/0:125/125 ../Debug/uksounds.wav
