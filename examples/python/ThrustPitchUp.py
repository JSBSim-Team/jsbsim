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

# Initial conditions
fdm['ic/h-sl-ft'] = 1000
fdm['ic/vc-kts'] = 135
fdm['ic/gamma-deg'] = -3

# Initialize the aircraft with initial conditions
fdm.run_ic()

# Trim
fdm['simulation/do_simple_trim'] = 1

print(f"Gear: {fdm['gear/gear-pos-norm']}")
print(f"Flap: {fdm['fcs/flap-pos-norm']}")
print(f"Stab: {fdm['fcs/stabilizer-pos-deg']}")

speeds = [115.        , 116.11111111, 117.22222222, 118.33333333,
       119.44444444, 120.55555556, 121.66666667, 122.77777778,
       123.88888889, 125.        , 126.11111111, 127.22222222,
       128.33333333, 129.44444444, 130.55555556, 131.66666667,
       132.77777778, 133.88888889, 135.        ]

stabnorms = [-0.6285902765496523, -0.6024333892168459, -0.5771978362735471, -0.5529232096791581, -0.5295389183108126, -0.5070066443487823, -0.4852898214754092, -0.4643535611292797, -0.444164580312699, -0.42469113119307034, -0.4059029326817033, -0.38777110412855487, -0.37026562850178846, -0.3533673186218593, -0.33704599540939717, -0.32127780446383825, -0.306040178558573, -0.29133292678122624, -0.2770849742467696]

times = []
thetas = []
ias = []
aoa = []
gamma = []
stabs = []

fdm['fcs/throttle-cmd-norm[0]'] = 0
fdm['fcs/throttle-cmd-norm[1]'] = 0

go_around = False
go_around_time = 0
stab_trim = fdm['fcs/pitch-trim-cmd-norm']

i_error = 0

while fdm.get_sim_time() < 40:
    fdm.run()
    times.append(fdm.get_sim_time())

    ias.append(fdm['velocities/vc-kts'])
    aoa.append(fdm['aero/alpha-deg'])
    thetas.append(fdm['attitude/theta-deg'])
    gamma.append(fdm['flight-path/gamma-deg'])
    stabs.append(fdm['fcs/stabilizer-pos-deg'])

    if go_around is False:
        """
        gamma_error = -3.0 - fdm['flight-path/gamma-deg']
        stab = fdm['fcs/pitch-trim-cmd-norm']
        if fdm.get_sim_time() < 3.5:
            stab -= 0.045/120.0
        else:
            stab -= 0.005/120.0
        #stab += gamma_error * -0.03/120.0
        i_error += gamma_error
        #stab = stab_trim + gamma_error * -0.3 + i_error * -0.01
        #stab = stab_trim + gamma_error * -0.3
        fdm['fcs/pitch-trim-cmd-norm'] = max(stab, -1.0)
        print(stab)
        """
        stabnorm = np.interp(fdm['velocities/vc-kts'], speeds, stabnorms)
        #fdm['fcs/pitch-trim-cmd-norm'] = stabnorm * 1.1  # Good one
        fdm['fcs/pitch-trim-cmd-norm'] = stabnorm * 1.08

    if fdm['velocities/vc-kts'] <= 115:
        go_around = True
        if go_around_time == 0:
            go_around_time = fdm.get_sim_time()
        fdm['fcs/throttle-cmd-norm[0]'] = 1
        fdm['fcs/throttle-cmd-norm[1]'] = 1

    if fdm['attitude/theta-deg'] > 15:
        fdm['fcs/elevator-cmd-norm'] = 0.2


fig, (axIAS, axAngles, axStab) = plt.subplots(3, 1)

axIAS.plot(times, ias, label='IAS')
axAngles.plot(times, aoa, label='$\\alpha$ - Angle of Attack')
axAngles.plot(times, thetas, label='$\\theta$ - Pitch Angle')
axAngles.plot(times, gamma, label='$\\gamma$ - Flight Path Angle')
axStab.plot(times, stabs, label='Stabilizer Position')

for ax in (axIAS, axAngles, axStab):
    ax.set_xlabel('Time (s)')
    ax.axvline(go_around_time, color='red', linestyle='--', label='Go Around')
    ax.legend()

axIAS.set_ylabel('IAS (kts)')
axAngles.set_ylabel('Angle (deg)')

plt.show()
