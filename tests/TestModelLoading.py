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

import os
import xml.etree.ElementTree as et
import pandas as pd
from JSBSim_utils import (JSBSimTestCase, CreateFDM, ExecuteUntil, append_xml,
                          CopyAircraftDef, isDataMatching, FindDifferences,
                          RunTest)


class TestModelLoading(JSBSimTestCase):
    def BuildReference(self, script_name):
        # Run the script
        self.script = self.sandbox.path_to_jsbsim_file('scripts', script_name)
        self.sandbox.delete_csv_files()
        fdm = CreateFDM(self.sandbox)
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests',
                                                                  'output.xml'))
        fdm.load_script(self.script)
        fdm['simulation/randomseed'] = 0.0

        fdm.run_ic()
        ExecuteUntil(fdm, 50.0)

        self.ref = pd.read_csv("output.csv", index_col=0)

        # Since the script will work with modified versions of the aircraft XML
        # definition file, we need to make a copy of the directory that
        # contains all the input data of that aircraft

        tree, self.aircraft_name, self.path_to_jsbsim_aircrafts = CopyAircraftDef(self.script, self.sandbox)
        self.aircraft_path = os.path.join('aircraft', self.aircraft_name)

    def ProcessAndCompare(self, section):
        # Here we determine if the original aircraft definition <section> is
        # inline or read from an external file.
        tree = et.parse(os.path.join(self.path_to_jsbsim_aircrafts,
                                     self.aircraft_name + '.xml'))
        root = tree.getroot()

        # Iterate over all the tags named <section>
        for section_element in root.findall(section):
            if 'file' in section_element.keys():
                self.InsertAndCompare(section_element, tree)
            else:
                self.DetachAndCompare(section_element, tree)

    def DetachAndCompare(self, section_element, tree):
        # Extract <section> from the original aircraft definition file and copy
        # it in a separate XML file 'section.xml'
        section_tree = et.ElementTree(element=section_element)
        if 'name' in section_element.keys():
            section = section_element.attrib['name']
        else:
            section = section_element.tag

        section_tree.write(os.path.join(self.aircraft_path, section+'.xml'),
                           xml_declaration=True)

        # Now, we need to clean up the aircraft definition file from all
        # references to <section>. We just need a single <section> tag that
        # points to the file 'section.xml'
        for element in list(section_element):
            section_element.remove(element)

        section_element.attrib = {'file': section+'.xml'}
        tree.write(os.path.join(self.aircraft_path, self.aircraft_name+'.xml'),
                   xml_declaration=True)

        self.Compare(section)

    def InsertAndCompare(self, section_element, tree):
        file_name = append_xml(section_element.attrib['file'])
        section_file = os.path.join(self.path_to_jsbsim_aircrafts, file_name)

        # If <section> is actually <system>, we need to iterate over all the
        # directories in which the file is allowed to be stored until the file
        # is located.
        if not os.path.exists(section_file) and section_element.tag == 'system':
            section_file = os.path.join(self.path_to_jsbsim_aircrafts,
                                        "systems", file_name)
            if not os.path.exists(section_file):
                section_file = self.sandbox.path_to_jsbsim_file("systems",
                                                                file_name)

        # The original <section> tag is dropped and replaced by the content of
        # the file.
        section_root = et.parse(section_file).getroot()

        del section_element.attrib['file']
        section_element.attrib.update(section_root.attrib)
        section_element.extend(section_root)

        tree.write(os.path.join(self.aircraft_path, self.aircraft_name+'.xml'))

        self.Compare(section_element.tag+" file:"+section_file)

    def Compare(self, section):
        # Rerun the script with the modified aircraft definition
        self.sandbox.delete_csv_files()
        fdm = CreateFDM(self.sandbox)
        # We need to tell JSBSim that the aircraft definition is located in the
        # directory build/.../aircraft
        fdm.set_aircraft_path('aircraft')
        fdm.set_output_directive(self.sandbox.path_to_jsbsim_file('tests',
                                                                  'output.xml'))
        fdm.load_script(self.script)
        fdm['simulation/randomseed'] = 0.0

        fdm.run_ic()
        ExecuteUntil(fdm, 50.0)

        mod = pd.read_csv('output.csv', index_col=0)

        # Check the data are matching i.e. the time steps are the same between
        # the two data sets and that the output data are also the same.
        self.assertTrue(isDataMatching(self.ref, mod))

        # Whether the data is read from the aircraft definition file or from an
        # external file, the results shall be exactly identical. Hence the
        # precision set to 0.0.
        diff = FindDifferences(self.ref, mod, 0.0)
        self.assertEqual(len(diff), 0,
                         msg='\nTesting section "'+section+'"\n'+diff.to_string())

    def test_model_loading(self):
        self.longMessage = True

        self.BuildReference('c1724.xml')
        output_ref = pd.read_csv('JSBout172B.csv', index_col=0)

        self.ProcessAndCompare('aerodynamics')
        self.ProcessAndCompare('autopilot')
        self.ProcessAndCompare('flight_control')
        self.ProcessAndCompare('ground_reactions')
        self.ProcessAndCompare('mass_balance')
        self.ProcessAndCompare('metrics')
        self.ProcessAndCompare('propulsion')
        self.ProcessAndCompare('system')

        # The <output> section needs special handling. In addition to the check
        # conducted by ProcessAndCompare with a directive file, we need to
        # verify that the <output> tag has been correctly executed by JSBSim.
        # In the case of the script c1724.xml, this means that the data output
        # in JSBout172B.csv is the same between the reference 'output_ref' and
        # the result 'mod' below where the <output> tag was moved in a separate
        # file.
        self.ProcessAndCompare('output')
        mod = pd.read_csv('JSBout172B.csv', index_col=0)
        self.assertTrue(isDataMatching(output_ref, mod))
        diff = FindDifferences(output_ref, mod, 0.0)
        self.assertEqual(len(diff), 0,
                         msg='\nTesting section "output"\n'+diff.to_string())

        self.BuildReference('weather-balloon.xml')
        self.ProcessAndCompare('buoyant_forces')

        self.BuildReference('Concorde_runway_test.xml')
        self.ProcessAndCompare('external_reactions')

RunTest(TestModelLoading)
