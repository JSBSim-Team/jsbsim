# CheckFGBug1503.py
#
# A regression test for the bug reported in FG issue 1503
# http://code.google.com/p/flightgear-bugs/issues/detail?id=1503
#
# Copyright (c) 2014 Bertrand Coconnier
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

import os, shutil, time, sys
import xml.etree.ElementTree as et
from multiprocessing import Process
from JSBSim_utils import SandBox, CreateFDM, append_xml

def ScriptExecution(fdm, script_path):
    fdm.load_script(script_path)
    fdm.run_ic()

    while fdm.run():
        pass

sandbox = SandBox()

# Run the script c1724.xml
script_path = sandbox.path_to_jsbsim_file('scripts', 'c1724.xml')

fdm = CreateFDM(sandbox)
start_time = time.time()
ScriptExecution(fdm, script_path)
exec_time = time.time() - start_time

# First, extract the aircraft name from the script
tree = et.parse(sandbox.elude(script_path))
use_element = tree.getroot().find('use')
aircraft_name = use_element.attrib['aircraft']

# Then, create a directory aircraft/aircraft_name in the build directory
aircraft_path = os.path.join('aircraft', aircraft_name)
path_to_jsbsim_aircrafts = sandbox.elude(sandbox.path_to_jsbsim_file(aircraft_path))
aircraft_path = sandbox(aircraft_path)
if not os.path.exists(aircraft_path):
    os.makedirs(aircraft_path)

# Make a copy of the initialization file in build/.../aircraft/aircraft_name
IC_file = append_xml(use_element.attrib['initialize'])
shutil.copy(os.path.join(path_to_jsbsim_aircrafts, IC_file), aircraft_path)

# Modify the rate_limit element and split in 2 elements : one with sense 'decr'
# the other with sense 'incr'.
tree = et.parse(os.path.join(path_to_jsbsim_aircrafts, aircraft_name+'.xml'))
actuator_element = tree.getroot().find('flight_control/channel/actuator//rate_limit/..')
rate_element = actuator_element.find('rate_limit')
rate_element.attrib['sense'] = 'decr'
new_rate_element = et.SubElement(actuator_element, 'rate_limit')
new_rate_element.attrib['sense'] = 'incr'
new_rate_element.text = str(float(rate_element.text) * 0.5)

tree.write(sandbox('aircraft', aircraft_name, aircraft_name+'.xml'))

# The aircraft definition file may already load some data from external files.
# If so, we need to copy these files in our directory build/.../aircraft/aircraft_name
# Only the external files that are in the original directory aircraft/aircraft_name
# will be copied. The files located in 'engine' and 'systems' do not need to be
# copied.
for element in list(tree.getroot()):
    if 'file' in element.keys():
        name = append_xml(element.attrib['file'])
        name_with_path = os.path.join(path_to_jsbsim_aircrafts, name)
        if os.path.exists(name_with_path):
            shutil.copy(name_with_path, aircraft_path)

# Run the script
fdm = CreateFDM(sandbox)
fdm.set_aircraft_path('aircraft')

p = Process(target=ScriptExecution, args=(fdm, script_path))
p.start()
p.join(exec_time * 10.0)
if p.is_alive():
    p.terminate()
    sys.exit(-1)

sandbox.erase()
