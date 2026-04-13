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

from JSBSim_utils import (ExecuteUntil, JSBSimTestCase, RunTest)
from jsbsim import (DefaultLogger, LogLevel, set_logger)

class TestPythonDefaultLoggerFiltering(JSBSimTestCase):

    def test_logoutput_fatal(self):
        """Confirm that no log output occurs for LogLevel.FATAL"""
        logger = DefaultLogger()
        logger.set_min_level(LogLevel.FATAL)
        set_logger(logger)

        fdm = self.create_fdm()
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts', 'ball_orbit.xml'))
        fdm.run_ic()
        ExecuteUntil(fdm, 1000.)

        self.assertEqual(logger.buffer, "")

    def test_logoutput_bulk(self):
        """Confirm that log output occurs for LogLevel.BULK"""
        logger = DefaultLogger()
        logger.set_min_level(LogLevel.BULK)
        set_logger(logger)

        fdm = self.create_fdm()
        fdm.load_script(self.sandbox.path_to_jsbsim_file('scripts', 'ball_orbit.xml'))
        fdm.run_ic()
        ExecuteUntil(fdm, 1000.)

        self.assertNotEqual(logger.buffer, "")

RunTest(TestPythonDefaultLoggerFiltering)
