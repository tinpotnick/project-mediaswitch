CCOPTS:=-Wall -g -O0 -fstack-protector-all -I../projectlib/
LL:=-lboost_system -lboost_random -lpthread -lcrypto -L../Debug/ -lproject

default: ../Debug/project-sip

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)

# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o)

../Debug/project-sip: $(ALLOBJS)
	g++ $(ALLOBJS) -o ../Debug/project-sip $(LL)

clean:
	rm $(ALLOBJS) -f
	rm ../Debug/project-sip

debug:
	mkdir -p ../Debug

../Debug/%.o: %.cpp
	g++ -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@


