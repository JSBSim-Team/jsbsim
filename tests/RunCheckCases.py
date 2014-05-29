import os, sys
from JSBSim_utils import InitFDM, Table
import jsbsim

path_to_jsbsim_files = os.path.join(os.path.relpath(sys.argv[1], os.getcwd()),
                                    'check_cases', 'orbit')

fdm = InitFDM(path_to_jsbsim_files)
fdm.load_script(os.path.join(path_to_jsbsim_files, 'scripts', 'ball_orbit.xml'))
fdm.run_ic()

while fdm.run():
    pass

ref, current = Table(), Table()
ref.ReadCSV(os.path.join(path_to_jsbsim_files, 'logged_data', 'BallOut.csv'))
current.ReadCSV('BallOut.csv')

diff = ref.compare(current)
if diff.empty():
    sys.exit(0) # Needed for 'make test' to report the test passed.

print diff
sys.exit(-1) # 'make test' will report the test failed.

