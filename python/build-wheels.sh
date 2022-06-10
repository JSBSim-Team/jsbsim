#!/bin/bash
set -e -x
cd /io/build

export PYTHON_MIN_VERSION=3,7
export PYTHON_MAX_VERSION=3,11

# Compile C++ code
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(sys.version_info >= (${PYTHON_MIN_VERSION})))" | grep -q 'True'; then
        "${PYBIN}/pip" install cmake
        "${PYBIN}/cmake" -DCMAKE_C_FLAGS_RELEASE="-g -O2 -DNDEBUG -fno-math-errno" -DCMAKE_CXX_FLAGS_RELEASE="-g -O2 -DNDEBUG -fno-math-errno" -DCMAKE_BUILD_TYPE=Release ..
        # Only build libJSBSim because that's all we need for Python wheels.
        "${PYBIN}/cmake" --build . --target libJSBSim -- -j2
        break
    fi
done

cd python

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(sys.version_info >= (${PYTHON_MIN_VERSION})))" | grep -q 'True'; then
        "${PYBIN}/pip" install build
        "${PYBIN}/python" -m build --wheel --config-setting=--build-number=$GITHUB_RUN_NUMBER
    fi
done

# Bundle external shared libraries into the wheels
for whl in dist/*.whl; do
    auditwheel repair "$whl" -w dist
done

# Install packages and test
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(sys.version_info >= (${PYTHON_MIN_VERSION})))" | grep -q 'True'; then
        "${PYBIN}/pip" install numpy  # Need numpy installed first as we use `--no-index` below
        "${PYBIN}/pip" install jsbsim --no-index -f dist
        "${PYBIN}/python" -c "import jsbsim;fdm=jsbsim.FGFDMExec('.', None);print(jsbsim.FGAircraft.__doc__)"
        "${PYBIN}/JSBSim" --root=../.. --script=scripts/c1721.xml
    fi
done
