CCOPTS:=-Wall -g -O0 -fstack-protector-all -I../projectlib/ -std=c++17
LL:=-lboost_system -lpthread -lcrypto -L../Debug/ -lproject -lspandsp -lilbc

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

debug:
	mkdir -p ../Debug

../Debug/%.o: %.cpp
	$(COMPILER) -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@
