# ResetOutputFiles.py
#
# A regression test on the interactions between the output file name changes and
# the reset procedure.
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
# Regression test that checks that output file names modifications made through
# FGFMExec::SetOutputFileName() are properly handled.
#
# Rules that are checked :
#  1. FGFDMExec::SetOutputFileName() change the name to what it has been
#     instructed to use (that's a minimum !)
#  2. Multiple calls to FGFDMExec::SetOutputFileName() are properly handled e.g.
#     the name used is the last one sent via the method.
#  3. If a call to FGFDMExec::SetOutputFileName() is made *after* the simulation
#     has been initialized (i.e. after FGModel::InitModel() is called) then it
#     is ignored until the next call to FGMFDMExec::InitModel().

import os, sys
import jsbsim

def delete_csv_files():
    files = os.listdir(".")
    for f in files:
        if f[-4:] == '.csv':
            os.remove(f)

path_to_jsbsim_files = os.path.relpath(sys.argv[1], os.getcwd())

delete_csv_files()

def InitFDM():
    _fdm = jsbsim.FGFDMExec(root_dir=os.path.join('.', ''))
    _fdm.set_aircraft_path(os.path.join(path_to_jsbsim_files, 'aircraft'))
    _fdm.set_engine_path(os.path.join(path_to_jsbsim_files, 'engine'))
    _fdm.set_systems_path(os.path.join(path_to_jsbsim_files, 'systems'))
    return _fdm

def ExecuteUntil(_fdm, end_time):
    while _fdm.run():
        if _fdm.get_sim_time() > end_time:
            return

#
# Regular run that checks the correct CSV file is created
# We are just checking its existence, not its content. To accelerate the test
# execution, the simulation is interrupted after 1.0sec of simulated time.
#
fdm = InitFDM()
fdm.load_script(os.path.join(path_to_jsbsim_files, 'scripts', 'c1722.xml'))

fdm.run_ic()
ExecuteUntil(fdm, 1.0)

if (not os.path.exists('JSBout172B.csv')):
  sys.exit(-1) # 'make test' will report the test failed.

#
# Reset the simulation and check that iteration number is correctly appended to
# the filename.
#
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if (not os.path.exists('JSBout172B_0.csv')):
  sys.exit(-1) # 'make test' will report the test failed.

#
# Change the output filename and check that the naming logic is reset (e.g. that
# no iteration number is appended to the filename
#
fdm.set_output_filename(0,'dummy.csv')
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if (not os.path.exists('dummy.csv')):
  sys.exit(-1) # 'make test' will report the test failed.

#
# Call FGFDMExec::SetOutputFileName() after the simulation is reset. And verify
# that the new output file name is ignored until the next call to
# FGOutput::SetStartNewOutput(). This should be so according to the
# documentation of FGOutput::SetOutputName().
#
fdm.reset_to_initial_conditions(1)
fdm.set_output_filename(0,'dummyx.csv')
ExecuteUntil(fdm, 1.0)

if (os.path.exists('dummyx.csv') or not os.path.exists('dummy_0.csv')):
  sys.exit(-1) # 'make test' will report the test failed.

#
# Check that the new filename is taken into account when the simulation is reset
#
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if (not os.path.exists('dummyx.csv')):
  sys.exit(-1) # 'make test' will report the test failed.

#
# Check against multiple calls to FGFDMExec::SetOutputFileName()
#
fdm.set_output_filename(0,'thisone.csv')
fdm.set_output_filename(0,'thatone.csv')
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if (os.path.exists('thisone.csv') or not os.path.exists('thatone.csv')):
  sys.exit(-1) # 'make test' will report the test failed.

#
# Check again on a brand new FDM
#
delete_csv_files()
fdm = InitFDM()
fdm.load_script(os.path.join(path_to_jsbsim_files,'scripts', 'c1722.xml'))

fdm.run_ic()
fdm.set_output_filename(0,'oops.csv') # Oops!! Changed my mind
ExecuteUntil(fdm, 1.0)

if (os.path.exists('oops.csv') or not os.path.exists('JSBout172B.csv')):
  sys.exit(-1) # 'make test' will report the test failed.

#
# The new file name 'oops.csv' has been ignored.
# Check if it is now taken into account.
#
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if not os.path.exists('oops.csv'):
  sys.exit(-1) # 'make test' will report the test failed.

sys.exit(0) # Needed for 'make test' to report the test passed.
