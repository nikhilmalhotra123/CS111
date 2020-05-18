#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

# Check valid inputs
./lab4b --period=2 --scale=C --log="LOGFILE" <<-EOF
SCALE=F
PERIOD=1
OFF
EOF
ret=$?
if [ $ret -ne 0 ]
then
	echo "VALID INPUT TEST FAILED RC=$ret"
  exit 1
fi

# Check if LOGFILE is created
if [ ! -s LOGFILE ]
then
	echo "LOGFILE TEST FAILED"
  exit 1
fi

# Check correct logs
grep "SCALE=F" LOGFILE > /dev/null
if [ $? -ne 0 ]
then
  echo "FAILED CORRECT LOG TEST"
  exit 1
fi

grep "SHUTDOWN" LOGFILE > /dev/null
if [ $? -ne 0 ]
then
  echo "FAILED SHUTDOWN TEST"
  exit 1
fi

egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9]+\.[0-9]\>' LOGFILE > FOUND
if [ $? -ne 0 ]
then
  echo "FAILED VALID TEMPERATURE FORMAT TEST"
  exit 1
fi

# Check invalid argument exit code
./lab4b --bogus
if [ $? -ne 1 ]; then
  echo "Failed argument exit code test"
  exit 1
fi

echo "Success"
