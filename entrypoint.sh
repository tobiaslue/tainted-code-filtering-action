#!/bin/sh -l

echo "Hello $1"
time=$(date)
echo "::set-output name=time::$time"

 #build project here
#Execute taint impact here ?
