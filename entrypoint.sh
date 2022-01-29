#!/bin/sh -l

# Execute here
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
sudo git checkout $branch

python3 /home/docker/parse_diff.py master branch test.cpp
