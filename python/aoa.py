import jsbsim
import matplotlib.pyplot as plt
import math

fdm = jsbsim.FGFDMExec('.') # The path supplied to FGFDMExec is the location of the folders "aircraft", "engines" and "systems"

fdm.load_model('global5000') # Load the aircraft 737

# Set engines running
fdm['propulsion/engine[0]/set-running'] = 1
fdm['propulsion/engine[1]/set-running'] = 1

results = []
alt=30000

for speed in range(120, 460, 10):
    fdm['ic/h-sl-ft'] = alt
    fdm['ic/vc-kts'] = speed
    fdm['ic/gamma-deg'] = 0
    # Set alpha range for trim solutions
    fdm['aero/alpha-max-rad'] = math.radians(12)
    fdm['aero/alpha-min-rad'] = math.radians(-4.0)

    fdm.run_ic() # Initialize the aircraft with initial conditions

    fdm.run()

    # Trim
    try:
        fdm['simulation/do_simple_trim'] = 1
        results.append((fdm['velocities/vc-kts'], fdm['aero/alpha-deg']))
    except RuntimeError as e:
        # The trim cannot succeed. Just make sure that the raised exception
        # is due to the trim failure otherwise rethrow.
        if e.args[0] != 'Trim Failed':
            raise

for result in results:
    print(result[0], result[1])

speed, alpha = zip(*results)
plt.plot(speed, alpha)

plt.xlabel('KCAS (kt)')
plt.ylabel('AoA (deg)')
plt.title('AoA vs KCAS at {} ft'.format(alt))

plt.show()
