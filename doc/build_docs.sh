#!/bin/bash
# Automatically generates the docs (skip if we are building a pull request)
set -ev
if [ "$TRAVIS_PULL_REQUEST" = "false" ] && [ "$USE_SYSTEM_EXPAT" = "OFF" ] && [ "$TRAVIS_PYTHON_VERSION" = "3.6" ]; then
    # Extracts the source code docs with Doxygen and build the C++ API documentation
    make doc
    # Make sure that GitHub does not skip underscored folders
    touch documentation/html/.nojekyll
    # Build the Python API documentation
    sphinx-build -b html . documentation/html/python
fi
