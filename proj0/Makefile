all: build

build:
	gcc lab0.c -o lab0 -g -Wall -Wextra

check: build
	./smoketest

.PHONY: clean
clean:
	-rm lab0
	-rm -R lab0.dSYM
	-rm lab0-505103892.tar.gz

dist:
	tar -czf lab0-505103892.tar.gz lab0.c Makefile README
