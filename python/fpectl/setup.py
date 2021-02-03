# Setup script for the fpectl module.
#
# Copyright (c) 2014-2020 Bertrand Coconnier
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>
#

import os, sys

from setuptools import setup
from setuptools.extension import Extension
from setuptools.command.build_ext import build_ext
from distutils import log


# Performs a build which verbosity is driven by VERBOSE
class QuietBuild(build_ext):
    def run(self):
        if "VERBOSE" not in os.environ:
            name = self.extensions[0].name
            log.info("building '{}' extension".format(name))

            self.oldstdout = os.dup(sys.stdout.fileno())
            self.devnull = open(os.path.join(self.build_lib, name+'-build.log'), 'w')
            os.dup2(self.devnull.fileno(), sys.stdout.fileno())

        build_ext.run(self)

        if "VERBOSE" not in os.environ:
            os.dup2(self.oldstdout, sys.stdout.fileno())
            self.devnull.close()

setup(
    name="fpectl",
    cmdclass={'build_ext': QuietBuild},
    ext_modules=[Extension('fpectl',
                           sources=['fpectlmodule.cpp'],
                           language='c++')])
