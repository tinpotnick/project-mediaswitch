all:
	mkdir -p build; \
	cd build; \
	cmake -DCMAKE_INSTALL_PREFIX=/home/adam/src/staging/usr -DCMAKE_STAGING_PREFIX=/home/adam/src/staging/ ..; \
	$(MAKE)

clean:
	rm -rf build
