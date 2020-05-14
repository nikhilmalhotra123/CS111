#! /usr/local/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
# 8. wait per lock (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations
#	lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations
#	lab2b_3.png ... successful iterations vs. threads for each synchronization method
#	lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists
# lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

set title "List-1: Total Operations per second vs Threads"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Total Operations per second"
set logscale y 10
set output 'lab2b_1.png'
set key right top

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/mutex' with points lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/spin-lock' with points lc rgb 'green'


set title "List-2: Wait-for-lock and Time per Operation vs Threads"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Time per operations (s)"
set logscale y 10
set output 'lab2b_2.png'
set key right top

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'wait-for-lock' with points lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'time per operation' with points lc rgb 'green'

set title "List-3: Protected and Unprotected Iterations that run without failure"
set xlabel "Threads"
unset xrange
set xrange [0.75:]
unset logscale y
set ylabel "successful iterations"
set yrange [0:100]
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "red" title "unprotected", \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "green" title "Mutex", \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "blue" title "Spin-lock"



set title "List-4: Cost per Operation Mutex vs Threads"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
unset yrange
set ylabel "Total Operations per second"
set logscale y 10
set output 'lab2b_4.png'
set key right top

plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists 1' with points lc rgb 'red', \
    "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title 'lists 4' with points lc rgb 'green', \
    "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title 'lists 8' with points lc rgb 'blue', \
    "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title 'lists 16' with points lc rgb 'black',

set title "List-5: Cost per Operation Spin-Lock vs Threads"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
unset yrange
set ylabel "Total Operations per second"
set logscale y 10
set output 'lab2b_5.png'
set key right top

plot \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists 1' with points lc rgb 'red', \
    "< grep -e 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title 'lists 4' with points lc rgb 'green', \
    "< grep -e 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title 'lists 8' with points lc rgb 'blue', \
    "< grep -e 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title 'lists 16' with points lc rgb 'black',
