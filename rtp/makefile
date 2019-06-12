CCOPTS:=-Wall -g -O0 -fstack-protector-all -I../projectlib/ -std=c++17
LL:=-lboost_system -lpthread -lcrypto -L../Debug/ -lproject

default: ../Debug/project-rtp

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)

# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o)

../Debug/project-rtp: $(ALLOBJS)
	g++ $(ALLOBJS) -o ../Debug/project-rtp $(LL)

clean:
	rm $(ALLOBJS) -f
	rm ../Debug/project-rtp

debug:
	mkdir -p ../Debug

../Debug/%.o: %.cpp
	g++ -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@
