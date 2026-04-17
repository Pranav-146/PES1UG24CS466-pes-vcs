#!/usr/bin/env bash
set -euo pipefail

make clean
make pes
rm -rf .pes hello.txt bye.txt file1.txt file2.txt
export PES_AUTHOR="XYZ <PES1UG24AMXXX>"

./pes init
echo "Hello" > hello.txt
./pes add hello.txt
./pes commit -m "Initial commit"

echo "World" >> hello.txt
./pes add hello.txt
./pes commit -m "Add world"

echo "Goodbye" > bye.txt
./pes add bye.txt
./pes commit -m "Add farewell"

./pes log
test -s .pes/refs/heads/main
echo "Integration test passed."
