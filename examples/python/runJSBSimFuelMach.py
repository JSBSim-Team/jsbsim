# Developed Guilherme A. L. da Silva - aerothermalsolutions.co

# Calculation required by aircraft icing enginering
# Find the optimum value for fuel consumption 
# For greatest distance - low AoA and high speed (Carson cruise)
# For greatest time - high AoA and low speed (miminum power required)

# GNU Lesser General Public License v2.1

# To install jsbsim module in Python
# pip install jsbsim
# Or conda install jsbsim (if you have anaconda, this is the best way)

# To install matplotlib module in Python
# pip install matplotlib
# Or conda install matplotlib (if you have anaconda, this is the best way)

import jsbsim
import numpy as np
import matplotlib.pyplot as plt

# Alternate visualization just uncomment
# import matplotlib.style as style
# style.use('fivethirtyeight')

# Prepare subplots to overlay plots
fig, ((ax,ax1),(ax2,ax3)) = plt.subplots(2,2,figsize=(12,8),sharex=True)

# Fuel Max for Global5000
fuelmax=8097.63
# Define here the payloads to be studied
payload=[1500,15172]
# Define here the mass of fuel
fuel=[500,fuelmax]
# Two cases for weight
weight=["light","heavy"]
# Consumption Option
# 0 - lb/nm - find point near Carson Cruise
# 1 - lb - find point near Minimum Power Required
consOpt=0

# Format lines in plot
strDash=["-","-o"]

# Put your path here
PATH_TO_JSBSIM_FILES="/Users/gasilva/Documents/GitHub/jsbsim" #my is a macbook

# Variation of weight
for i in range(2):
    machMin=[]
    deltaMin=[]
    # Variation of altitude
    for k in range(5):
        results=[]
        tank=[]
        # Variation of speed
        for j in range(45):
            fdm = jsbsim.FGFDMExec(PATH_TO_JSBSIM_FILES) 
            fdm.load_model('global5000')
            fdm.reset_to_initial_conditions(1)
            fdm['propulsion/tank[0]/contents-lbs'] = fuel[i]
            fdm['propulsion/tank[1]/contents-lbs'] = fuel[i]
            fdm['propulsion/tank[2]/contents-lbs'] = fuel[i]
            fdm['inertia/pointmass-weight-lbs[0]'] = payload[i]
            # Set engines running
            fdm['propulsion/engine[0]/set-running'] = 1
            fdm['propulsion/engine[1]/set-running'] = 1
            fdm['ic/h-sl-ft'] = 10000+k*10000
            fdm['ic/gamma-deg'] = 0
            fdm['ap/alpha_hold']= 0
            fdm['ap/altitude_hold']= 0
            fdm['ic/vc-kts']=110+5*float(j)

            # Initial tank contents
            tank0i=fdm["propulsion/tank[0]/contents-lbs"]
            tank1i=fdm["propulsion/tank[1]/contents-lbs"]
            tank2i=fdm["propulsion/tank[2]/contents-lbs"]

            # Run JSBSim
            fdm.run_ic()
            fdm.run()

            # Trim
            try:
                fdm['simulation/do_simple_trim'] = 1

                while fdm.run() and fdm.get_sim_time() <= 10:
                    if fdm.get_sim_time()<1:
                        fdm['ap/altitude_hold']= 1
                    fdm.run()

                results.append((fdm['simulation/sim-time-sec'],fdm['velocities/vc-kts'], fdm['aero/alpha-deg'],fdm["propulsion/tank[0]/contents-lbs"],fdm["propulsion/tank[1]/contents-lbs"],fdm["propulsion/tank[2]/contents-lbs"]))

                tank0f=fdm["propulsion/tank[0]/contents-lbs"]
                tank1f=fdm["propulsion/tank[1]/contents-lbs"]
                tank2f=fdm["propulsion/tank[2]/contents-lbs"]

                # Calculate Fuel Used in each Tank
                deltaTank0=tank0i-tank0f
                deltaTank1=tank1i-tank1f
                deltaTank2=tank2i-tank2f
                deltaTotal=deltaTank0+deltaTank2+deltaTank1

                print("--------------------------------------")
                print("Fuel Consumed = {}".format(deltaTotal))
                print("Average Rate Fuel Consumed per hour = {}".format(deltaTotal/(1600/3600)))
                print("--------------------------------------")

                if consOpt==0:
                    distance=fdm["position/distance-from-start-mag-mt"]/1852
                else:
                    distance=1
                tank.append((fdm['velocities/mach'],fdm['aero/alpha-deg'],fdm['ic/h-sl-ft'],deltaTotal/distance,fdm["fcs/throttle-cmd-norm"]))

            except RuntimeError as e:
                if e.args[0] != 'Trim Failed':
                    raise

        if  len(results)>0 and len(tank)>0:
            time, speed, alpha, tank0, tank1, tank2 = zip(*results)
            machPlot,alpha,altitude,deltaPlot,TLA = zip(*tank)

            # Plot results
            size=len(machPlot)
            ax.plot(machPlot, deltaPlot[:size],strDash[i],label="{0:.0f} kft {1}".format(altitude[1]/1000,weight[i]))
            ax1.plot(machPlot, alpha[:size],strDash[i])
            ax2.plot(machPlot, altitude[:size],strDash[i])
            ax3.plot(machPlot, TLA[:size],strDash[i])
            ind=np.argmin(deltaPlot)
            machMin.append(machPlot[ind])
            deltaMin.append(deltaPlot[ind])

    # Plot line of optimum locals
    ax.plot(machMin,deltaMin,'--',linewidth=2.0,color="black")

# Finalize Plots
ax.legend(frameon=False,loc='lower left', ncol=2,fontsize=10)
ax2.set_xlabel('Mach')
ax3.set_xlabel('Mach')
if consOpt==0:
    ax.set_ylabel('Delta Fuel [lb/nm]')
else:
    ax.set_ylabel('Delta Fuel [lb]')
ax1.set_ylabel('Alpha [degrees]')
ax2.set_ylabel('Altitude [ft]')
ax3.set_ylabel('Throttle')
fig.suptitle('Fuel Consumption and Alpha per Mach')
plt.tight_layout()
plt.show()