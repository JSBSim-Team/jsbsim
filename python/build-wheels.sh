#!/bin/bash
set -e -x

make clean
make
ctest

for PYBIN in /opt/python/*/bin; do
    "${PYBIN}/pip" install -r cython numpy
    "${PYBIN}/python" python/setup.py build_scripts
    "${PYBIN}/python" python/setup.py bdist_wheel
done

for PYBIN in /opt/python/*/bin; do
    "${PYBIN}/pip" install jsbsim --no-index -f python/dist
    "${PYBIN}/python" -c "import jsbsim;fdm=jsbsim.FGFDMExec('.', None);print(jsbsim.FGAircraft.__doc__)"
done
