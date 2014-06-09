# TestModelLoading.py
#
# A regression test that checks if the model inclusion with the attribute
# 'file=' is working.
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

import os, sys, shutil
import xml.etree.ElementTree as et
from JSBSim_utils import Table, CreateFDM, ExecuteUntil, SandBox

def append_xml(name):
    if len(name) < 4 or name[-4:] != '.xml':
        return name+'.xml'
    return name

def BuildReference(sandbox, script_name):
    aircraft = {}

    # Run the script
    script_full_path = sandbox.path_to_jsbsim_file(os.path.join('scripts', script_name))
    aircraft['script'] = script_full_path

    sandbox.delete_csv_files()
    fdm = CreateFDM(sandbox)
    fdm.set_output_directive(sandbox.path_to_jsbsim_file('tests', 'output.xml'))
    fdm.load_script(script_full_path)

    fdm.run_ic()
    ExecuteUntil(fdm, 50.0)

    ref = Table()
    ref.ReadCSV(sandbox("output.csv"))
    aircraft['CSV'] = ref

    # Since the script will work with modified versions of the aircraft XML
    # definition file, we need to make a copy of the directory that contains all
    # the input data of that aircraft

    # First, extract the aircraft name from the script
    tree = et.parse(sandbox.elude(script_full_path))
    use_element = tree.getroot().find('use')
    aircraft_name = use_element.attrib['aircraft']

    # Then, create a directory aircraft/aircraft_name in the build directory
    aircraft_path = os.path.join('aircraft', aircraft_name)
    path_to_jsbsim_aircrafts = sandbox.elude(sandbox.path_to_jsbsim_file(aircraft_path))
    aircraft_path = sandbox(aircraft_path)
    if not os.path.exists(aircraft_path):
        os.makedirs(aircraft_path)

    aircraft['JSBSim path'] = path_to_jsbsim_aircrafts
    aircraft['name'] = aircraft_name
    aircraft['path'] = aircraft_path

    # Make a copy of the initialization file in build/.../aircraft/aircraft_name
    IC_file = append_xml(use_element.attrib['initialize'])
    shutil.copy(os.path.join(path_to_jsbsim_aircrafts, IC_file), aircraft_path)

    tree = et.parse(os.path.join(path_to_jsbsim_aircrafts, aircraft_name+'.xml'))

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

    return aircraft

def ProcessAndCompare(sandbox, aircraft, section):
    # Here we determine if the original aircraft definition <section> is inline
    # or read from an external file.
    tree = et.parse(os.path.join(aircraft['JSBSim path'],
                                 aircraft['name']+'.xml'))
    root = tree.getroot()

    # Iterate over all the tags named <section>
    for section_element in root.findall(section):
        if 'file' in section_element.keys():
            InsertAndCompare(sandbox, aircraft, section_element, tree)
        else:
            DetachAndCompare(sandbox, aircraft, section_element, tree)

def DetachAndCompare(sandbox, aircraft, section_element, tree):
    aircraft_path = aircraft['path']

    # Extract <section> from the original aircraft definition file and copy it
    # in a separate XML file 'section.xml'
    section_tree = et.ElementTree(element=section_element)
    if 'name' in section_element.keys():
        section = section_element.attrib['name']
    else:
        section = section_element.tag

    section_tree.write(os.path.join(aircraft_path, section+'.xml'),
                       xml_declaration=True)

    # Now, we need to clean up the aircraft definition file from all references
    # to <section>. We just need a single <section> tag that points to the file
    # 'section.xml'
    for element in list(section_element):
        section_element.remove(element)

    section_element.attrib = {'file': section+'.xml'}
    tree.write(os.path.join(aircraft_path, aircraft['name']+'.xml'),
               xml_declaration=True)

    Compare(sandbox, aircraft)

def InsertAndCompare(sandbox, aircraft, section_element, tree):
    path_to_jsbsim_aircrafts = aircraft['JSBSim path']
    file_name = append_xml(section_element.attrib['file'])
    section_file = os.path.join(path_to_jsbsim_aircrafts, file_name)

    # If <section> is actually <system>, we need to iterate over all the
    # directories in which the file is allowed to be stored until the file is
    # located.
    if not os.path.exists(section_file) and section_element.tag == 'system':
        section_file = os.path.join(path_to_jsbsim_aircrafts, "Systems", file_name)
        if not os.path.exists(section_file):
            section_file = sandbox.elude(sandbox.path_to_jsbsim_file("systems", file_name))

    # The original <section> tag is dropped and replaced by the content of the
    # file.
    section_root = et.parse(section_file).getroot()
    root = tree.getroot()
    root.remove(section_element)
    root.append(section_root)
    tree.write(os.path.join(aircraft['path'], aircraft['name']+'.xml'))

    Compare(sandbox, aircraft)

def Compare(sandbox, aircraft):
    # Rerun the script with the modified aircraft definition
    sandbox.delete_csv_files()
    fdm = CreateFDM(sandbox)
    # We need to tell JSBSim that the aircraft definition is located in the
    # directory build/.../aircraft
    fdm.set_aircraft_path('aircraft')
    fdm.set_output_directive(sandbox.path_to_jsbsim_file('tests', 'output.xml'))
    fdm.load_script(aircraft['script'])

    fdm.run_ic()
    ExecuteUntil(fdm, 50.0)

    mod = Table()
    mod.ReadCSV(sandbox('output.csv'))

    # Whether the data is read from the aircraft definition file or from an
    # external file, the results shall be exactly identical. Hence the precision
    # set to 0.0.
    diff = aircraft['CSV'].compare(mod, 0.0)
    if not diff.empty:
        print diff
        sys.exit(-1)

sandbox = SandBox()

aircraft = BuildReference(sandbox, 'c1724.xml')
ProcessAndCompare(sandbox, aircraft, 'aerodynamics')
ProcessAndCompare(sandbox, aircraft, 'autopilot')
ProcessAndCompare(sandbox, aircraft, 'flight_control')
ProcessAndCompare(sandbox, aircraft, 'ground_reactions')
ProcessAndCompare(sandbox, aircraft, 'mass_balance')
ProcessAndCompare(sandbox, aircraft, 'metrics')
ProcessAndCompare(sandbox, aircraft, 'propulsion')
ProcessAndCompare(sandbox, aircraft, 'system')

aircraft = BuildReference(sandbox, 'weather-balloon.xml')
ProcessAndCompare(sandbox, aircraft, 'buoyant_forces')

aircraft = BuildReference(sandbox, 'Concorde_runway_test.xml')
ProcessAndCompare(sandbox, aircraft, 'external_reactions')

sandbox.erase()
