/*
Copyright (c) 2009, Brian Mills
All rights reserved.

Copyright (c) 2021, Agostino De Marco, Elia Tarasov, Michal Podhradsky, Tilda Sikström

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/* JSBSIm SFunction 2021-07-08
 *
 * Several changes have been made to integrate Simulink with JSBSim Version 1.1.6
 * For the original code, see 
 * https://se.mathworks.com/matlabcentral/fileexchange/25042-jsbsim-s-function-gui-0-3
 * and 
 * https://github.com/podhrmic/JSBSim-Matlab
 * A big thanks to Michal Podhradsky for the work done. 
 *
 * SFunction block parameters are changed to: 
 * 'ac_name_string', [u-fps v-fps w-fps p-radsec q-radsec r-radsec h-sl-ft long-gc-deg lat-gc-deg 
 *   phi-rad theta-rad psi-rad],
 * [throttle-cmd-norm aileron-cmd-norm elevator-cmd-norm rudder-cmd-norm mixture-cmd-norm set-running flaps-cmd-norm gear-cmd-norm],
 * [delta_T], 'script/scriptname'
 * 
 * This means it is now possible to define a script as usual in JSBSim. If
 * a valid script name is not defined, Simulink will try to load the specified 
 * aircraft and run the script using the input parameters. 
 *
 * Input parameters: [throttle, aileron, elevator, rudder, mixture, set-running, flaps and gear]
 *
 * Output parameters have been updated, there are 4 output ports. 
 * 0 (states): [u-fps v-fps w-fps p-rad-sec q-rad-sec r-rad-sec h-sl-ft long-deg lat-deg phi-rad theta-rad psi-rad]
 * 1 (Flight controls): [thr-pos-norm left-ail-pos-rad right-ail-pos-rad el-pos-rad rud-pos-rad flap-pos-norm 
 *   speedbrake-pos-rad spoiler-pos-rad gear-pos-norm] 
 * 2: (Propulsion output): Not yet defined in the SFunction. Placeholder. Needs to be engine dependent. 
 * 3: (Pilot related output): [pilot-Nz alpha-rad alpha-dot-rad-sec beta-rad beta-dot-rad-sec vc-fps vc-kts 
 *						Vt-fps vg-fps mach climb-rate-fps qbar-psf]
 *
 * Verbosity settings and JSBSim Multiplier have been removed. 
 * 
 * It is currently needed to run the clearSF.m function in the command window 
 * in matlab before each simulation. This should be fixed. 
 *
 * 2021-07-08 Tilda Sikström
 * Linköping, Sweden 
 *
 * 
 * ***********************************************************************************************
 * **************************************************************************************************
 * Bug fixes
 * %%% Fixed issues with Debug Verbosity settings
 * %%% Fixed problem with "verbose" Verbosity setting that did not allow simulation to run properly
 * %%% Fixed issue with throttles not being initialized properly and angines not being properly spooled up to the 
 * intended power setting
 *
 * 01/22/10 Brian Mills
 * 
 * *********Discrete States Version******************************************************************
 * JSBSim calculates states.  NO integration performed by Simulink.
 * Use fixed step discrete state solver
 * Basic implementation of a JSBSim S-Function that takes 5 input parameters
 * at the S-Function's block parameters dialog box:
 * 'ac_name_string', 
 * [u-fps v-fps w-fps p-radsec q-radsec r-radsec h-sl-ft long-gc-deg lat-gc-deg 
 *   phi-rad theta-rad psi-rad],
 * [throttle-cmd-norm aileron-cmd-norm elevator-cmd-norm rudder-cmd-norm mixture-cmd-norm set-running flaps-cmd-norm gear-cmd-norm],
 * [delta_T], 'script/scriptname'
 * The model currently takes 8 control inputs:throttle, aileron, elevator, rudder, mixture, set-running, flaps and gear.
 * The model has 12 states:[u-fps v-fps w-fps p-rad-sec q-rad-sec r-rad-sec h-sl-ft long-deg lat-deg phi-rad theta-rad psi-rad] 
 * Model has 4 output ports: state vector, control output vector, propulsion output vector and calculated output vector.
 * States output [u-fps v-fps w-fps p-rad-sec q-rad-sec r-rad-sec h-sl-ft long-deg lat-deg phi-rad theta-rad psi-rad] 
 * Flight Controls output [thr-pos-norm left-ail-pos-rad right-ail-pos-rad el-pos-rad rud-pos-rad flap-pos-norm 
     speedbrake-pos-rad spoiler-pos-rad gear-pos-norm]
 * Propulsion output piston (per engine) [prop-rpm prop-thrust-lbs mixture fuel-flow-gph advance-ratio power-hp pt-lbs_sqft 
 * volumetric-efficiency bsfc-lbs_hphr prop-torque blade-angle prop-pitch]
 * Propulsion output turbine (per engine) [thrust-lbs n1 n2 fuel-flow-pph fuel-flow-pps pt-lbs_sqft pitch-rad reverser-rad yaw-rad inject-cmd 
 * set-running fuel-dump]
 * Calculated outputs [pilot-Nz alpha-rad alpha-dot-rad-sec beta-rad beta-dot-rad-sec vc-fps vc-kts 
 *						Vt-fps vg-fps mach climb-rate-fps]
 *
 * The UpdateStates method added to JSBSimInterface is called for every s-function simulation time step.
 * Currently it is advised that if the AC model FCS has integrators, then after each simulation run "clearSF" 
 * should be entered at the Matlab command line to reset the simulation.
 * This will ensure that every consecutive simulation run starts from the same initial states.  
 * It is planned to fix this in the near future.
 * Please look in the mdlInitializeSizes method for more detailed input port and output port details.
 * *************************************************************************************************************************
 * *************************************************************************************************************************
 * 08/08/09 JSBSimSFunction revision 1.0 for campatibility with JSBSim 1.0
 * Brian Mills
 * *************************************************************************************************************************
 * JSBSimInterface written by Agostino De Marco for use in the JSBSimMexFunction project. Additional functions have been added and changes 
 * made to work with SFunction API. Thanks to Agostino for providing the basis for this project.
 * *************************************************************************************************************************

*/
#ifdef __cplusplus
 
