#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: build

build:
	gcc lab2_list.c -o lab2_list -g SortedList.c -Wall -Wextra -pthread

run: build
	./lab2_list --threads=1 --iterations=6 --sync=s --yield=id --lists=5

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
