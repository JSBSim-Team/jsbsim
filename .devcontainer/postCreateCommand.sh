#!/bin/bash
set -e

# Install Python dependencies
pip install -r python/requirements.txt

# Configure and build JSBSim
mkdir -p build && cd build
cmake \
  -DCPACK_GENERATOR=DEB \
  -DINSTALL_JSBSIM_PYTHON_MODULE=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSYSTEM_EXPAT=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  ..
cmake --build . --parallel "$(nproc)"