#endif       // defined within this scope
#define S_FUNCTION_NAME  JSBSim_SFunction
#define S_FUNCTION_LEVEL 2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */
#include "simstruc.h"
#include "mex.h"
#include "matrix.h"
#include <iostream>
#include <string>
#include <vector>
#include <FGFDMExec.h>
#include <models/FGPropagate.h>
#include <models/FGAuxiliary.h>
#include <models/FGFCS.h>
#include "JSBSimInterface.h"

// Name of JSBSim aircraft model file to load
#define AIRCRAFT_NAME_PARAM     0

#define ac_name			        ssGetSFcnParam(S, AIRCRAFT_NAME_PARAM)

// Initial conditions for the aircraft state
#define INITIAL_STATE_PARAM     1
#define SIZE_INITIAL_STATES     12

#define u_fps			        mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[0]
#define v_fps			        mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[1]
#define w_fps			        mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[2]

#define p_radsec                mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[3]
#define q_radsec                mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[4]
#define r_radsec                mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[5]

#define h_sl_ft		            mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[6]
#define long_gc_deg	            mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[7]
#define lat_gc_deg		        mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[8]

#define phi_rad			        mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[9]
#define theta_rad		        mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[10]
#define psi_rad			        mxGetPr(ssGetSFcnParam(S, INITIAL_STATE_PARAM))[11]

// Initial conditions for the control commands
#define INITIAL_CTRL_CMD_PARAM  2
#define SIZE_INITIAL_CRTL_CMD   10

#define throttle1               mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[0]
#define throttle2               mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[1]
#define aileron                 mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[2]
#define elevator                mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[3]
#define rudder                  mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[4]
#define mixture1                mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[5]
#define mixture2                mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[6]
#define runset                  mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[7]
#define flaps                   mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[8]
#define gear                    mxGetPr(ssGetSFcnParam(S, INITIAL_CTRL_CMD_PARAM))[9]

