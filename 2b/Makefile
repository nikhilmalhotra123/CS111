#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

LD_PRELOAD = /Users/nikhilmalhora/Desktop/CS111/2b/thing
CPUPROFILE = ./raw.gperf ./lab2_list

default: build

build:
	gcc lab2_list.c -o lab2_list -g SortedList.c -Wall -Wextra -pthread -lprofiler

run: build
	./lab2_list --threads=12 --iterations=6 --sync=m --yield=id --lists=5

clean:
	-@rm -f lab2_list
	-@rm -f lab2b-505103892.tar.gz
	-@rm -rf lab2_list.dSYM

tests: build
	-./runListtest.sh > lab2_list.csv

graphs: tests
	./lab2_list.gp

dist: build graphs
	tar -cvzf lab2b-505103892.tar.gz lab2_list.c Makefile README SortedList.h SortedList.c runListtest.sh lab2_list.csv lab2_list.gp

profile:
	./thing/bin/pprof --text ./lab2_list ./raw.gperf > profile.out

p:
	pprof --list= ./lab2_list ./raw.gperf >> profile.out
