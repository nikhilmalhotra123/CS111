#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: build

build: clean
	gcc lab0.c -o lab0 -g -Wall -Wextra

check: build
	./smoketest.sh

.PHONY: clean
clean:
	-@rm -f lab0
	-@rm -Rf lab0.dSYM
	-@rm -f lab0-505103892.tar.gz

dist: clean build
	tar -cvzf lab0-505103892.tar.gz lab0.c Makefile README backtrace.png breakpoint.png smoketest.sh
