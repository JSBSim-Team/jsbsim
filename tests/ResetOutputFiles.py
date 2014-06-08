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

import sys
from JSBSim_utils import CreateFDM, ExecuteUntil, SandBox

sandbox = SandBox()

#
# Regular run that checks the correct CSV file is created
# We are just checking its existence, not its content. To accelerate the test
# execution, the simulation is interrupted after 1.0sec of simulated time.
#
fdm = CreateFDM(sandbox)
fdm.load_script(sandbox.path_to_jsbsim_file('scripts', 'c1722.xml'))

fdm.run_ic()
ExecuteUntil(fdm, 1.0)

if not sandbox.exists('JSBout172B.csv'):
  print "Standard run: the file 'JSBout172B.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

#
# Reset the simulation and check that iteration number is correctly appended to
# the filename.
#
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if not sandbox.exists('JSBout172B_0.csv'):
  print "Reset: the file 'JSBout172B_0.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

#
# Change the output filename and check that the naming logic is reset (e.g. that
# no iteration number is appended to the filename
#
fdm.set_output_filename(0, 'dummy.csv')
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if not sandbox.exists('dummy.csv'):
  print "Output name renaming: the file 'dummy.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

#
# Call FGFDMExec::SetOutputFileName() after the simulation is reset. And verify
# that the new output file name is ignored until the next call to
# FGOutput::SetStartNewOutput(). This should be so according to the
# documentation of FGOutput::SetOutputName().
#
fdm.reset_to_initial_conditions(1)
fdm.set_output_filename(0, 'dummyx.csv')
ExecuteUntil(fdm, 1.0)

if sandbox.exists('dummyx.csv') or not sandbox.exists('dummy_0.csv'):
  if sandbox.exists('dummyx.csv'):
    print "Late renaming: 'dummyx.csv' should not exist."
  if not sandbox.exists('dummy_0.csv'):
    print "Late renaming: 'dummy_0.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

#
# Check that the new filename is taken into account when the simulation is reset
#
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if not sandbox.exists('dummyx.csv'):
  print "Reset after late renaming: 'dummyx.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

#
# Check against multiple calls to FGFDMExec::SetOutputFileName()
#
fdm.set_output_filename(0, 'this_one.csv')
fdm.set_output_filename(0, 'that_one.csv')
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if sandbox.exists('this_one.csv') or not sandbox.exists('that_one.csv'):
  if sandbox.exists('this_one.csv'):
    print "Output name overwritten: 'this_one.csv' should not exist."
  if not sandbox.exists('that_one.csv'):
    print "Output name overwritten: 'that_one.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

#
# Check again on a brand new FDM
#
sandbox.delete_csv_files()
fdm = CreateFDM(sandbox)
fdm.load_script(sandbox.path_to_jsbsim_file('scripts', 'c1722.xml'))

fdm.run_ic()
fdm.set_output_filename(0,'oops.csv') # Oops!! Changed my mind
ExecuteUntil(fdm, 1.0)

if sandbox.exists('oops.csv') or not sandbox.exists('JSBout172B.csv'):
  if sandbox.exists('oops.csv'):
    print "New FDM: 'oops.csv' should not exist."
  if not sandbox.exists('JSBout172B.csv'):
    print "New FDM: 'JSBout172B.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

#
# The new file name 'oops.csv' has been ignored.
# Check if it is now taken into account.
#
fdm.reset_to_initial_conditions(1)
ExecuteUntil(fdm, 1.0)

if not sandbox.exists('oops.csv'):
  print "Reset new FDM: 'oops.csv' should exist."
  sys.exit(-1) # 'make test' will report the test failed.

sandbox.erase()
