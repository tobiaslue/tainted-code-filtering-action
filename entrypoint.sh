#!/bin/sh -l

# Execute here
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
sudo git fetch
sudo git checkout $branch
echo $(ls)
echo $(git branch)
echo $(git diff HEAD^..HEAD)
echo $(git diff --name-only master ${GITHUB_HEAD_REF})
echo $(git diff master $branch)
