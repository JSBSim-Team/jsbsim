import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest, CopyAircraftDef


import jsbsim


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

        linearization = jsbsim.FGLinearization(fdm)

        print(linearization.x0.shape, (12,))
        print(linearization.u0.shape, (4,))
        print(linearization.y0.shape, (12,))
        print(linearization.state_space[0].shape, (12, 12))
        print(linearization.state_space[1].shape, (12, 4))
        print(linearization.state_space[2].shape, (12, 12))
        print(linearization.state_space[3].shape, (12, 4))

        self.assertEqual(linearization.x0.shape, (12,))
        self.assertEqual(linearization.u0.shape, (4,))
        self.assertEqual(linearization.y0.shape, (12,))
        self.assertEqual(linearization.state_space[0].shape, (12, 12))
        self.assertEqual(linearization.state_space[1].shape, (12, 4))
        self.assertEqual(linearization.state_space[2].shape, (12, 12))
        self.assertEqual(linearization.state_space[3].shape, (12, 4))

        self.assertEqual(linearization.x_names, ('Vt', 'Alpha', 'Theta', 'Q', 'Beta', 'Phi', 'P', 'Psi',
                                                 'R', 'Latitude', 'Longitude', 'Alt'))
        self.assertEqual(linearization.u_names, ('ThtlCmd', 'DaCmd', 'DeCmd', 'DrCmd'))
        self.assertEqual(linearization.y_names, ('Vt', 'Alpha', 'Theta', 'Q', 'Beta', 'Phi', 'P', 'Psi',
                                                 'R', 'Latitude', 'Longitude', 'Alt'))
        self.assertEqual(linearization.x_units, ('ft/s', 'rad', 'rad', 'rad/s', 'rad', 'rad', 'rad/s',
                                                 'rad', 'rad/s', 'rad', 'rad', 'ft'))
        self.assertEqual(linearization.u_units, ('norm', 'norm', 'norm', 'norm'))
        self.assertEqual(linearization.y_units, ('ft/s', 'rad', 'rad', 'rad/s', 'rad', 'rad', 'rad/s',
                                                 'rad', 'rad/s', 'rad', 'rad', 'ft'))


RunTest(TestLinearization)
