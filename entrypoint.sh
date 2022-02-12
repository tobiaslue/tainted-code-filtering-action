#!/bin/bash
set -o xtrace

# Execute here
sudo chown -R docker .
chmod -R 700 .
mkdir tmp
git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
#sudo git checkout $branch
git checkout test_branch
python3 ../parse-diff.py master test_branch

echo $(ls)
echo {} > /home/docker/tainted_functions.json
cmake\
    -DCMAKE_CXX_COMPILER=/opt/llvm/bin/clang++1\
    && make