#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: dist

build:
	gcc lab4b.c -o lab4b -g -Wall -Wextra -lmraa -lm

run: build
	./lab4b

check: build
	./smoketest.sh

clean:
	-@rm -f lab4b-505103892.tar.gz
	-@rm -f lab4b

dist: clean build
	tar -cvzf lab4b-505103892.tar.gz Makefile README lab4b.c smoketest.sh
