#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

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
	-./runtest.sh > lab2b_list.csv

graphs: tests
	./lab2_list.gp

dist: build graphs 
	tar -cvzf lab2b-505103892.tar.gz lab2_list.c Makefile README SortedList.h SortedList.c runtest.sh lab2b_list.csv lab2_list.gp profile.out lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png 

profile:
	-@rm -f raw.gperf
	LD_PRELOAD=/u/cs/ugrad/malhotra/CS111/2b/thing/lib/libprofiler.s.0
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	/u/cs/ugrad/malhotra/CS111/2b/thing/bin/pprof --text lab2_list raw.gperf > profile.out
	/u/cs/ugrad/malhotra/CS111/2b/thing/bin/pprof --list=modifyList lab2_list raw.gperf >> profile.out
	-@rm -f raw.gperf
