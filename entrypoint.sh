#!/bin/sh -l

# Execute here
git clone git@github.com:tobiaslue/tainted-code-filtering-action.git
branch=${GITHUB_REF##*/}
echo $(git branch)
echo $(git diff HEAD^..HEAD)
echo $(git diff --name-only origin/master origin/${GITHUB_HEAD_REF})
echo $(git diff master $branch)
