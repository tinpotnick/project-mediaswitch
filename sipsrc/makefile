CCOPTS:=-Wall -g -O0
LL:=-lboost_system -lpthread

default: ../Debug/project-s

# Find all sources
ALLSOURCES:=$(shell ls *.cpp)

# Make targets out of them
ALLOBJS=$(ALLSOURCES:%.cpp=../Debug/%.o)

TESTOBJECTS = $(filter-out ../Debug/main.o, $(ALLOBJS))
SOBJECTS = $(filter-out test.o, $(ALLOBJS))

../Debug/project-s: $(SOBJECTS)
	g++ $(SOBJECTS) -o ../Debug/project-s $(LL)

clean:
	rm ../Debug/* -f

debug:
	mkdir -p ../Debug

test: CCOPTS += -DTESTCODE
test: $(TESTOBJECTS)
	g++ $(TESTOBJECTS) -o ../Debug/test $(LL)

../Debug/%.o: %.cpp
	g++ -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@



