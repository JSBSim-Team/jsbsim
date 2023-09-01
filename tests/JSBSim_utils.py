# JSBSim_utils.py
#
# Some utilities to help developing Python scripts with JSBSim.
#
# Copyright (c) 2014-2020 Bertrand Coconnier
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

import os, sys, tempfile, shutil, unittest, functools
import xml.etree.ElementTree as et
import numpy as np
import pandas as pd

sys.path.append(os.getcwd())

import jsbsim


# Hides startup and debug messages
jsbsim.FGJSBBase().debug_lvl = 0


class SandBox:
    def __init__(self, *args):
        self._tmpdir = tempfile.mkdtemp(dir=os.getcwd())
        path_to_jsbsim = os.path.join(os.path.dirname(sys.argv[0]), '..', *args)
        self._relpath_to_jsbsim = os.path.relpath(path_to_jsbsim, self._tmpdir)

    def __call__(self, *args):
        return os.path.relpath(os.path.join(self._tmpdir, *args), os.getcwd())

    def delete_csv_files(self):
        files = os.listdir(self._tmpdir)
        for f in files:
            if f[-4:] == '.csv':
                os.remove(os.path.join(self._tmpdir, f))

    def path_to_jsbsim_file(self, *args):
        return os.path.join(self._relpath_to_jsbsim, *args)

    def exists(self, filename):
        return os.path.exists(self(filename))

    def erase(self):
        shutil.rmtree(self._tmpdir)


def CreateFDM(sandbox, pm=None):
    _fdm = jsbsim.FGFDMExec(os.path.join(sandbox(), ''), pm)
    path = sandbox.path_to_jsbsim_file()
    _fdm.set_aircraft_path(os.path.join(path, 'aircraft'))
    _fdm.set_engine_path(os.path.join(path, 'engine'))
    _fdm.set_systems_path(os.path.join(path, 'systems'))
    return _fdm


def ExecuteUntil(_fdm, end_time):
    while _fdm.run():
        if _fdm.get_sim_time() > end_time:
            return


def append_xml(name):
    if len(name) < 4 or name[-4:] != '.xml':
        return name+'.xml'
    return name


def CheckXMLFile(f, header):
    # Is f a file ?
    if not os.path.isfile(f):
        return False

    # Is f an XML file ?
    try:
        tree = et.parse(f)
    except et.ParseError:
        return False

    # Check the file header
    return tree.getroot().tag.upper() == header.upper()


def CopyAircraftDef(script_path, sandbox):
    # Get the aircraft name
    tree = et.parse(script_path)
    use_element = tree.getroot().find('use')
    aircraft_name = use_element.attrib['aircraft']

    # Then, create a directory aircraft/aircraft_name in the build directory
    aircraft_path = os.path.join('aircraft', aircraft_name)
    path_to_jsbsim_aircrafts = sandbox.path_to_jsbsim_file(aircraft_path)
    aircraft_path = sandbox(aircraft_path)
    if not os.path.exists(aircraft_path):
        os.makedirs(aircraft_path)

    # Make a copy of the initialization file in
    # build/.../aircraft/aircraft_name
    IC_file = append_xml(use_element.attrib['initialize'])
    shutil.copy(os.path.join(path_to_jsbsim_aircrafts, IC_file), aircraft_path)

    tree = et.parse(os.path.join(path_to_jsbsim_aircrafts,
                                 aircraft_name+'.xml'))

    # The aircraft definition file may also load some data from external files.
    # If so, we need to copy these files in our directory
    # build/.../aircraft/aircraft_name Only the external files that are in the
    # original directory aircraft/aircraft_name will be copied. The files
    # located in 'engine' and 'systems' do not need to be copied.
    for element in list(tree.getroot()):
        if 'file' in element.keys():
            name = append_xml(element.attrib['file'])
            name_with_path = os.path.join(path_to_jsbsim_aircrafts, name)
            subdirs = os.path.split(name)[0]
            if os.path.exists(name_with_path):
                shutil.copy(name_with_path, os.path.join(aircraft_path,
                                                         subdirs))
            else:
                name_with_system_path = os.path.join(path_to_jsbsim_aircrafts,
                                                     'Systems', name)

                if os.path.exists(name_with_system_path):
                    system_path = sandbox(aircraft_path, 'Systems')
                    if not os.path.exists(system_path):
                        os.makedirs(system_path)
                    shutil.copy(name_with_system_path,
                                os.path.join(system_path, subdirs))

    return tree, aircraft_name, path_to_jsbsim_aircrafts


