#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

default: build

build:
	gcc lab2_add.c -o lab2_add -g -Wall -Wextra -pthread
	gcc lab2_list.c -o lab2_list -g SortedList.c -Wall -Wextra -pthread

run: build
	./lab2_add --threads=10 --iterations=10000 --sync=m

run-list: build
	./lab2_list --threads=12 --iterations=4 --sync=s --yield=l

clean:
	-@rm -f lab2_add
	-@rm -f lab2_list
	-@rm -f lab2a-505103892.tar.gz
	-@rm -rf lab2_add.dSYM
	-@rm -rf lab2_list.dSYM

dist: clean build
	tar -cvzf lab2a-505103892.tar.gz lab2_add.c lab2_list.c Makefile README
