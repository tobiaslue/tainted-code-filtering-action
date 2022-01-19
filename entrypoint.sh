#!/bin/sh -l

echo "Hello $1"
time=$(date)
echo "::set-output name=time::$time"

 #build project here
cd /
mkdir code 
cd code
git clone git@github.com:tobiaslue/tainted-code-filtering-action.git
branch=${GITHUB_REF##*/}
echo $(git branch)
echo $(git diff HEAD^..HEAD)
echo $(git diff --name-only origin/master origin/${GITHUB_HEAD_REF})
echo $(git diff master $branch)
#Execute taint impact here ?
