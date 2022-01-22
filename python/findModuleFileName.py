import sys

from setuptools.command.build_ext import build_ext
from setuptools.dist import Distribution
from setuptools.extension import Extension

ext = Extension(sys.argv[1], [])
name = build_ext(Distribution()).get_ext_filename(ext.name)

sys.stdout.write(name)
