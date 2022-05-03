#!/bin/bash
# Modify the files for a test release 0.99.xx where xx is the value passed
# to the command line.

# Dummy version number
sed -ri "s/(PROJECT_VERSION_MAJOR\s\")([0-9]+)(.*)/\10\3/g" CMakeLists.txt
sed -ri "s/(PROJECT_VERSION_MINOR\s\")([0-9]+)(.*)/\199\3/g" CMakeLists.txt
sed -ri "s/(PROJECT_VERSION_PATCH\s\")([0-9]+)(.*)/\1$1\3/g" CMakeLists.txt

# Test commit to TestPyPI
sed -ri "s/secrets.PYPI/secrets.TESTPYPI/g" .github/workflows/cpp-python-build.yml
sed -ri "s/upload dist/upload --repository testpypi dist/g" .github/workflows/cpp-python-build.yml

# Trigger a CodeQL review
sed -ri "s/branches:\s\[master\]/branches: \[master, release\/v1.1\]/g" .github/workflows/codeql-analysis.yml