class JSBSimTestCase(unittest.TestCase):
    def __init__(self, methodName):
        unittest.TestCase.__init__(self, methodName)
        self._fdm = None

    def setUp(self, *args):
        self.sandbox = SandBox(*args)
        self.currentdir = os.getcwd()
        os.chdir(self.sandbox())

    def tearDown(self):
        self.delete_fdm()
        os.chdir(self.currentdir)
        self.sandbox.erase()

    # Generator that returns the full path to all the scripts in JSBSim
    def script_list(self, blacklist=[]):
        script_path = self.sandbox.path_to_jsbsim_file('scripts')
        for f in os.listdir(script_path):
            if f in blacklist:
                continue

            fullpath = os.path.join(script_path, f)

            # Does f contain a JSBSim script ?
            if CheckXMLFile(fullpath, 'runscript'):
                yield fullpath

    def create_fdm(self, pm=None):
        self._fdm = CreateFDM(self.sandbox, pm)
        return self._fdm

    def delete_fdm(self):
        self._fdm = None

    def load_script(self, script_name):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       append_xml(script_name))
        self._fdm.load_script(script_path)

    def get_aircraft_xml_tree(self, script_name):
        script_path = self.sandbox.path_to_jsbsim_file('scripts',
                                                       append_xml(script_name))
        tree = et.parse(script_path)
        use_element = tree.getroot().find('use')
        aircraft_name = use_element.attrib['aircraft']

        aircraft_path = self.sandbox.path_to_jsbsim_file('aircraft', aircraft_name,
                                                         aircraft_name+'.xml')
        return et.parse(aircraft_path)


def spare(filename):
    # Decorator to spare a file from the deletion of the sandbox temporary
    # directory
    def decorated(func):
        @functools.wraps(func)
        def wrapper(self):
            try:
                response = func(self)
            finally:
                shutil.copy(self.sandbox(filename),
                            os.path.join(self.currentdir,
                                         os.path.split(filename)[-1]))
            return response
        return wrapper
    return decorated


class FlightModel:
    def __init__(self, test_case, name):
        self.path = test_case.sandbox.path_to_jsbsim_file('tests')
        self.fdm = test_case.create_fdm()
        self.fdm.set_aircraft_path('.')

        self.name = name
        self.tree = et.parse(os.path.join(self.path, name+'.xml'))
        self.root = self.tree.getroot()

    def include_system_test_file(self, file_name):
        self.fdm.set_systems_path(self.path)
        system_tag = et.SubElement(self.root, 'system')
        system_tag.attrib['file'] = file_name

    def include_planet_test_file(self, file_name):
        system_tag = et.SubElement(self.root, 'planet')
        system_tag.attrib['file'] = file_name

    def before_loading(self):
        pass

    def before_ic(self):
        pass

    def start(self):
        self.tree.write(self.name+'.xml')

        self.before_loading()
        self.fdm.load_model(self.name, False)

        self.before_ic()
        self.fdm.run_ic()

        return self.fdm


def RunTest(test):
    suite = unittest.TestLoader().loadTestsFromTestCase(test)
    test_result = unittest.TextTestRunner(verbosity=2).run(suite)
    if test_result.failures or test_result.errors:
        sys.exit(-1)  # 'make test' will report the test failed.


def isDataMatching(ref, other):
    delta = np.abs(ref - other)
    # Check the data are matching i.e. the time steps are the same between the
    # two data sets and that their layouts are also the same (same number of
    # lines & columns). If they are not, pandas will fill the missing data with
    # NaNs. The check consists therefore in checking there are no NaNs.
    return delta.notnull().any().any()


def FindDifferences(ref, other, tol):
    delta = np.abs(ref - other)

    idxmax = delta.idxmax()
    ref_max = pd.Series(np.diag(ref.loc[idxmax]), index=ref.columns)
    other_max = pd.Series(np.diag(other.loc[idxmax]), index=other.columns)
    diff = pd.concat([idxmax, delta.max(), ref_max, other_max], axis=1)
    diff.columns = ['Time', 'delta', 'ref value', 'value']
    return diff[diff['delta'] > tol]
