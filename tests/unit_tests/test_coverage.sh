#!/bin/bash
# Automatically generates the coverage reports of the C++ unit tests
set -ev
if [ "$TRAVIS_PULL_REQUEST" = "false" ] && [ "$USE_SYSTEM_EXPAT" = "OFF" ]; then
    make clean
    cmake -DCMAKE_C_FLAGS="--coverage" -DCMAKE_CXX_FLAGS="--coverage" ..
    make -j2
    ctest -R Test1
    lcov -d . -c -o tmp.info
    lcov -r tmp.info /usr/include/c++/\* /usr/include/cxxtest/\* \*/tests/unit_tests/\* -o coverage.info
    genhtml -o documentation/html/coverage -t "JSBSim unit tests" coverage.info
fi
