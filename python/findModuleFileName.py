import sys
from distutils.dist import Distribution
from distutils.extension import Extension
from distutils.command.build_ext import build_ext

ext = Extension('jsbsim', [])
name = build_ext(Distribution()).get_ext_filename(ext.name)

sys.stdout.write(name)
