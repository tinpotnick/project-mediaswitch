CCOPTS:=-Wall -g -O3 -fstack-protector-all -std=c++17

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)

# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o)

COMPILER:=g++
LINKER:=ar

CPUCOUNT=$(shell grep -c   "^processor" /proc/cpuinfo)

ifeq (, $(shell which ccache))
$(error "No ccache in $(PATH), use dnf install ccache or apt-get install ccache")
endif

# Some speed options
## 1. Use ccache (note above warning if it is not avaiable)
CCACHE_EXISTS := $(shell ccache -V)
COMPILER:=ccache $(COMPILER)

## 2. Have a multi flag to re-run with multi jobs
.PHONY: multi
multi:
	$(MAKE) -j$(CPUCOUNT) default

default: ../Debug/libproject.a

../Debug/libproject.a: $(ALLOBJS)
	$(LINKER) -cvq ../Debug/libproject.a $(ALLOBJS)


../Debug/%.o: %.cpp
	$(COMPILER) -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@

clean:
	rm $(ALLOBJS) -f
	rm ../Debug/libproject.a
	ccache -C