// Simulation discrete time step
#define TIME_STEP_PARAM         3

#define delta_t		            mxGetPr(ssGetSFcnParam(S, TIME_STEP_PARAM))[0]

// Parameter for using the supplied script (1 to use, 0 to not use) and
// for enabling control input to script (1 to enable, 0 to disable)
#define USE_SCRIPT_PARAM        4
#define SIZE_USE_SCRIPT_PARAM   2

#define use_script	            mxGetPr(ssGetSFcnParam(S, USE_SCRIPT_PARAM))[0]
#define allow_control_of_script	mxGetPr(ssGetSFcnParam(S, USE_SCRIPT_PARAM))[1]

// The file path to the script to run
#define SCRIPT_FILE_PARAM       5

#define script_name             ssGetSFcnParam(S, SCRIPT_FILE_PARAM)

// Initial condition parameters for resetting the aircraft, from aircraft type directory
#define RESET_FILE_PARAM        6

#define reset_name              ssGetSFcnParam(S, RESET_FILE_PARAM)

#define NUM_PARAMS              7

// The input
#define NUM_INPUTS              1

#define CTRL_CMD_INPUT          0
#define SIZE_CTRL_CMD_INPUT     SIZE_INITIAL_CRTL_CMD

// The output
#define NUM_OUTPUTS             4

#define STATES_OUTPUT           0
#define SIZE_STATES_OUTPUT      12

#define CTRL_CMD_OUTPUT         1
#define SIZE_CTRL_CMD_OUTPUT    10

#define PROPULSION_OUTPUT       2
#define SIZE_PROPULSION_OUTPUT  48

#define AUX_VARS_OUTPUT         3
#define SIZE_AUX_VARS_OUTPUT    13

// Necessary to create mxArray of initial conditions 
#define NUMBER_OF_STRUCTS (sizeof(ic)/sizeof(struct init_cond))
#define NUMBER_OF_FIELDS (sizeof(field_names)/sizeof(*field_names))


struct init_cond{
    const char *name;
    double value;
};


/* Error handling
 * --------------
 *
 * You should use the following technique to report errors encountered within
 * an S-function:
 *
 *       ssSetErrorStatus(S,"Error encountered due to ...");
 *       return;
 *
 * Note that the 2nd argument to ssSetErrorStatus must be persistent memory.
 * It cannot be a local variable. For example the following will cause
 * unpredictable errors:
 *
 *      mdlOutputs()
 *      {
 *         char msg[256];         {ILLEGAL: to fix use "static char msg[256];"}
 *         sprintf(msg,"Error due to %s", string);
 *         ssSetErrorStatus(S,msg);
 *         return;
 *      }
 *
 * See matlabroot/simulink/src/sfuntmpl_doc.c for more details.
 */

/*====================*
 * S-function methods *
 *====================*/

