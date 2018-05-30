import sys, os
from distutils.dist import Distribution
from distutils.command.install import install

cmd = install(Distribution())
cmd.ensure_finalized()
path = cmd.install_lib

if "VIRTUAL_ENV" in os.environ:
    path = os.path.relpath(path, os.environ["VIRTUAL_ENV"])

sys.stdout.write(path)
