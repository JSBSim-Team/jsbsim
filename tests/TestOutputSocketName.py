# Check that socket output names use the same hostname:protocol/port format
# accepted by FGOutputSocket::SetOutputName().
#
# Copyright (c) 2026 JSBSim contributors
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>

from JSBSim_utils import JSBSimTestCase, RunTest


class TestOutputSocketName(JSBSimTestCase):
    def test_socket_output_name(self):
        with open('socket_output.xml', 'w') as output_file:
            output_file.write('<output type="SOCKET" name="localhost" '
                              'protocol="UDP" port="4321"/>')

        fdm = self.create_fdm()
        self.assertTrue(fdm.set_output_directive('socket_output.xml'))
        self.assertEqual(fdm.get_output_filename(0), 'localhost:UDP/4321')

        self.assertTrue(fdm.set_output_filename(0, 'example.com:TCP/9876'))
        self.assertEqual(fdm.get_output_filename(0), 'example.com:TCP/9876')


RunTest(TestOutputSocketName)