/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */
static void mdlInitializeSizes(SimStruct *S)
{
  /* See sfuntmpl_doc.c for more details on the macros below */
    ssSetNumSFcnParams(S, NUM_PARAMS);  /* Number of expected parameter vectors*/ 
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
        /* Return if number of expected != number of actual parameters */
        return;
    }

    ssSetNumDiscStates(S, SIZE_STATES_OUTPUT);

    if (!ssSetNumInputPorts(S, NUM_INPUTS)) return;
    ssSetInputPortWidth(S, CTRL_CMD_INPUT, SIZE_CTRL_CMD_INPUT);


    if (!ssSetNumOutputPorts(S, NUM_OUTPUTS)) return;
    ssSetOutputPortWidth(S, STATES_OUTPUT, SIZE_STATES_OUTPUT);
	ssSetOutputPortWidth(S, CTRL_CMD_OUTPUT, SIZE_CTRL_CMD_OUTPUT);
	/* Propulsion output piston (per engine) [prop-rpm prop-thrust-lbs mixture fuel-flow-gph advance-ratio power-hp pt-lbs_sqft 
	 * volumetric-efficiency bsfc-lbs_hphr prop-torque blade-angle prop-pitch]
	 * Propulsion output turbine (per engine) [thrust-lbs n1 n2 fuel-flow-pph fuel-flow-pps pt-lbs_sqft pitch-rad reverser-rad yaw-rad inject-cmd 
	 * set-running fuel-dump]
	 */
    //TODO: currently not in use, could be defined later 
	ssSetOutputPortWidth(S, PROPULSION_OUTPUT, SIZE_PROPULSION_OUTPUT);	
	ssSetOutputPortWidth(S, AUX_VARS_OUTPUT, SIZE_AUX_VARS_OUTPUT);
    
    if(!ssSetNumDWork(   S, 4)) return; //HW change 

    ssSetDWorkWidth(     S, 0, ssGetInputPortWidth(S,0));//Work vector for input port
    ssSetDWorkDataType(  S, 0, SS_DOUBLE); /* use SS_DOUBLE if needed */

    // Work vector for states-- don't need since stored in ssGetRealDiscStates
    // TODO: may need to add actuator states!
    // ssSetDWorkWidth(     S, 1, ssGetNumDiscStates(S));
    // ssSetDWorkDataType(  S, 1, SS_DOUBLE);

    // // Work vector derivatives-- don't need since stored in ssGetdX
	// ssSetDWorkWidth(     S, 2, ssGetNumDiscStates(S));
    // ssSetDWorkDataType(  S, 2, SS_DOUBLE);

    // Work vector for flight controls outputs
	ssSetDWorkWidth(     S, 1, ssGetOutputPortWidth(S,1));
    ssSetDWorkDataType(  S, 1, SS_DOUBLE);

    // Work vector for propulsion outputs
	ssSetDWorkWidth(     S, 2, ssGetOutputPortWidth(S,2));
    ssSetDWorkDataType(  S, 2, SS_DOUBLE);

    // Work vector for auxillary outputs
	ssSetDWorkWidth(     S, 3, ssGetOutputPortWidth(S,3));
    ssSetDWorkDataType(  S, 3, SS_DOUBLE);	
    
	ssSetNumPWork(S, 1); // reserve element in the pointers vector
                         // to store a JSBSimInterface

    ssSetNumNonsampledZCs(S, 0);

    ssSetOptions(S, 0);
		
}



/* Function: mdlInitializeSampleTimes =========================================
 * Abstract:
 *    This function is used to specify the sample time(s) for your
 *    S-function. You must register the same number of sample times as
 *    specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, delta_t);
    ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_INITIALIZE_CONDITIONS   /* Change to #undef to remove function */
#if defined(MDL_INITIALIZE_CONDITIONS)
/* Function: mdlInitializeConditions ========================================
* Abstract:
*    In this function, you should initialize the continuous and discrete
*    states for your S-function block.  The initial states are placed
*    in the state vector, ssGetContStates(S) or ssGetRealDiscStates(S).
*    You can also perform any other initialization activities that your
*    S-function may require. Note, this routine will be called at the
*    start of simulation and if it is present in an enabled subsystem
*    configured to reset states, it will be call when the enabled subsystem
*    restarts execution to reset the states.
*/
static void mdlInitializeConditions(SimStruct *S)
{	
    /* Create new JSBSimInterface object and initialize it with delta_t and
        also create a pointer to the JSBSimInterface object so we can access its member
        functions.
    */
    ssGetPWork(S)[0] = (void *) new JSBSimInterface(delta_t);
    JSBSimInterface *JII = (JSBSimInterface *) ssGetPWork(S)[0];
    
    // Check if a script file is given in Simulink.
    // If not, initialize an aircraft
    // The RunIC() command is run in either OpenScript or Copy_Init_To_JSBSim.
        
    // Keep this as null so that we can use the initialization settings from the script.
    // See bool FGScript::LoadScript(const SGPath&, double, const SGPath&) for details.
    SGPath initfile;
    initfile = "";
    
    // Get the user provided script.
    char buf[128];
    mwSize buflen;
    buflen = mxGetNumberOfElements(script_name) + 1;
    mxGetString(script_name, buf, buflen);
    std::string script = "";
    script = std::string(buf);
    mexPrintf("Script input: %s \n", script.c_str());

    // Check both that script is set to be used and that it works to open the script 
    if (use_script && JII->OpenScript(SGPath(script), delta_t, initfile)){
        
        mexPrintf("Using Scripts! \n");
    }
    
    // Get the user provided intialization settings for resetting the aircraft
    // to a default state.
    char buf2[128];
    mwSize buflen2;        
    buflen2 = mxGetNumberOfElements(reset_name) +1; 
    mxGetString(reset_name, buf2, buflen2); 
    std::string reset = "";
    reset = std::string(buf2);
    mexPrintf("Reset file: '%s' .\n", reset.c_str());

    if (!use_script){

        mexPrintf("\nJSBSim S-Function is initializing...\n\n");
        mexPrintf("Note: For Aircraft with integrators in the FCS, please type 'clearSF' to completely reset S-Function.\n\n");

        // Open the supplied aircraft file.
        char buf[128];
        mwSize buflen;
        buflen = mxGetNumberOfElements(ac_name) + 1;
        mxGetString(ac_name, buf, buflen);
        std::string aircraft = "";
        aircraft = std::string(buf);
        if (!JII->Open(aircraft)){
            mexErrMsgTxt("Aircraft file could not be loaded.\n");
        }

        mexPrintf("'%s' Aircraft File has been successfully loaded!\n", aircraft.c_str());
        JII->LoadIC(SGPath(reset));
    }
}
#endif /* MDL_INITIALIZE_CONDITIONS */



