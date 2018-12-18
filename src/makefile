CCOPTS:=-Wall -g -O0
LL:=-lboost_system

default: ../Debug/project-s

../Debug/project-s: ../Debug/main.o ../Debug/projectsipserver.o ../Debug/projectsippacket.o ../Debug/projectwebdocument.o
	g++ ../Debug/main.o ../Debug/projectsipserver.o ../Debug/projectsippacket.o ../Debug/projectwebdocument.o -o ../Debug/project-s $(LL)

clean:
	rm ../Debug/*

debug:
	mkdir -p ../Debug

test: ../Debug/projectsipserver.o ../Debug/projectwebdocument.o ../Debug/projectsippacket.o ../Debug/tests.o
	g++ ../Debug/projectsipserver.o ../Debug/projectsippacket.o ../Debug/projectwebdocument.o ../Debug/tests.o -o ../Debug/test $(LL)

../Debug/main.o: main.cpp
	g++ -c $(CCOPTS) main.cpp -o ../Debug/main.o $(INCLUDE) $(DEFINE)

../Debug/projectsipserver.o: projectsipserver.cpp
	g++ -c $(CCOPTS) projectsipserver.cpp -o ../Debug/projectsipserver.o $(INCLUDE) $(DEFINE)

../Debug/projectsippacket.o: projectsippacket.cpp
	g++ -c $(CCOPTS) projectsippacket.cpp -o ../Debug/projectsippacket.o $(INCLUDE) $(DEFINE)

../Debug/projectwebdocument.o: projectwebdocument.cpp
	g++ -c $(CCOPTS) projectwebdocument.cpp -o ../Debug/projectwebdocument.o $(INCLUDE) $(DEFINE)

../Debug/tests.o: tests.cpp
	g++ -c $(CCOPTS) tests.cpp -o ../Debug/tests.o $(INCLUDE) $(DEFINE)

