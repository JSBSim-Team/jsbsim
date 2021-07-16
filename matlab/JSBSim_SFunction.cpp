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
#include "mclcppclass.h"
#include "matrix.h"
#include <iostream>
#include <string>
#include <vector>
#include <FGFDMExec.h>
#include <models/FGPropagate.h>
#include <models/FGAuxiliary.h>
#include <models/FGFCS.h>
#include "JSBSimInterface.h"


// 12 States of Initial Condition Vector
#define u_fps			mxGetPr(ssGetSFcnParam(S, 1))[0]
#define v_fps			mxGetPr(ssGetSFcnParam(S, 1))[1]
#define w_fps			mxGetPr(ssGetSFcnParam(S, 1))[2]

#define p_radsec          mxGetPr(ssGetSFcnParam(S, 1))[3]
#define q_radsec          mxGetPr(ssGetSFcnParam(S, 1))[4]
#define r_radsec          mxGetPr(ssGetSFcnParam(S, 1))[5]

#define h_sl_ft				mxGetPr(ssGetSFcnParam(S, 1))[6]
#define long_gc_deg			mxGetPr(ssGetSFcnParam(S, 1))[7]
#define lat_gc_deg			mxGetPr(ssGetSFcnParam(S, 1))[8]

#define phi_rad			mxGetPr(ssGetSFcnParam(S, 1))[9]
#define theta_rad		mxGetPr(ssGetSFcnParam(S, 1))[10]
#define psi_rad			mxGetPr(ssGetSFcnParam(S, 1))[11]


// Initial conditions for the controls 
#define throttle         mxGetPr(ssGetSFcnParam(S, 2))[0]
#define aileron              mxGetPr(ssGetSFcnParam(S, 2))[1]
#define elevator              mxGetPr(ssGetSFcnParam(S, 2))[2]
#define rudder              mxGetPr(ssGetSFcnParam(S, 2))[3]
#define mixture             mxGetPr(ssGetSFcnParam(S, 2))[4]
#define runset              mxGetPr(ssGetSFcnParam(S, 2))[5]
#define flaps              mxGetPr(ssGetSFcnParam(S, 2))[6]
#define gear              mxGetPr(ssGetSFcnParam(S, 2))[7]

