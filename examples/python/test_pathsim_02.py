#
# Test script to simulate aircraft response to elevator input using JSBSim and PathSim
# NOTE: Make sure to upgrade to numpy>=1.26.4 to satisfy scipy requirements 
#
import os
import jsbsim
import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt

# Relative path to the directory where the flight model is stored
# Note - Aircraft directory needs to be writeable in order to modify the cg
PATH_TO_JSBSIM_FILES="../.."

print(f'=================================================================')
print(f'Current working directory: {os.getcwd()}')

# Global variables that must be modified to match your particular need
# The aircraft name
# Note - It should match the exact spelling of the model file
AIRCRAFT_NAME="global5000"

# Avoid flooding the console with log messages
jsbsim.FGJSBBase().debug_lvl = 0

# Instantiate the FDMExec object and load the aircraft
fdm = jsbsim.FGFDMExec(PATH_TO_JSBSIM_FILES)
fdm.load_model(AIRCRAFT_NAME)

ac_xml_tree = ET.parse(os.path.join(fdm.get_root_dir(), f'aircraft/{AIRCRAFT_NAME}/{AIRCRAFT_NAME}.xml'))
ac_xml_root = ac_xml_tree.getroot()

# Get the empty weight from aircraft xml [assume lbs]
for x in ac_xml_root.findall('mass_balance'):
        w = x.find('emptywt').text

empty_weight = float(w)

# Get the original CG from aircraft xml, assum inches from Construction axes origin
for loc in ac_xml_root.findall('mass_balance/location'):
        x_cg_ = loc.find('x').text

x_cg_0 = float(x_cg_)

# Assume a payload, midweight, lb
payload_0 = 15172/2

# Fuel max for Global5000, lbm
fuelmax = 8097.63

# Assume the mass of fuel, half tanks, lb
fuel_per_tank = fuelmax/2


# Select a speed to run the simulation at, knots
speed_cas = 250

# Assume a flight altitude
h_ft_0 = 15000

# Fuel max for Global5000, lbm
fuelmax = 8097.63

weight_0 = empty_weight + payload_0 + fuel_per_tank*3

# Assume a zero flight path angle, gamma_0
gamma_0 = 0

print("-----------------------------------------")
print("Altitude {} ft, Weight {} lb, CoG-x {} in".format(h_ft_0, weight_0, x_cg_0))
print("-----------------------------------------")
print("Running simulation at initial CAS = {} kn, and Altitude = {} ft".format(speed_cas, h_ft_0))
print("-----------------------------------------")

# Set engines running
fdm['propulsion/set-running'] = -1
trim_results = []
trim_results_fcs = []

fdm['ic/h-sl-ft'] = h_ft_0
fdm['ic/vc-kts'] = speed_cas
fdm['ic/gamma-deg'] = gamma_0
fdm['propulsion/tank[0]/contents-lbs'] = fuel_per_tank
fdm['propulsion/tank[1]/contents-lbs'] = fuel_per_tank
fdm['propulsion/tank[2]/contents-lbs'] = fuel_per_tank
fdm['inertia/pointmass-weight-lbs[0]'] = payload_0

# Initialize the aircraft with initial conditions
fdm.run_ic()

# Run fdm model
fdm.run()

# Trim the aircraft to initial conditions
fdm['simulation/do_simple_trim'] = 1
fdm.run()

print("-----------------------------------------")
trim_results.append((fdm['velocities/vc-kts'], fdm['aero/alpha-deg']))
trim_results_fcs.append((fdm['fcs/elevator-pos-rad'], fdm['fcs/elevator-pos-deg'], fdm['fcs/elevator-pos-norm']))
print("-----------------------------------------")

# Print trim results
print("Trim results:")
print("CAS (knots): {}, Alpha (deg): {}".format(trim_results[0][0], trim_results[0][1]))
print("Elevator pos (rad): {}, Elevator pos (deg): {}, Elevator pos (norm): {}".format(trim_results_fcs[0][0], trim_results_fcs[0][1], trim_results_fcs[0][2]))
print("-----------------------------------------")


import numpy as np
from pathsim import Simulation, Connection
from pathsim.blocks import DynamicalFunction, GaussianPulseSource, StepSource, Scope

def f_aircraft(u, t):
    fdm["fcs/elevator-cmd-norm"] = trim_results_fcs[0][2] + u
    fdm.run()
    return fdm["aero/alpha-deg"]

aircraft = DynamicalFunction(f_aircraft)

# -- Example Simulation --

# https://docs.pathsim.org/pathsim/v0.16.7/api

# Create a source signal (Gaussian pulse)
# src = GaussianPulseSource(amplitude=0.1, f_max=1.0, tau=0.8)

# starts at 0, jumps to -0.1 at 1, jumps to 0 at 2
src = StepSource(amplitude=[-0.1, 0, 0.1, 0], tau=[10, 11, 12, 13])

# Create a scope to visualize the control input and the VCO output
sco = Scope(labels=["U1=Elevator Command Normalized (x100)", "Alpha (deg)"])

def f_gain(u, t):
    return 100.0 * (trim_results_fcs[0][2] + u)

amp = DynamicalFunction(f_gain)

# Define connections
connections = [
    Connection(src, aircraft),
    Connection(src, amp),
    Connection(amp, sco[0]),
    Connection(aircraft, sco[1])
]

# Initialize simulation
# Use a small time step (dt) to capture the 100 Hz oscillation
sim = Simulation([src, aircraft, sco, amp], connections, dt=1e-3, log=True)

# Run simulation
if __name__ == "__main__":
    print("Running simulation...")
    sim.run(40.0)
    sco.plot()
    plt.show()

