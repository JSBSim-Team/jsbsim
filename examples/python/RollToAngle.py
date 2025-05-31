import jsbsim
import matplotlib.pyplot as plt
import math
import numpy as np

# Global variables that must be modified to match your particular need
# The aircraft name
# Note - It should match the exact spelling of the model file
AIRCRAFT_NAME="737Roll"
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


def xx(technique):
    fdm.reset_to_initial_conditions(1)

    # Initial conditions
    fdm['ic/h-sl-ft'] = 10000
    fdm['ic/vc-kts'] = 250
    fdm['ic/gamma-deg'] = 0

    # Initialize the aircraft with initial conditions
    fdm.run_ic()

    # Trim
    fdm['simulation/do_simple_trim'] = 1

    # aero/coefficient/Clda
    # aero/coefficient/Clp
    # velocities/p-rad_sec
    # attitude/psi-deg

    print(f"cg-x: {fdm['inertia/cg-x-in']}")
    print(f"cg-y: {fdm['inertia/cg-y-in']}")
    print(f"cg-z: {fdm['inertia/cg-z-in']}")

    fdm['propulsion/fuel_freeze'] = 1

    times = []
    cldas = []
    clps = []
    ps = []
    phis = []
    ailerons = []

    l_moments = []

    center = False
    center_time = 0

    while fdm.get_sim_time() < 10:
        fdm.run()
        times.append(fdm.get_sim_time())

        cldas.append(fdm['aero/coefficient/Clda'])
        clps.append(fdm['aero/coefficient/Clp'])
        ps.append(math.degrees(fdm['velocities/p-rad_sec']))
        phis.append(fdm['attitude/phi-deg'])
        ailerons.append(fdm['fcs/aileron-cmd-norm'])
        l_moments.append(fdm['moments/l-aero-lbsft'])

        if fdm.get_sim_time() >= 1:
            fdm['fcs/aileron-cmd-norm'] = 0.2

        if technique == "slow":
            #if fdm['attitude/phi-deg'] > 25:
            if fdm['attitude/phi-deg'] > 8:
                center = True

            if center:
                fdm['fcs/aileron-cmd-norm'] = 0
        else:
            if fdm['attitude/phi-deg'] > 27 and center is False:   
                center = True
                center_time = fdm.get_sim_time()

            if center:
                if fdm.get_sim_time() - center_time < 0.5:
                    fdm['fcs/aileron-cmd-norm'] = -0.1
                else:
                    fdm['fcs/aileron-cmd-norm'] = 0

        if fdm.get_sim_time() > 0:
            print(f"Time: {fdm.get_sim_time()}")
            print(f"Clda: {fdm['aero/coefficient/Clda']}")
            print(f"Clp: {fdm['aero/coefficient/Clp']}")
            print(f"Clb: {math.degrees(fdm['aero/coefficient/Clb'])}")
            print(f"Clr: {math.degrees(fdm['aero/coefficient/Clr'])}")
            print(f"Cldr: {fdm['aero/coefficient/Cldr']}")

            print(f"L-body: {fdm['moments/l-aero-lbsft']}")
            print(f"L-stability: {fdm['moments/roll-stab-aero-lbsft']}")
            print(f"L-wind: {fdm['moments/roll-wind-aero-lbsft']}")

    fig, (axMoments, axAngles, axControl) = plt.subplots(3, 1)

    axMoments.plot(times, cldas, label='$C_{l_{\delta a}}$')
    axMoments.plot(times, clps, label='$C_{l_p}$')
    axMoments.plot(times, l_moments, label='L Moment')

    axMoments.set_ylabel("lbsft")
    axMoments.legend()

    axAngles.plot(times, ps, label='Roll Rate')
    axAngles.plot(times, phis, label='Roll Angle')

    axControl.plot(times, ailerons, label='Aileron Command')

    axControl.legend()
    axControl.set_xlabel("Time (s)")

    axAngles.set_ylabel("deg, deg/s")
    axAngles.legend()

    plt.show()


xx('slow')
xx('fast')
