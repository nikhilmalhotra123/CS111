#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: build

build:
	gcc lab1a.c -o lab1a -g -Wall -Wextra

run: build
	./lab1a

runs: build
	./lab1a --shell=ls

clean:
	-@rm -f lab1a

dist: clean build
	tar -cvzf lab1a-505103892.tar.gz lab1a.c Makefile README
