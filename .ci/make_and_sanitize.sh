#!/usr/bin/env bash
echo -en "travis_fold:start:script.build\\r"
echo "Building..."

set -evx


mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -C../.ci/tsan-cache.cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache $@
cmake --build . -- -j2

set +evx
echo -en "travis_fold:end:script.build\\r"
echo -en "travis_fold:start:script.test\\r"
echo "Testing..."
set -evx

ctest --verbose

set +evx
echo -en "travis_fold:end:script.test\\r"
