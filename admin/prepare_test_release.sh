#!/bin/bash
# Modify the files for a test release 0.99.xx where xx is the value passed
# to the command line.

if [ -z "$1" ]; then
  echo "Please provide a patch version number."
  exit 1
fi

# Dummy version number
sed -ri "s/(PROJECT_VERSION_MAJOR\s\")([0-9]+)(.*)/\10\3/g" CMakeLists.txt
sed -ri "s/(PROJECT_VERSION_MINOR\s\")([0-9]+)(.*)/\199\3/g" CMakeLists.txt
sed -ri "s/(PROJECT_VERSION_PATCH\s\")([0-9]+)(.*)/\1$1\3/g" CMakeLists.txt

# Test commit to TestPyPI
sed -ri "/pypa\/gh-action-pypi-publish@release\/v1/a \        with:\n          repository-url: https:\/\/test.pypi.org\/legacy\/" .github/workflows/cpp-python-build.yml

# Trigger a CodeQL review
sed -ri "s/branches:\s\[master\]/branches: \[master, test-release\/v0.99\]/g" .github/workflows/codeql-analysis.yml

# Commit the changes.
git commit -m "Prepare test release v0.99.$1" CMakeLists.txt JSBSim.vcxproj .github/workflows/cpp-python-build.yml .github/workflows/codeql-analysis.yml