#define MDL_START  /* Change to #undef to remove function */
#if defined(MDL_START) 
/* Function: mdlStart =======================================================
* Abstract:
*    This function is called once at start of model execution. If you
*    have states that should be initialized once, this is the place
*    to do it.
*/
static void mdlStart(SimStruct *S)
{
    JSBSimInterface *JII = (JSBSimInterface *) ssGetPWork(S)[0];

    // TODO: We may want to generalize this.
    // real_T *x2 = ssGetRealDiscStates(S);

    // JII->Copy_States_From_JSBSim(x2);

    // x2[0] = u_fps;
    // x2[1] = v_fps;
    // x2[2] = w_fps;
    // x2[3] = p_radsec;
    // x2[4] = q_radsec;
    // x2[5] = r_radsec;
    // x2[6] = h_sl_ft;
    // x2[7] = long_gc_deg;
    // x2[8] = lat_gc_deg;
    // x2[9] = phi_rad;
    // x2[10] = theta_rad;
    // x2[11] = psi_rad;
    //x[12] = alpha_rad;
    //x[13] = beta_rad;
}
#endif /*  MDL_START */

#undef MDL_SET_OUTPUT_PORT_WIDTH   /* Change to #undef to remove function */
#if defined(MDL_SET_OUTPUT_PORT_WIDTH) && defined(MATLAB_MEX_FILE)
  /* Function: mdlSetOutputPortWidth ==========================================
   * Abstract:
   *    This method is called with the candidate width for a dynamically
   *    sized port.  If the proposed width is acceptable, the method should
   *    go ahead and set the actual port width using ssSetOutputPortWidth.  If
   *    the size is unacceptable an error should generated via
   *    ssSetErrorStatus.  Note that any other dynamically sized input or
   *    output ports whose widths are implicitly defined by virtue of knowing
   *    the width of the given port can also have their widths set via calls
   *    to ssSetInputPortWidth or ssSetOutputPortWidth.
   */
  static void mdlSetOutputPortWidth(SimStruct *S, int portIndex, int width)
  {
	 ssSetOutputPortWidth(S, 2, ssGetDWorkWidth(S,4));
	 

  } /* end mdlSetOutputPortWidth */
#endif /* MDL_SET_OUTPUT_PORT_WIDTH */

