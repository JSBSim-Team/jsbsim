# Originally developed by JSBSim Team
# Modified by Guilherme A. L. da Silva - aerothermalsolutions.co
# Calculation required by aircraft icing enginering
# Information required to calculate fuel consuption in leveled flight
# with no slats or flaps in order to provide data for mission profile analysis

# GNU Lesser General Public License v2.1

# To install jsbsim module in Python
# pip install jsbsim
# Or conda install jsbsim (if you have anaconda, this is the best way)

# To install matplotlib module in Python
# pip install matplotlib
# Or conda install matplotlib (if you have anaconda, this is the best way)

# You should change aircraft XML global5000.xml to use
# autopilot global5000apMach.xml to hold Mach number

import jsbsim
import matplotlib.pyplot as plt

# Alternate visualization just uncomment
# import matplotlib.style as style
# style.use('fivethirtyeight')

# Prepare subplots to overlay plots
fig, ax = plt.subplots(figsize=(9,7))

# Initial results variable
results=[]

# Put your path here
PATH_TO_JSBSIM_FILES="/Users/gasilva/Documents/GitHub/jsbsim" #my is a macbook
# Executable of jsbsim
fdm = jsbsim.FGFDMExec(root_dir=PATH_TO_JSBSIM_FILES)

# Load script for cruise flight
fdm.load_script('aircraft/global5000/scripts/trim-cruise_ap.xml') 

# Initial conditions
fdm.reset_to_initial_conditions(1)
fdm['ic/mach']=0.43
fdm.run_ic()

# Mark first iteration
i=0
# optM True mach is kept
# optM False mach varies
optM=True
# Data for mach variation
machFinal=0.52
machInit=0.49

# Run each time step as defined in the script
while fdm.run():

    if optM:
        # Keep Mach along flight with a step
        if fdm['simulation/sim-time-sec']>2500:
            fdm['ap/mach_setpoint']=0.5
        else:
            fdm['ap/mach_setpoint']=0.43
    else:
        # Varying Mach
        fdm['ap/mach_setpoint']=machInit+(machFinal-machInit)*fdm['simulation/sim-time-sec']/18000       

    # Save results for plotting
    results.append((fdm['simulation/sim-time-sec'],fdm['velocities/mach'], 
        fdm['aero/alpha-deg'],fdm["propulsion/tank[0]/contents-lbs"],
        fdm["propulsion/tank[1]/contents-lbs"],fdm["propulsion/tank[2]/contents-lbs"],
        fdm["propulsion/engine/fuel-used-lbs"]))
    if i==0:
        # Inital tank contents
        tank0i=fdm["propulsion/tank[0]/contents-lbs"]
        tank1i=fdm["propulsion/tank[1]/contents-lbs"]
        tank2i=fdm["propulsion/tank[2]/contents-lbs"]
    i+=1
    # Final tank contents
    tank0f=fdm["propulsion/tank[0]/contents-lbs"]
    tank1f=fdm["propulsion/tank[1]/contents-lbs"]
    tank2f=fdm["propulsion/tank[2]/contents-lbs"]
    pass

# Delta Consumption for Tanks 0,1 and 2
deltaTank0=tank0i-tank0f
deltaTank1=tank1i-tank1f
deltaTank2=tank2i-tank2f
deltaTotal=deltaTank0+deltaTank2+deltaTank1
initialTanks=tank0i+tank1i+tank2i
print("--------------------------------------")
print("Fuel Consumed = {:.0f}".format(deltaTotal))
print("Average Rate Fuel Consumed per hour = {:.0f}".format(deltaTotal/5))
print("--------------------------------------")

# Variables for plotting
time, speed, alpha, tank0, tank1, tank2, allFuel = zip(*results)

# Subtract used fuel from inital fuel contents
allFuel= [initialTanks-2*x for x in allFuel]

# Plot results in time
ax.plot(time, tank0,'-',label="Tank 0")
ax.plot(time, tank1,'--',label="Tank 1")
ax.plot(time, tank2,'-',label="Tank 2")
ax.plot(time, allFuel,'-',label="All Tanks")

# Plot show final results
ax.legend(frameon=False)
ax.set_xlabel('Time [s]')
ax.set_ylabel('Fuel [lb]')
ax.set_title('Fuel Consumption')
ax.text(0.4, .7, "Fuel Consumed = {:.0f}".format(deltaTotal), fontsize=12,
    transform=ax.transAxes)
hours=time[-1]/3600
ax.text(0.4, .65, "Average Rate Fuel Consumed per hour = {:.0f}".format(deltaTotal/hours), 
    fontsize=12,transform=ax.transAxes)
plt.tight_layout()
plt.show()

# Prepare subplots to overlay plots
fig, ax = plt.subplots(figsize=(9,7))
ax.set_xlabel('Time [s]')
ax.set_ylabel('Mach')
ax.set_title('Mach Hold During Flight')
ax.plot(time, speed,'-',label="mach")
ax.ticklabel_format(useOffset=False)
ax.set_ylim([0,0.6])
plt.tight_layout()
plt.show()

# Prepare subplots to overlay plots
fig, ax = plt.subplots(figsize=(9,7))
ax.set_xlabel('Time [s]')
ax.set_ylabel('Mach')
ax.set_title('Zoom - Mach Hold During Flight')
ax.plot(time, speed,'-',label="mach")
ax.ticklabel_format(useOffset=False)
ax.set_ylim([.4,0.55])
ax.set_xlim([0,18000])
plt.tight_layout()
plt.show()

