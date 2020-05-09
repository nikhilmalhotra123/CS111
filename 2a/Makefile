#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: build

build:
	gcc lab2_add.c -o lab2_add -g -Wall -Wextra -pthread
	gcc lab2_list.c -o lab2_list -g SortedList.c -Wall -Wextra -pthread

run: run-add run-list

run-add: build
	./lab2_add --threads=10 --iterations=10000 --sync=m

run-list: build
	./lab2_list --threads=12 --iterations=4 --sync=s --yield=l

clean:
	-@rm -f lab2_add
	-@rm -f lab2_list
	-@rm -f lab2a-505103892.tar.gz
	-@rm -rf lab2_add.dSYM
	-@rm -rf lab2_list.dSYM

tests: build
	./runtest.sh > lab2_add.csv
	-./runListtest.sh > lab2_list.csv

graphs: tests
	./lab2_add.gp
	./lab2_list.gp

dist: build graphs
	tar -cvzf lab2a-505103892.tar.gz lab2_add.c lab2_list.c Makefile README SortedList.h SortedList.c runtest.sh runListtest.sh lab2_add.csv lab2_list.csv lab2_add.gp lab2_list.gp lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
