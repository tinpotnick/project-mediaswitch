CCOPTS:=-Wall -g -O3 -fstack-protector-all -I../projectlib/ -std=c++17
LL:=-L../Debug/ -lproject -lboost_system -lboost_random -lpthread -lcrypto -lresolv

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

default: ../Debug/project-sip

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)

# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o)

../Debug/project-sip: $(ALLOBJS)
	$(LINKER) $(ALLOBJS) -o ../Debug/project-sip $(LL)

clean:
	rm $(ALLOBJS) -f
	rm ../Debug/project-sip

debug:
	mkdir -p ../Debug

../Debug/%.o: %.cpp
	$(COMPILER) -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@
