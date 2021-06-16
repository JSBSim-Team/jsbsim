Short instruction to how to use the S-function to integrate JSBSim with Simulink 

1. Build and install JSBSim. 
2. Open Matlab and navigate to the JSBSim folder. 
3. Open JSBSimSimulinkCompile and uncomment either the Linux or Windows mex row. 
4. Run JSBSimSimulinkCompile in the Matlab command window. Note that the compiler needed is gcc8. 
5. Open the Simulink example ex737cruise.slx and press run. 
6. Once the simulation is completed, run the clearSF.m file before running the Simulink program again. 



NOTE: 
* clearSF.m needs to be run after each Simulation. 
* The simulation can either be run from a script or from input from Simulink.
 
