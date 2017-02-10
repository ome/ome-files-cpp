help:
	@echo "make html|pdf|linkcheck|all|clean"

all: man html pdf linkcheck

configure: build/CMakeCache.txt
build/CMakeCache.txt: CMakeLists.txt conf.py.in
	rm -rf build && \
	mkdir build && \
	cd build && \
	cmake -G "Unix Makefiles" -Dome-files_VERSION_MAJOR=X -Dome-files_VERSION_MINOR=Y -Dome-files_VERSION=X.Y.Z -Dsphinx:BOOL=ON -Dsphinx-pdf:BOOL=ON ..

man: configure
	cd build && \
	make man

html: configure
	cd build && \
	make doc-html

pdf: configure
	cd build && \
	make doc-pdf

linkcheck: configure
	cd build && \
	make doc-linkcheck

clean:
	rm -rf build

.PHONY: all html pdf configure clean
