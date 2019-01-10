CCOPTS:=-Wall -g -O0
LL:=-lboost_system -lpthread -lcrypto

default: ../Debug/project-s

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)
CSOURCES:=$(shell ls *.c)


# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o) $(CSOURCES:%.c=../Debug/%.o)

TESTOBJECTS = $(filter-out ../Debug/main.o, $(ALLOBJS))
SOBJECTS = $(filter-out test.o, $(ALLOBJS))

../Debug/project-sip: $(SOBJECTS)
	g++ $(SOBJECTS) -o ../Debug/project-sip $(LL)

clean:
	rm ../Debug/* -f

debug:
	mkdir -p ../Debug

test: CCOPTS += -DTESTCODE
test: $(TESTOBJECTS)
	g++ $(TESTOBJECTS) -o ../Debug/siptest $(LL)

../Debug/%.o: %.cpp
	g++ -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@

../Debug/%.o: %.c
	gcc -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@

