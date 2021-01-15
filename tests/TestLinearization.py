import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest, CopyAircraftDef
import jsbsim


class TestLinearization(JSBSimTestCase):
    def test_do_linearization(self):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       '737_cruise.xml')

        fdm = self.create_fdm()
        fdm.load_script(script_path)
        fdm.run_ic()

        # Set engines running
        fdm['propulsion/engine[0]/set-running'] = 1
        fdm['propulsion/engine[1]/set-running'] = 1
        fdm.run()

        # Trim
        fdm.debug_lvl = 1 # Enable debug messages to log trimmed values
        fdm['simulation/do_simple_trim'] = 1
        fdm.debug_lvl = 0 # Disable debug messages

        linearization = jsbsim.FGLinearization(fdm)

        self.assertEqual(linearization.x0.shape, (12,))
        self.assertEqual(linearization.u0.shape, (4,))
        self.assertEqual(linearization.y0.shape, (12,))
        self.assertEqual(len(linearization.state_space), 4)
        self.assertEqual(linearization.state_space[0].shape, (12, 12))
        self.assertEqual(linearization.state_space[1].shape, (12, 4))
        self.assertEqual(linearization.state_space[2].shape, (12, 12))
        self.assertEqual(linearization.state_space[3].shape, (12, 4))
        self.assertEqual(linearization.system_matrix.shape, (12, 12))
        self.assertEqual(linearization.input_matrix.shape, (12, 4))
        self.assertEqual(linearization.output_matrix.shape, (12, 12))
        self.assertEqual(linearization.feedforward_matrix.shape, (12, 4))
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