#define delta_t				mxGetPr(ssGetSFcnParam(S, 3))[0] // delta T
#define script_bool				mxGetPr(ssGetSFcnParam(S, 3))[1] //1 if use script 
//#define verbosity			ssGetSFcnParam(S, 4) //Verbosity parameter
#define ac_name				ssGetSFcnParam(S, 0) //Name of JSBSim aircraft model file to load
#define reset_name          ssGetSFcnParam(S, 5) //Reset name 
//#define multiplier			mxGetPr(ssGetSFcnParam(S, 5))[0] //JSBSim multiplier
#define script_name         ssGetSFcnParam(S, 4)


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
    ssSetNumSFcnParams(S, 6);  /* Number of expected parameter vectors*/ 
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
        /* Return if number of expected != number of actual parameters */
        return;
    }

    //ssSetNumContStates(S, 12);
    ssSetNumDiscStates(S, 12);

    /* if (!ssSetNumInputPorts(S, 1)) return; */
	ssSetNumInputPorts(S, 1);
    ssSetInputPortWidth(S, 0, 8);//[thr ail el rud mxtr run flap gear]
    
    /* ssSetInputPortRequiredContiguous(S, 0, true); /*direct input signal access*/

    if (!ssSetNumOutputPorts(S, 4)) return;
    ssSetOutputPortWidth(S, 0, 12);//The model has 12 states:[u v w p q r h-sl-ft long lat phi theta psi]	
	
	/* Flight Controls output [thr-pos-norm left-ail-pos-rad right-ail-pos-rad el-pos-rad rud-pos-rad flap-pos-norm ]
     * speedbrake-pos-rad spoiler-pos-rad gear-pos-norm]
	 */
	ssSetOutputPortWidth(S, 1, 9);

	/* Propulsion output piston (per engine) [prop-rpm prop-thrust-lbs mixture fuel-flow-gph advance-ratio power-hp pt-lbs_sqft 
	 * volumetric-efficiency bsfc-lbs_hphr prop-torque blade-angle prop-pitch]
	 * Propulsion output turbine (per engine) [thrust-lbs n1 n2 fuel-flow-pph fuel-flow-pps pt-lbs_sqft pitch-rad reverser-rad yaw-rad inject-cmd 
	 * set-running fuel-dump]
	 */
	ssSetOutputPortWidth(S, 2, 48); //currently not in use, could be defined later 
		

	ssSetOutputPortWidth(S, 3, 13);//Calculated outputs [pilot-Nz alpha alpha-dot beta beta-dot vc-fps vc-kts 
 						           //					 Vt-fps vg-fps mach climb-rate qbar-psf el-cmd-norm]    
    
	//ssSetNumSampleTimes(S, 1);
    if(!ssSetNumDWork(   S, 6)) return; //HW change 

    ssSetDWorkWidth(     S, 0, ssGetInputPortWidth(S,0));//Work vector for input port
    ssSetDWorkDataType(  S, 0, SS_DOUBLE); /* use SS_DOUBLE if needed */

    ssSetDWorkWidth(     S, 1, ssGetNumDiscStates(S));//Work vector for states * may need to add actuator states!
    ssSetDWorkDataType(  S, 1, SS_DOUBLE);

	ssSetDWorkWidth(     S, 2, ssGetNumDiscStates(S));	//Work vector derivatives
    ssSetDWorkDataType(  S, 2, SS_DOUBLE);

	ssSetDWorkWidth(     S, 3, ssGetOutputPortWidth(S,1));//Work vector for flight controls outputs
    ssSetDWorkDataType(  S, 3, SS_DOUBLE);

	ssSetDWorkWidth(     S, 4, ssGetOutputPortWidth(S,2));//Work vector for propulsion outputs
    ssSetDWorkDataType(  S, 4, SS_DOUBLE);

	ssSetDWorkWidth(     S, 5, ssGetOutputPortWidth(S,3));//Work vector for calculated outputs
    ssSetDWorkDataType(  S, 5, SS_DOUBLE);	
    
	ssSetNumPWork(S, 1); // reserve element in the pointers vector
                         // to store a C++ object

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
    //ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
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
		JSBSimInterface *JII = (JSBSimInterface *) ssGetPWork(S)[0];   // retrieve C++ object pointers vector
        
        // Check if a script file is given in Simulink.
        // If not, initialize a aircraft
        // The RunIC() command is run in either OpenScript or Copy_Init_To_JSBSim.
         
        SGPath initfile;
        initfile = "";
        
        char buf[128];
        mwSize buflen;
        buflen = mxGetNumberOfElements(script_name) + 1;
        mxGetString(script_name, buf, buflen);
        std::string script = "";
        script = std::string(buf);
        mexPrintf("Script input: %s \n", script.c_str());
        if(script_bool && JII->OpenScript(SGPath(script), delta_t, initfile)){ //check both that script is indicated and that it works to open the script 
            mexPrintf("Using Scripts! \n");
        }
        
        char buf2[128];
        mwSize buflen2;        
        buflen2 = mxGetNumberOfElements(reset_name) +1; 
        mxGetString(reset_name, buf2, buflen2); 
        std::string reset = "";
        reset = std::string(buf2);
        mexPrintf("Reset file: '%s' .\n", reset.c_str());
        if(!script_bool){
            mexPrintf("\n");
            mexPrintf("JSBSim S-Function is initializing...\n");
            mexPrintf("\n");
            mexPrintf("Note: For Aircraft with integrators in the FCS, please type 'clearSF' to completely reset S-Function.\n");
            mexPrintf("\n");
            char buf[128];
            mwSize buflen;
            buflen = mxGetNumberOfElements(ac_name) + 1;
            mxGetString(ac_name, buf, buflen);
            std::string aircraft = "";
            aircraft = std::string(buf);
            if(!JII->Open(aircraft)){
                mexPrintf("Aircraft file could not be loaded.\n");
                mexPrintf("\n");
            }

            else{
                mexPrintf("'%s' Aircraft File has been successfully loaded!\n", aircraft.c_str());
                JII->LoadIC(SGPath(reset)); 
            }
                //PrintCatalog(); Print AC Catalog when ac model loads
            /*
            real_T *x = ssGetRealDiscStates(S); 		
            int j;
            // Initialize the state vector 
            for (j = 0; j < ssGetNumDiscStates(S); j++)
            {
                x[j] = mxGetPr(ssGetSFcnParam(S, 1))[j];
            }
            */
            
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
	   
	  real_T *x2 = ssGetRealDiscStates(S);

		x2[0] = u_fps;
		x2[1] = v_fps;
		x2[2] = w_fps;
		x2[3] = p_radsec;
		x2[4] = q_radsec;
		x2[5] = r_radsec;
		x2[6] = h_sl_ft;
		x2[7] = long_gc_deg;
		x2[8] = lat_gc_deg;
		x2[9] = phi_rad;
		x2[10] = theta_rad;
		x2[11] = psi_rad;
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
    
	//real_T *x = ssGetContStates(S);
    real_T *x2 = ssGetRealDiscStates(S);  
    
    // These are vectors with the length of the output port
    real_T *y1 = ssGetOutputPortRealSignal(S, 0);
	real_T *y2 = ssGetOutputPortRealSignal(S, 1);
	real_T *y3 = ssGetOutputPortRealSignal(S, 2);
	real_T *y4 = ssGetOutputPortRealSignal(S, 3);
    //real_T *y5 = ssGetOutputPortRealSignal(S, 4);
    
	//real_T *y5 = ssGetOutputPortRealSignal(S, 4);
	double *w3 =  (double *) ssGetDWork(S,3); 
	double *w4 = (double *) ssGetDWork(S,4);
	double *w5 = (double *) ssGetDWork(S,5);
    //double *w7 = (double *) ssGetDWork(S,7);
    
    int i;
/*
	for (i = 0; i < ssGetNumContStates(S); i++)
	 {
		y1[i] = x[i]; // outputs are the states 
	 }
*/
	for (i = 0; i < ssGetNumDiscStates(S); i++)
	 {
		y1[i] = x2[i]; /* outputs are the states */
	 }

	for (i = 0; i < ssGetDWorkWidth(S,3); i++)
	 {
		y2[i] = w3[i]; // outputs are the flight control outputs 
	 }

	for (i = 0; i < ssGetDWorkWidth(S,4); i++)
	 {
		y3[i] = w4[i]; // outputs are the propulsion outputs 
	 }
	for (i = 0; i < ssGetDWorkWidth(S,5); i++)
	 {
		y4[i] = w5[i]; // outputs are the calculated pilot outputs 
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

	 JSBSimInterface *JII = (JSBSimInterface *) ssGetPWork(S)[0];   // retrieve C++ object pointers vector
     
	 real_T *x2 = ssGetRealDiscStates(S);
	 //real_T *x = ssGetContStates(S);
	 InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0); // Input 
     throttle = *uPtrs[0];
     aileron = *uPtrs[1];
     elevator = *uPtrs[2];
     rudder = *uPtrs[3];
     mixture = *uPtrs[4];
     runset = *uPtrs[5];
     flaps = *uPtrs[6];
     gear = *uPtrs[7];
          
	 //double *derivatives = (double *) ssGetDWork(S,2);

     // These point to the output 
	 double *inputs   = (double *) ssGetDWork(S,0);
	 double *states = (double *) ssGetDWork(S,1);
	 double *controls = (double *) ssGetDWork(S,3);
	 double *pilot = (double *) ssGetDWork(S,5);
     
	 int k;
     
	 for (k=0; k < ssGetDWorkWidth(S,0); k++) {
        inputs[k] = (*uPtrs[k]);
     }
	 for (k=0; k < ssGetDWorkWidth(S,1); k++) {
        states[k] = x2[k];
     }
     
     double ctrl_vec[8] = {throttle, aileron, elevator, rudder, mixture, runset, flaps, gear};

     if(!script_bool){            
         if(JII->Copy_Controls_To_JSBSim(ctrl_vec)){
             //mexPrintf("Success to copy controls \n");
         }    
     }    

     JII->Update();    
     
     JII->Copy_States_From_JSBSim(states); 
     JII->Copy_Pilot_From_JSBSim(pilot); 
     JII->Copy_Control_From_JSBSim(controls); 
     
	// From previous code, why is this done twice? 
	for (k=0; k < ssGetDWorkWidth(S,1); k++) {
        x2[k] = states[k];
    } 
	/* for (k=0; k < ssGetDWorkWidth(S,2); k++) {
        dx[k] = derivatives[k];
    }*/
	//UNUSED_ARG(tid);
  }
#endif /* MDL_UPDATE */


// NOT USED! 
#undef MDL_DERIVATIVES  /* Change to #undef to remove function */
#if defined(MDL_DERIVATIVES)
  /* Function: mdlDerivatives =================================================
   * Abstract:
   *    In this function, you compute the S-function block's derivatives.
   *    The derivatives are placed in the derivative vector, ssGetdX(S).
   */
  static void mdlDerivatives(SimStruct *S)
  {
	  real_T *dx = ssGetdX(S);
	  double *w2 =  (double *) ssGetDWork(S,2);
	  for (int i = 0; i < ssGetDWorkWidth(S,2); i++)
	 {
		dx[i] = w2[i]; // outputs are the flight control outputs 
	 }

  }
#endif /* MDL_DERIVATIVES */



/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.  For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
	
	JSBSimInterface *JII = (JSBSimInterface *) ssGetPWork(S)[0];   // retrieve C++ object pointers vector
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
