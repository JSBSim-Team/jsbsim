# TestPythonDefaultLoggerFiltering.py
#
# Test that the default logger is filtering messages according to the log level. 
#
# Copyright (c) 2015-2026 Sean McLeod
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

from typing import final
from JSBSim_utils import (ExecuteUntil, JSBSimTestCase, RunTest)
from jsbsim import (DefaultLogger, LogLevel, set_logger)
import io
import sys

class CapturedDefaultLogger(DefaultLogger):
    def __init__(self):
        super().__init__()
        self.buffer = io.StringIO()

    def file_location(self, filename: str, line: int) -> None:
        original_stdout = sys.stdout
        try:
            sys.stdout = self.buffer
            super().file_location(filename, line)
        finally:
            sys.stdout = original_stdout

    def message(self, message: str) -> None:
        original_stdout = sys.stdout
        try:
            sys.stdout = self.buffer
            super().message(message)
        finally:
            sys.stdout = original_stdout

class TestPythonDefaultLoggerFiltering(JSBSimTestCase):

    def test_logoutput_fatal(self):
        """Confirm that no log output occurs for LogLevel.FATAL"""
        logger = CapturedDefaultLogger()
        logger.set_min_level(LogLevel.FATAL)
        set_logger(logger)

        fdm = self.create_fdm()
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts', 'ball_orbit.xml'))
        fdm.run_ic()
        ExecuteUntil(fdm, 1000.)

        self.assertEqual(logger.buffer.getvalue(), "")

    def test_logoutput_bulk(self):
        """Confirm that log output occurs for LogLevel.BULK"""
        logger = CapturedDefaultLogger()
        logger.set_min_level(LogLevel.BULK)
        set_logger(logger)

        fdm = self.create_fdm()
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts', 'ball_orbit.xml'))
        fdm.run_ic()
        ExecuteUntil(fdm, 1000.)

        self.assertNotEqual(logger.buffer.getvalue(), "")

RunTest(TestPythonDefaultLoggerFiltering)
