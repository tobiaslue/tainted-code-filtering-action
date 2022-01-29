#!/bin/sh -l

# Execute here
sudo git clone https://github.com/tobiaslue/tainted-code-filtering-action.git
cd tainted-code-filtering-action
branch=${GITHUB_REF##*/}
echo $(git branch)
echo $(git diff HEAD^..HEAD)
echo $(git diff --name-only origin/master origin/${GITHUB_HEAD_REF})
echo $(git diff master $branch)