/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block.
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{ 
    
    // These are vectors with the length of the output port
    real_T *statesOutput = ssGetOutputPortRealSignal(S, STATES_OUTPUT);
	real_T *ctrlCmdOutput = ssGetOutputPortRealSignal(S, CTRL_CMD_OUTPUT);
	real_T *propulsionOutput = ssGetOutputPortRealSignal(S, PROPULSION_OUTPUT);
	real_T *auxVarsOutput = ssGetOutputPortRealSignal(S, AUX_VARS_OUTPUT);
    
    real_T *discStates = ssGetRealDiscStates(S);
	double *dWorkCtrlCmd = (double *) ssGetDWork(S,1);
	double *dWorkPropulsion = (double *) ssGetDWork(S,2);
	double *dWorkAuxVars = (double *) ssGetDWork(S,3);
    
    int i;
	for (i = 0; i < ssGetNumDiscStates(S); i++)
	{
	    statesOutput[i] = discStates[i]; /* outputs are the states */
	}

	for (i = 0; i < ssGetDWorkWidth(S,1); i++)
	{
		ctrlCmdOutput[i] = dWorkCtrlCmd[i]; // outputs are the flight control outputs 
	}

	for (i = 0; i < ssGetDWorkWidth(S,2); i++)
	{
		propulsionOutput[i] = dWorkPropulsion[i]; // outputs are the propulsion outputs 
	}
	for (i = 0; i < ssGetDWorkWidth(S,3); i++)
	{
		auxVarsOutput[i] = dWorkAuxVars[i]; // outputs are the calculated pilot/auxillary outputs 
	}
}

#define MDL_UPDATE  /* Change to #undef to remove function */
#if defined(MDL_UPDATE)
  /* Function: mdlUpdate ======================================================
   * Abstract:
   *    This function is called once for every major integration time step.
   *    Discrete states are typically updated here, but this function is useful
   *    for performing any tasks that should only take place once per
   *    integration step.
   */
static void mdlUpdate(SimStruct *S, int_T tid)
{
    /* send update inputs to JSBSimInterface, run one cycle, 
    retrieve state vector, and update sim state vector 
    */

    JSBSimInterface *JII = (JSBSimInterface *) ssGetPWork(S)[0];
    
    real_T *discStates = ssGetRealDiscStates(S);

    InputRealPtrsType ctrlCmdInput = ssGetInputPortRealSignalPtrs(S,CTRL_CMD_INPUT);
    throttle1 = *ctrlCmdInput[0];
    throttle2 = *ctrlCmdInput[1];
    aileron = *ctrlCmdInput[2];
    elevator = *ctrlCmdInput[3];
    rudder = *ctrlCmdInput[4];
    mixture1 = *ctrlCmdInput[5];
    mixture2 = *ctrlCmdInput[6];
    runset = *ctrlCmdInput[7];
    flaps = *ctrlCmdInput[8];
    gear = *ctrlCmdInput[9];

    // These point to the output 
    double *dWorkCtrlCmdIn = (double *) ssGetDWork(S,0);
    double *dWorkCtrlCmdOut = (double *) ssGetDWork(S,1);
	double *dWorkPropulsion = (double *) ssGetDWork(S,2);
	double *dWorkAuxVars = (double *) ssGetDWork(S,3);
    
    int i;
    
    for (i = 0; i < ssGetDWorkWidth(S,0); i++) {
        dWorkCtrlCmdIn[i] = *ctrlCmdInput[i];
    }
    
    double ctrl_vec[SIZE_CTRL_CMD_INPUT] = {throttle1, throttle2, aileron, elevator, rudder, mixture1, mixture2, runset, flaps, gear};

    if (allow_control_of_script) {         
        JII->Copy_Controls_To_JSBSim(ctrl_vec);
    }

    JII->Update();
    
    JII->Copy_States_From_JSBSim(discStates);
    JII->Copy_Control_From_JSBSim(dWorkCtrlCmdOut);
    JII->Copy_Pilot_From_JSBSim(dWorkAuxVars);
}
#endif /* MDL_UPDATE */

/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.  For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
	
	JSBSimInterface *JII = (JSBSimInterface *) ssGetPWork(S)[0];
    delete JII; 

	mexPrintf("\n");
	mexPrintf("Simulation completed.\n");
	mexPrintf("Remember to reset the program by typing clearSF in the matlab command window! \n");
}

/*======================================================*
 * See sfuntmpl_doc.c for the optional S-function methods *
 *======================================================*/

/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
