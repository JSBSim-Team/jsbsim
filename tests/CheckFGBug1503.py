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

# This test checks that:
# 1. The JSBSim does not hang when the parameter 'sense' of actuator
#    <rate_limit> is used.
# 2. The property 'fcs/left-aileron-pos-rad' remains equal to 0.0 during the
#    execution of the script c1724.xml when <rate_limit> is used with a property.

import os, shutil, time, sys
import xml.etree.ElementTree as et
from multiprocessing import Process
from JSBSim_utils import SandBox, CreateFDM, append_xml

def ScriptExecution(fdm, script_path):
    fdm.load_script(script_path)
    fdm.run_ic()

    while fdm.run():
        if fdm.get_property_value('fcs/left-aileron-pos-rad') != 0.0:
            sys.exit(-1)

sandbox = SandBox()

# First, the execution time of the script c1724.xml is measured. It will be used
# as a reference to check if JSBSim hangs or not.
script_path = sandbox.path_to_jsbsim_file('scripts', 'c1724.xml')
fdm = CreateFDM(sandbox)
start_time = time.time()
ScriptExecution(fdm, script_path)
exec_time = time.time() - start_time

# Since we will alter the aircraft definition file, we need make a copy of it
# and all the files it is refering to.

# Get the aircraft name
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

tree = et.parse(os.path.join(path_to_jsbsim_aircrafts, aircraft_name+'.xml'))
root = tree.getroot()

# The aircraft definition file may also load some data from external files.
# If so, we need to copy these files in our directory build/.../aircraft/aircraft_name
# Only the external files that are in the original directory aircraft/aircraft_name
# will be copied. The files located in 'engine' and 'systems' do not need to be
# copied.
for element in list(root):
    if 'file' in element.keys():
        name = append_xml(element.attrib['file'])
        name_with_path = os.path.join(path_to_jsbsim_aircrafts, name)
        if os.path.exists(name_with_path):
            shutil.copy(name_with_path, aircraft_path)

# Now the copy of the aircraft definition file will be altered: the <rate_limit>
# element is split in two: one with sense 'decr', the other with sense 'incr'.
actuator_element = root.find('flight_control/channel/actuator//rate_limit/..')
rate_element = actuator_element.find('rate_limit')
rate_element.attrib['sense'] = 'decr'
new_rate_element = et.SubElement(actuator_element, 'rate_limit')
new_rate_element.attrib['sense'] = 'incr'
new_rate_element.text = str(float(rate_element.text) * 0.5)

tree.write(sandbox('aircraft', aircraft_name, aircraft_name+'.xml'))

# Run the script with the modified aircraft
fdm = CreateFDM(sandbox)
fdm.set_aircraft_path('aircraft')

# A new process is created that launches the script. We wait for 10 times the
# reference execution time for the script completion. Beyond that time, if the
# process is not completed, it is terminated and the test is failed.
p = Process(target=ScriptExecution, args=(fdm, script_path))
p.start()
p.join(exec_time * 10.0) # Wait 10 times the reference time
if p.is_alive():
    # The process has not yet completed: it means it probably hanged.
    p.terminate()
    sys.exit(-1)

# Second part of the test.
# #######################
#
# The test is run again but this time, <rate_limit> will be read from a
# property instead of being read from a value hard coded in the aircraft
# definition file. It has been reported in the bug 1503 of FlightGear that for
# such a configuration the <actuator> output is constantly increasing even if
# the input is null. For this script the <actuator> output is stored in the
# property fcs/left-aileron-pos-rad. The function ScriptExecution will monitor
# that property and if it changes then the test is failed.

tree = et.parse(os.path.join(path_to_jsbsim_aircrafts, aircraft_name+'.xml'))
flight_control_element = tree.getroot().find('flight_control')
property = et.SubElement(flight_control_element, 'property')
property.text = 'fcs/rate-limit-value'
property.attrib['value'] = rate_element.text
actuator_element = flight_control_element.find('channel/actuator//rate_limit/..')
rate_element = actuator_element.find('rate_limit')
rate_element.attrib['sense'] = 'decr'
rate_element.text = property.text
new_rate_element = et.SubElement(actuator_element, 'rate_limit')
new_rate_element.attrib['sense'] = 'incr'
new_rate_element.text = rate_element.text

tree.write(sandbox('aircraft', aircraft_name, aircraft_name+'.xml'))

fdm = CreateFDM(sandbox)
fdm.set_aircraft_path('aircraft')
ScriptExecution(fdm, script_path)

sandbox.erase()
