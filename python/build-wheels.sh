#!/bin/bash
set -e -x
cd /io/build

export PYTHON_MIN_VERSION=3,6
export PYTHON_MAX_VERSION=3,9

# Compile C++ code
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(sys.version_info <= (${PYTHON_MAX_VERSION})))" | grep -q 'True'; then
        "${PYBIN}/pip" install cmake
        "${PYBIN}/cmake" -DCMAKE_C_FLAGS_RELEASE="-g -O2 -DNDEBUG" -DCMAKE_CXX_FLAGS_RELEASE="-g -O2 -DNDEBUG" -DCMAKE_BUILD_TYPE=Release ..
        # Only build libJSBSim because that's all we need for Python wheels.
        "${PYBIN}/cmake" --build . --target libJSBSim -- -j2
        break
    fi
done

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(sys.version_info <= (${PYTHON_MAX_VERSION})))" | grep -q 'True'; then
        "${PYBIN}/pip" install cython numpy
        "${PYBIN}/cython" --cplus python/jsbsim.pyx -o python/jsbsim.cxx
        "${PYBIN}/python" python/setup.py bdist_wheel --build-number=$GITHUB_RUN_NUMBER
    fi
done

# Bundle external shared libraries into the wheels
for whl in python/dist/*.whl; do
    auditwheel repair "$whl" --plat manylinux2010_x86_64 -w python/dist
done

# Install packages and test
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(sys.version_info <= (${PYTHON_MAX_VERSION})))" | grep -q 'True'; then
        "${PYBIN}/pip" install jsbsim --no-index -f python/dist
        "${PYBIN}/python" -c "import jsbsim;fdm=jsbsim.FGFDMExec('.', None);print(jsbsim.FGAircraft.__doc__)"
        "${PYBIN}/JSBSim" --root=.. --script=scripts/c1721.xml
    fi
done
