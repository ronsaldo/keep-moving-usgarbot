#!/bin/bash
set -ex

mkdir -p build-web
cd build-web
emcmake cmake .. -DCMAKE_BUILD_TYPE="RelWithDebInfo"
emmake make
