import jsbsim
import matplotlib.pyplot as plt
import math
import numpy as np

# Global variables that must be modified to match your particular need
# The aircraft name
# Note - It should match the exact spelling of the model file
AIRCRAFT_NAME="737THS"
# Path to JSBSim files, location of the folders "aircraft", "engines" and "systems"
PATH_TO_JSBSIM_FILES="../.."

# Avoid flooding the console with log messages
#jsbsim.FGJSBBase().debug_lvl = 0

fdm = jsbsim.FGFDMExec(PATH_TO_JSBSIM_FILES)

# Load the aircraft model
fdm.load_model(AIRCRAFT_NAME)

# Set engines running
fdm['propulsion/set-running'] = -1

# Set alpha range for trim solutions
fdm['aero/alpha-max-rad'] = math.radians(12)
fdm['aero/alpha-min-rad'] = math.radians(-4.0)

# Aircraft landing config
fdm['fcs/flap-cmd-norm'] = 1
fdm['fcs/gear-cmd-norm'] = 1

speeds = np.linspace(115, 135, 19)
stabs = []
stabnorms = []

for speed in speeds:
    # Initial conditions
    fdm['ic/h-sl-ft'] = 1000
    fdm['ic/vc-kts'] = speed
    fdm['ic/gamma-deg'] = -3

    # Initialize the aircraft with initial conditions
    fdm.run_ic()

    # Trim
    fdm['simulation/do_simple_trim'] = 1

    stabs.append(fdm['fcs/stabilizer-pos-deg'])
    stabnorms.append(fdm['fcs/pitch-trim-cmd-norm'])

plt.figure()
#plt.plot(speeds, stabs)
plt.plot(speeds, stabnorms)
plt.show()

