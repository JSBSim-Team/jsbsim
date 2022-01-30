import os
import sys

from setuptools.command.install import install
from setuptools.dist import Distribution

cmd = install(Distribution())
cmd.ensure_finalized()
path = cmd.install_lib

if "VIRTUAL_ENV" in os.environ:
    path = os.path.relpath(path, os.environ["VIRTUAL_ENV"])

sys.stdout.write(path)
