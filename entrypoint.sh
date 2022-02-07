#!/bin/bash

# Execute here
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
sudo git checkout $branch

export LD_LIBRARY_PATH=/opt/llvm/lib


sudo python3 /home/docker/parse-diff.py master $branch
