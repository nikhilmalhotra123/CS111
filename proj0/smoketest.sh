#NAME: Nikhil Malhotra
#EMAIL: nikhilmalhotra@g.ucla.edu
#ID: 505103892

# Check input/output
echo "test text" > input.txt
./lab0 --input_file=input.txt --output_file=output.txt
if cmp -s "input.txt" "output.txt"; then
  rm -f input.txt output.txt
  continue
else
  echo "Failed input/output test"
  rm -f input.txt output.txt
  exit 1
fi

# Check input from stdin
echo "test text" > input.txt
cat input.txt | ./lab0 --output_file=output.txt
if cmp -s "input.txt" "output.txt"; then
  rm -f input.txt output.txt
  continue
else
  echo "Failed input/output test"
  rm -f input.txt output.txt
  exit 1
fi

# Check output to stdout
echo "test text" > input.txt
./lab0 --input_file=input.txt > output.txt
if cmp -s "input.txt" "output.txt"; then
  rm -f input.txt output.txt
  continue
else
  echo "Failed input/output test"
  rm -f input.txt output.txt
  exit 1
fi

# Check segfault
./lab0 --segfault
if [[ $? -ne 139 ]]; then
  echo "Failed segfault test"
  exit 1
fi

# Check catch
./lab0 --segfault --catch
if [[ $? -ne 4 ]]; then
  echo "Failed catch test"
  exit 1
fi

# Check input/output exit code
echo "test text" > input.txt
./lab0 --input_file=input.txt --output_file=output.txt
if [[ $? -ne 0 ]]; then
  echo "Failed input/output exit code test"
  rm -f input.txt output.txt
  exit 1
fi
rm -f input.txt output.txt

# Check invalid argument exit code
./lab0 --bogus
if [[ $? -ne 1 ]]; then
  echo "Failed argument exit code test"
  exit 1
fi

# Check invalid input file exit code
./lab0 --input_file=file_that_does_not_exist.txt
if [[ $? -ne 2 ]]; then
  echo "Failed invalid input file test"
  exit 1
fi

# Check invalid output file exit code
touch file.txt
chmod -w file.txt
./lab0 --output_file=file.txt
if [[ $? -ne 3 ]]; then
  echo "Failed invalid input file test"
  rm -f file.txt
  exit 1
fi
rm -f file.txt

echo "Success"
