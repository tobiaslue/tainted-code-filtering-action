#!/bin/sh -l

# Execute here
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
sudo git checkout $branch
sudo apt-get install python-clang-3.9


# export PYTHONPATH=/home/tobiaslue/llvm-project/clang/bindings/python
# export LD_LIBRARY_PATH=/home/tobiaslue/llvm-project/build/lib/

sudo python3 /home/docker/parse-diff.py master branch test.cpp
