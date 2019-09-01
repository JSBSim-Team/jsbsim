#!/bin/bash
# Automatically generates the docs (skip if we are building a pull request)
set -ev
if [ "$TRAVIS_PULL_REQUEST" = "false" ] && [ "$USE_SYSTEM_EXPAT" = "OFF" ] && [ "$TRAVIS_PYTHON_VERSION" = "3.6" ]; then
    # Make sure that GitHub does not skip underscored folders
    touch documentation/html/.nojekyll
    # Build the Python API documentation
    sphinx-build -b html documentation documentation/html/python
fi
