
CCOPTS:=-Wall -g -O0 -fstack-protector-all

# Find all sources
CPPSOURCES:=$(shell ls *.cpp)

# Make targets out of them
ALLOBJS=$(CPPSOURCES:%.cpp=../Debug/%.o)

default: ../Debug/libproject.a

../Debug/libproject.a: $(ALLOBJS)
	ar -cvq ../Debug/libproject.a $(ALLOBJS)


../Debug/%.o: %.cpp
	g++ -c $(CCOPTS) $(INCLUDE) $(DEFINE) $< -o $@


clean:
	rm $(ALLOBJS) -f
	rm ../Debug/libproject.a


