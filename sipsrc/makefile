CCOPTS:=-Wall -g -O0 -fstack-protector-all
LL:=-lboost_system -lpthread -lcrypto

default: ../Debug/project-sip

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)
CSOURCES:=$(shell ls *.c)


# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o) $(CSOURCES:%.c=../Debug/%.o)

../Debug/project-sip: $(ALLOBJS)
	g++ $(ALLOBJS) -o ../Debug/project-sip $(LL)

clean:
	rm ../Debug/* -f

debug:
	mkdir -p ../Debug

../Debug/%.o: %.cpp
	g++ -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@

../Debug/%.o: %.c
	gcc -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@

