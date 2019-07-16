#!/bin/bash
set -ev
if [ "$TRAVIS_PULL_REQUEST" = "false" ] && [ "$USE_SYSTEM_EXPAT" = "OFF" ]; then
    make clean
    make -j2 CFLAGS="--coverage" CXXFLAGS="--coverage"
    lcov -d . -z
    ctest -R Test1
    lcov -d . -c -o tmp.info
    lcov -r tmp.info /usr/include/c++/\* /usr/include/cxxtest/\* */test/unit_tests/\* -o coverage.info
    genhtml -o documentation/html/coverage -t "JSBSim unit tests" coverage.info
fi
