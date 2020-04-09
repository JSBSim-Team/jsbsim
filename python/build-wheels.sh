#!/bin/bash
set -e -x

# Compile C++ code
cd /io/build
cmake ..
make

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(int(sys.version_info >= (3,5))))" | grep -q '1'; then
        "${PYBIN}/pip" install cython numpy
        "${PYBIN}/cython" --cplus python/jsbsim.pyx -o python/jsbsim.cxx
        "${PYBIN}/python" python/setup.py bdist_wheel
    fi
done

# Bundle external shared libraries into the wheels
for whl in python/dist/*.whl; do
    auditwheel repair "$whl" --plat manylinux2010_x86_64 -w python/dist
done

# Install packages and test
for PYBIN in /opt/python/*/bin; do
    # Skip deprecated or unsupported versions
    if "${PYBIN}/python" -c "import sys;sys.stdout.write(str(int(sys.version_info >= (3,5))))" | grep -q '1'; then
        "${PYBIN}/pip" install jsbsim --no-index -f python/dist
        "${PYBIN}/python" -c "import jsbsim;fdm=jsbsim.FGFDMExec('.', None);print(jsbsim.FGAircraft.__doc__)"
        "${PYBIN}/JSBSim" --root=.. --script=scripts/c1721.xml
    fi
done
