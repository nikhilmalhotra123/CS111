#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: build

build:
	gcc lab1a.c -o lab1a -g -Wall -Wextra

run: build
	./lab1a

clean:
	-@rm -f lab1a
	-@rm -f lab1a-505103892.tar.gz
	-@rm -rf lab1a.dSYM

dist: clean build
	tar -cvzf lab1a-505103892.tar.gz lab1a.c Makefile README
