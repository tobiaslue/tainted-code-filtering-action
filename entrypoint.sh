#!/bin/sh -l

echo "Hello $1"
time=$(date)
echo "::set-output name=time::$time"

cd tainted-code-filtering-action
cmake -DCAMKE_C_COMPILER=/usr/local/clang_9.0.0/bin/clang -DCMAKE_CXX_COMPILER=/usr/local/clang_9.0.0/bin/clang++ -DLLVM_DIR=/opt/llvm -DLIBCXX_PATH=/opt/llvm/lib -DJSONCPP_PATH=/usr/local/json