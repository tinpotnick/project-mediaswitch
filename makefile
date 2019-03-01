

all: ./Debug/project-sip

./Debug/libproject.a:
	$(MAKE) -C projectlib

./Debug/project-sip: ./Debug/libproject.a
	$(MAKE) -C sip

