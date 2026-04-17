#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

export PES_AUTHOR="Sri Pranav Gautam Buduguru <PES1UG24CS466>"
mkdir -p screenshots

clean_lab_state() {
  rm -rf .pes hello.txt bye.txt file1.txt file2.txt src
}

make clean
make all

clean_lab_state
./pes init
./test_objects | tee screenshots/1A-test_objects.txt
find .pes/objects -type f | sort | tee screenshots/1B-object-store-find.txt

./test_tree | tee screenshots/2A-test_tree.txt
tree_path="$(find .pes/objects -type f | sort | tail -1)"
xxd "$tree_path" | head -20 | tee screenshots/2B-tree-object-xxd.txt

clean_lab_state
{
  ./pes init
  echo "hello" > file1.txt
  echo "world" > file2.txt
  ./pes add file1.txt file2.txt
  ./pes status
} | tee screenshots/3A-init-add-status.txt
cat .pes/index | tee screenshots/3B-index-cat.txt

clean_lab_state
{
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
} | tee screenshots/4A-log.txt
find .pes -type f | sort | tee screenshots/4B-pes-find.txt
{
  cat .pes/refs/heads/main
  cat .pes/HEAD
} | tee screenshots/4C-refs.txt

make test-integration | tee screenshots/final-integration.txt
