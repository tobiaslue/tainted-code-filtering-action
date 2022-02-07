#!/bin/bash

# Execute here
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
sudo git checkout $branch

sudo python3 /home/docker/parse-diff.py master $branch
