#!/bin/bash
set -o xtrace

mkdir /home/docker/tmp
sudo git clone $6 repository
cd repository
# branch=${GITHUB_REF##*/}
sudo git checkout Test_gcd
sudo python3 /home/docker/parse-diff.py $5 Test_gcd


echo {} > /home/docker/tainted_functions.json
echo "1" > /home/docker/counter.txt

$1 && $2

echo {} > /home/docker/tainted_functions.json

$3

cat /home/docker/tainted_functions.json

echo "2" > /home/docker/counter.txt

$4 && $2 && $3

cat /home/docker/tainted_lines.json

# /entrypoint.sh "sudo cmake  -DCMAKE_CXX_COMPILER=/opt/llvm/bin/clang++1" "sudo make"  "./TestLibrary" "sudo make clean" master "https://github.com/tobiaslue/C-Plus-Plus.git"