#!/bin/bash
set -o xtrace

# Execute here
# sudo chown -R docker .
# chmod -R 700 .
mkdir /home/docker/tmp
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
# sudo git checkout $branch
sudo git checkout test
sudo python3 /home/docker/parse-diff.py master test


echo {} > /home/docker/tainted_functions.json
echo "1" > /home/docker/counter.txt

$1 && $2

echo {} > /home/docker/tainted_functions.json

$3

cat /home/docker/tainted_functions.json

echo "2" > /home/docker/counter.txt

$4 && $2 && $3

cat /home/docker/tainted_lines.json
# Create a file with value 0 for the first compile. Change to 1 after the first execution. in the wrapper, check the value of the file, and change the passes based on the value.