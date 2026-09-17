#!/bin/bash

cur_pwd=$(pwd)
mkdir -p "$cur_pwd/build"
rm -rf "$cur_pwd/build/*"
cd "$cur_pwd/build"
cmake -DCMAKE_BUILD_TYPE=Debug -DCASTLE_BUILD_SAMPLES=ON ..
make -C "$cur_pwd/build" -j$(nproc)
cd "$cur_pwd"