#!/bin/bash
set -e -x

# Compile C++ code
make
#ctest

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    "${PYBIN}/pip" install -r cython numpy
    "${PYBIN}/python" python/setup.py build_scripts
    "${PYBIN}/python" python/setup.py bdist_wheel
done

# Bundle external shared libraries into the wheels
for whl in python/dist/*.whl; do
    auditwheel repair "$whl" --plat manylinux2010_x86_64 -w /io/python/dist
done

# Install packages and test
for PYBIN in /opt/python/*/bin; do
    "${PYBIN}/pip" install jsbsim --no-index -f python/dist
    "${PYBIN}/python" -c "import jsbsim;fdm=jsbsim.FGFDMExec('.', None);print(jsbsim.FGAircraft.__doc__)"
done
