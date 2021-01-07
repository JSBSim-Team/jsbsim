import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest, CopyAircraftDef


class TestLinearization(JSBSimTestCase):
    def test_do_linearization(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       '737_cruise.xml')
        aircraft_tree, aircraft_name, b = CopyAircraftDef(script_path,
                                                          self.sandbox)
        aircraft_tree.write(self.sandbox('aircraft', aircraft_name,
                                         aircraft_name+'.xml'))

        IC_file = self.sandbox('aircraft', aircraft_name, 'cruise_init.xml')
        tree = et.parse(IC_file)
        heading_el = tree.find('psi')
        heading_el.text = '270.0'
        tree.write(IC_file)

        fdm = self.create_fdm()
        fdm.set_aircraft_path(self.sandbox('aircraft'))
        fdm.load_script(script_path)
        fdm.run_ic()

        x0, u0, y0, A, B, C, D, \
        state_names, input_names, output_name, state_units, input_units, output_units = fdm.do_linearization()

        self.assertEqual(x0.shape, (12,))
        self.assertEqual(u0.shape, (4,))
        self.assertEqual(y0.shape, (12,))
        self.assertEqual(A.shape, (12, 12))
        self.assertEqual(B.shape, (12, 4))
        self.assertEqual(C.shape, (12, 12))
        self.assertEqual(D.shape, (12, 4))

        self.assertEqual(state_names, ('Vt', 'Alpha', 'Theta', 'Q', 'Beta', 'Phi', 'P', 'Psi',
                                       'R', 'Latitude', 'Longitude', 'Alt'))
        self.assertEqual(input_names, ('ThtlCmd', 'DaCmd', 'DeCmd', 'DrCmd'))
        self.assertEqual(output_name, ('Vt', 'Alpha', 'Theta', 'Q', 'Beta', 'Phi', 'P', 'Psi',
                                       'R', 'Latitude', 'Longitude', 'Alt'))
        self.assertEqual(state_units, ('ft/s', 'rad', 'rad', 'rad/s', 'rad', 'rad', 'rad/s',
                                       'rad', 'rad/s', 'rad', 'rad', 'ft'))
        self.assertEqual(input_units, ('norm', 'norm', 'norm', 'norm'))
        self.assertEqual(output_units, ('ft/s', 'rad', 'rad', 'rad/s', 'rad', 'rad', 'rad/s',
                                        'rad', 'rad/s', 'rad', 'rad', 'ft'))


RunTest(TestLinearization)
