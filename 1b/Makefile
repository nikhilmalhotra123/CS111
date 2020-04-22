#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: build

build-client:
	gcc lab1b-client.c -o lab1b-client -g -Wall -Wextra -lz

build-server:
	gcc lab1b-server.c -o lab1b-server -g -Wall -Wextra -lz

build: build-client build-server

run: build
	./lab1b-client

client: build-client clean-log
	./lab1b-client --port=3002 --log=logfile --compress

server: build-server
	./lab1b-server --port=3002 --shell=bash --compress

clean-log:
	-@rm -f logfile

clean: clean-log
	-@rm -f lab1b-client
	-@rm -f lab1b-server
	-@rm -f lab1b-505103892.tar.gz
	-@rm -rf lab1b-client.dSYM
	-@rm -rf lab1b-server.dSYM

dist: clean build
	tar -cvzf lab1a-505103892.tar.gz lab1a.c Makefile README
