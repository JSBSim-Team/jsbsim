# TestInitialConditions.py
#
# A regression test that checks that IC are correctly read from the IC file
# then loaded in the ic/ properties. It also checks that the correct ICs are
# reported in the data written in CSV files.
#
# Copyright (c) 2015 Bertrand Coconnier
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

import os, math, shutil
import xml.etree.ElementTree as et
import pandas as pd
from JSBSim_utils import append_xml, ExecuteUntil, JSBSimTestCase, RunTest

# Values copied from FGJSBBase.cpp and FGXMLElement.cpp
convtoft = {'FT': 1.0, 'M': 3.2808399, 'IN': 1.0/12.0}
convtofps = {'FT/SEC': 1.0, 'KTS': 1.68781}
convtodeg = {'DEG': 1.0, 'RAD': 57.295779513082320876798154814105}
convtokts = {'KTS': 1.0, 'FT/SEC': 1.0/1.68781, 'M/S': 3.2808399/1.68781}


class TestInitialConditions(JSBSimTestCase):
    def getElementTrees(self, s):
        # Read the IC file name from the script
        tree = et.parse(s)
        use_tag = tree.getroot().find('use')

        aircraft_name = use_tag.attrib['aircraft']
        aircraft_path = os.path.join('aircraft', aircraft_name)
        path_to_jsbsim_aircrafts = self.sandbox.path_to_jsbsim_file(aircraft_path)

        IC_file = append_xml(use_tag.attrib['initialize'])
        IC_tree = et.parse(os.path.join(path_to_jsbsim_aircrafts, IC_file))

        return (tree, IC_tree)

    def test_initial_conditions_v1(self):
        prop_output_to_CSV = ['velocities/vc-kts']
        # A dictionary that contains the XML tags to extract from the IC file
        # along with the name of the properties that contain the values
        # extracted from the IC file.
        vars = [{'tag': 'vt', 'unit': convtofps, 'default_unit': 'FT/SEC',
                 'ic_prop': 'ic/vt-fps', 'prop': 'velocities/vt-fps',
                 'CSV_header': 'V_{Total} (ft/s)'},
                {'tag': 'vc', 'unit': convtokts, 'default_unit': 'KTS',
                 'ic_prop': 'ic/vc-kts', 'prop': 'velocities/vc-kts',
                 'CSV_header': '/fdm/jsbsim/velocities/vc-kts'},
                {'tag': 'ubody', 'unit': convtofps, 'default_unit': 'FT/SEC',
                 'ic_prop': 'ic/u-fps', 'prop': 'velocities/u-fps',
                 'CSV_header': 'UBody'},
                {'tag': 'vbody', 'unit': convtofps, 'default_unit': 'FT/SEC',
                 'ic_prop': 'ic/v-fps', 'prop': 'velocities/v-fps',
                 'CSV_header': 'VBody'},
                {'tag': 'wbody', 'unit': convtofps, 'default_unit': 'FT/SEC',
                 'ic_prop': 'ic/w-fps', 'prop': 'velocities/w-fps',
                 'CSV_header': 'WBody'},
                {'tag': 'vnorth', 'unit': convtofps, 'default_unit': 'FT/SEC',
                 'ic_prop': 'ic/vn-fps', 'prop': 'velocities/v-north-fps',
                 'CSV_header': 'V_{North} (ft/s)'},
                {'tag': 'veast', 'unit': convtofps, 'default_unit': 'FT/SEC',
                 'ic_prop': 'ic/ve-fps', 'prop': 'velocities/v-east-fps',
                 'CSV_header': 'V_{East} (ft/s)'},
                {'tag': 'vdown', 'unit': convtofps, 'default_unit': 'FT/SEC',
                 'ic_prop': 'ic/vd-fps', 'prop': 'velocities/v-down-fps',
                 'CSV_header': 'V_{Down} (ft/s)'},
                {'tag': 'latitude', 'unit': convtodeg, 'default_unit': 'RAD',
                 'ic_prop': 'ic/lat-gc-deg', 'prop': 'position/lat-gc-deg',
                 'CSV_header': 'Latitude (deg)'},
                {'tag': 'longitude', 'unit': convtodeg, 'default_unit': 'RAD',
                 'ic_prop': 'ic/long-gc-deg', 'prop': 'position/long-gc-deg',
                 'CSV_header': 'Longitude (deg)'},
                {'tag': 'altitude', 'unit': convtoft, 'default_unit': 'FT',
                 'ic_prop': 'ic/h-agl-ft', 'prop': 'position/h-agl-ft',
                 'CSV_header': 'Altitude AGL (ft)'},
                {'tag': 'altitudeAGL', 'unit': convtoft, 'default_unit': 'FT',
                 'ic_prop': 'ic/h-agl-ft', 'prop': 'position/h-agl-ft',
                 'CSV_header': 'Altitude AGL (ft)'},
                {'tag': 'altitudeMSL', 'unit': convtoft, 'default_unit': 'FT',
                 'ic_prop': 'ic/h-sl-ft', 'prop': 'position/h-sl-ft',
                 'CSV_header': 'Altitude ASL (ft)'},
                {'tag': 'phi', 'unit': convtodeg, 'default_unit': 'RAD',
                 'ic_prop': 'ic/phi-deg', 'prop': 'attitude/phi-deg',
                 'CSV_header': 'Phi (deg)'},
                {'tag': 'theta', 'unit': convtodeg, 'default_unit': 'RAD',
                 'ic_prop': 'ic/theta-deg', 'prop': 'attitude/theta-deg',
                 'CSV_header': 'Theta (deg)'},
                {'tag': 'psi', 'unit': convtodeg, 'default_unit': 'RAD',
                 'ic_prop': 'ic/psi-true-deg', 'prop': 'attitude/psi-deg',
                 'CSV_header': 'Psi (deg)'},
                {'tag': 'elevation', 'unit': convtoft, 'default_unit': 'FT',
                 'ic_prop': 'ic/terrain-elevation-ft',
                 'prop': 'position/terrain-elevation-asl-ft',
                 'CSV_header': 'Terrain Elevation (ft)'}]

        for s in self.script_list(('ZLT-NT-moored-1.xml',
                                   '737_cruise_steady_turn_simplex.xml')):
            (tree, IC_tree) = self.getElementTrees(s)
            IC_root = IC_tree.getroot()

            # Only testing version 1.0 of init files
            if 'version' in IC_root.attrib and float(IC_root.attrib['version']) != 1.0:
                continue

            f, fdm = self.LoadScript(tree, s, prop_output_to_CSV)
            self.CheckICValues(vars, 'script %s' % (f,), fdm, IC_root)

    def LoadScript(self, tree, script_path, prop_output_to_CSV=[]):
        # Make a local copy of files referenced by the script.
        for element in list(tree.getroot()):
            if 'file' in element.keys():
                name = append_xml(element.attrib['file'])
                name_with_path = os.path.join(os.path.dirname(script_path),
                                              name)
                if os.path.exists(name_with_path):
                    shutil.copy(name_with_path, name)

        # Generate a CSV file to check that it is correctly initialized
        # with the initial values
        output_tag = et.SubElement(tree.getroot(), 'output')
        output_tag.attrib['name'] = 'check_csv_values.csv'
        output_tag.attrib['type'] = 'CSV'
        output_tag.attrib['rate'] = '10'
        position_tag = et.SubElement(output_tag, 'position')
        position_tag.text = 'ON'
        velocities_tag = et.SubElement(output_tag, 'velocities')
        velocities_tag.text = 'ON'
        for props in prop_output_to_CSV:
            property_tag = et.SubElement(output_tag, 'property')
            property_tag.text = props
        f = os.path.split(script_path)[-1]  # Script name
        tree.write(f)

        # Initialize the script
        fdm = self.create_fdm()
        self.assertTrue(fdm.load_script(f),
                        msg="Failed to load script %s" % (f,))
        fdm.run_ic()

        return (f, fdm)

    def CheckICValues(self, vars, f, fdm, IC_root):
        # Extract the IC values from XML
        for var in vars:
            var_tag = IC_root.find(var['tag'])
            var['specified'] = var_tag is not None

            if var_tag is None:
                var['value'] = 0.0
                continue

            var['value'] = float(var_tag.text)
            if 'unit' in var_tag.attrib:
                conv = var['unit'][var_tag.attrib['unit']]
            else:
                conv = var['unit'][var['default_unit']]
            var['value'] *= conv

        # Sanity check, we just initialized JSBSim with the ICs, the time must
        # be set to 0.0
        self.assertEqual(fdm['simulation/sim-time-sec'], 0.0)

        # Check that the properties (including in 'ic/') have been correctly
        # initialized (i.e. that they contain the value read from the XML
        # file).
        for var in vars:
            if not var['specified']:
                continue

            if self.PhiOrPsiAndGimbalLock(var, vars):
                continue

            value = var['value']
            prop = fdm[var['ic_prop']]
            if var['tag'] == 'psi':
                if abs(prop - 360.0) <= 1E-8:
                    prop = 0.0
            self.assertAlmostEqual(value, prop, delta=1E-7,
                                   msg="In %s: %s should be %f but found %f" % (f, var['tag'], value, prop))
            prop = fdm[var['prop']]
            if var['tag'] == 'psi':
                if abs(prop - 360.0) <= 1E-8:
                    prop = 0.0
            self.assertAlmostEqual(value, prop, delta=1E-7,
                                   msg="In %s: %s should be %f but found %f" % (f, var['tag'], value, prop))

        # Execute the first second of the script. This is to make sure that the
        # CSV file is open and the ICs have been written in it.
        try:
            ExecuteUntil(fdm, 1.0)
        except RuntimeError as e:
            if e.args[0] == 'Trim Failed':
                self.fail("Trim failed in %s" % (f,))
            else:
                raise

        # Sanity check: make sure that the time step 0.0 has been copied in the
        # CSV file.
        ref = pd.read_csv('check_csv_values.csv')
        self.assertEqual(ref['Time'][0], 0.0)

        # Check that the value in the CSV file equals the value read from the
        # IC file.
        for var in vars:
            if not var['specified']:
                continue

            if self.PhiOrPsiAndGimbalLock(var, vars):
                continue

            value = var['value']
            csv_value = float(ref[var['CSV_header']][0])
            if var['tag'] == 'psi':
                if abs(csv_value - 360.0) <= 1E-8:
                    csv_value = 0.0
            self.assertAlmostEqual(value, csv_value, delta=1E-7,
                                   msg="In {}: {} should be {} but found {}".format(f, var['tag'], value, csv_value))

    def GetVariables(self, lat_tag):
        vars = [{'tag': 'longitude', 'unit': convtodeg, 'default_unit': 'RAD',
                 'ic_prop': 'ic/long-gc-deg', 'prop': 'position/long-gc-deg',
                 'CSV_header': 'Longitude (deg)'},
                {'tag': 'altitudeAGL', 'unit': convtoft, 'default_unit': 'FT',
                 'ic_prop': 'ic/h-agl-ft', 'prop': 'position/h-agl-ft',
                 'CSV_header': 'Altitude AGL (ft)'},
                {'tag': 'altitudeMSL', 'unit': convtoft, 'default_unit': 'FT',
                 'ic_prop': 'ic/h-sl-ft', 'prop': 'position/h-sl-ft',
                 'CSV_header': 'Altitude ASL (ft)'}]

        if lat_tag is None:
            lat_vars = []
        elif 'type' not in lat_tag.attrib or lat_tag.attrib['type'][:4] != "geod":
            lat_vars = [{'tag': 'latitude', 'unit': convtodeg,
                         'default_unit': 'RAD', 'ic_prop': 'ic/lat-gc-deg',
                         'prop': 'position/lat-gc-deg',
                         'CSV_header': 'Latitude (deg)'}]
        else:
            lat_vars = [{'tag': 'latitude', 'unit': convtodeg,
                         'default_unit': 'RAD', 'ic_prop': 'ic/lat-geod-deg',
                         'prop': 'position/lat-geod-deg',
                         'CSV_header': 'Latitude Geodetic (deg)'}]

        return lat_vars+vars

    def test_geod_position_from_init_file_v2(self):
        for s in self.script_list(('ZLT-NT-moored-1.xml',
                                   '737_cruise_steady_turn_simplex.xml')):
            (tree, IC_tree) = self.getElementTrees(s)
            IC_root = IC_tree.getroot()

            # Only testing version 2.0 of init files
            if ('version' not in IC_root.attrib or float(IC_root.attrib['version']) != 2.0):
                continue

            position_tag = IC_root.find('position')
            lat_tag = position_tag.find('latitude')

            f, fdm = self.LoadScript(tree, s)
            self.CheckICValues(self.GetVariables(lat_tag), 'script %s' % (f,),
                               fdm, position_tag)

    def test_initial_latitude(self):
        Output_file = self.sandbox.path_to_jsbsim_file('tests', 'output.xml')
        GEODETIC, ELEVATION, ALTITUDE = (1, 2, 4)

        for v in ('', '_v2'):
            IC_file = self.sandbox.path_to_jsbsim_file('aircraft', 'ball',
                                                       'reset00'+v+'.xml')

            for i in range(8):
                for latitude_pos in range(4):
                    IC_tree = et.parse(IC_file)
                    IC_root = IC_tree.getroot()
                    if v:
                        position_tag = IC_root.find('position')
                        latitude_tag = et.SubElement(position_tag, 'latitude')
                        latitude_tag.attrib['unit'] = 'DEG'
                    else:
                        position_tag = IC_root
                        latitude_tag = IC_root.find('latitude')

                    latitude_tag.text = str(latitude_pos*30.)

                    if i & GEODETIC:
                        latitude_tag.attrib['type'] = 'geod'

                    if i & ELEVATION:
                        elevation_tag = et.SubElement(IC_root, 'elevation')
                        elevation_tag.text = '1000.'

                    if i & ALTITUDE:
                        if v:
                            altitude_tag = position_tag.find('altitudeMSL')
                            altitude_tag.tag = 'altitudeAGL'
                        else:
                            altitude_tag = position_tag.find('altitude')
                            altitude_tag.tag = 'altitudeMSL'

                    IC_tree.write('IC.xml')

                    fdm = self.create_fdm()
                    fdm.load_model('ball')
                    fdm.set_output_directive(Output_file)
                    fdm.set_output_filename(1, 'check_csv_values.csv')
                    fdm.load_ic('IC.xml', False)
                    fdm.run_ic()

                    self.CheckICValues(self.GetVariables(latitude_tag),
                                       'IC%d' % (i,), fdm, position_tag)

    def test_set_initial_geodetic_latitude(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       '737_cruise.xml')
        output_file = self.sandbox.path_to_jsbsim_file('tests', 'output.xml')
        fdm = self.create_fdm()
        fdm.load_script(script_path)
        fdm.set_output_directive(output_file)

        alt = fdm['ic/h-agl-ft']
        glat = fdm['ic/lat-geod-deg'] - 30.
        fdm['ic/lat-geod-deg'] = glat
        fdm.run_ic()

        self.assertAlmostEqual(fdm['ic/h-agl-ft'], alt)
        self.assertAlmostEqual(fdm['ic/lat-geod-deg'], glat)
        self.assertAlmostEqual(fdm['ic/lat-geod-rad'], glat*math.pi/180.)
        self.assertAlmostEqual(fdm['position/lat-geod-deg'], glat)

        # Sanity check: make sure that the time step 0.0 has been copied in the
        # CSV file.
        ref = pd.read_csv('output.csv')
        self.assertEqual(ref['Time'][0], 0.0)
        self.assertAlmostEqual(ref['Latitude Geodetic (deg)'][0], glat)

    def PhiOrPsiAndGimbalLock(self, var, vars):
        # Look out for quaternion singularity/gimbal-lock when ic/theta == 90
        # and avoid checking psi and phi in this case
        if ((var['tag'] == 'psi' or var['tag'] == 'phi') and 
            [x for x in vars if x['tag'] == 'theta' and abs(x['value'] - 90.0) <= 1E-8]):
            return True
        return False

RunTest(TestInitialConditions)
