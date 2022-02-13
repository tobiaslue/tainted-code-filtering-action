#!/bin/bash
set -o xtrace

# Execute here
# sudo chown -R docker .
# chmod -R 700 .
mkdir tmp
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
#sudo git checkout $branch
sudo git checkout test_branch
sudo python3 /home/docker/parse-diff.py master test_branch

echo $(ls)
echo {} > /home/docker/tainted_functions.json
sudo cmake\
    -DCMAKE_CXX_COMPILER=/opt/llvm/bin/clang++1\
    && sudo make