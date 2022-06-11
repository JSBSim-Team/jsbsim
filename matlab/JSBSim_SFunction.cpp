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

#include "input_output/FGPropertyManager.h"
#include "input_output/FGXMLParse.h"
#include "input_output/FGXMLFileRead.h"

#include <models/FGPropagate.h>
#include <models/FGAuxiliary.h>
#include <models/FGFCS.h>

#include "JSBSimInterface.h"

// Name of JSBSim aircraft model file to load
#define AIRCRAFT_NAME_PARAM     0

#define ac_name			        ssGetSFcnParam(S, AIRCRAFT_NAME_PARAM)

// Simulation discrete time step
#define TIME_STEP_PARAM         1

#define delta_t		            mxGetPr(ssGetSFcnParam(S, TIME_STEP_PARAM))[0]

// Parameter for using the supplied script (1 to use, 0 to not use) and
// for enabling control input to script (1 to enable, 0 to disable)
#define USE_SCRIPT_PARAM        2

#define use_script	            mxGetPr(ssGetSFcnParam(S, USE_SCRIPT_PARAM))[0]

// The file path to the script to run
#define SCRIPT_FILE_PARAM       3

#define script_name             ssGetSFcnParam(S, SCRIPT_FILE_PARAM)

// Initial condition parameters for resetting the aircraft, from aircraft type directory
#define RESET_FILE_PARAM        4

#define reset_name              ssGetSFcnParam(S, RESET_FILE_PARAM)

// The file path to the input/output configuration for the aircraft.
#define IO_CONFIG_FILE_PARAM    5

#define io_config_file_name     ssGetSFcnParam(S, IO_CONFIG_FILE_PARAM)

#define NUM_PARAMS              6

int numOutputs;
int inputSize;
int weatherInputSize;
bool useWeather;

// Helper function for getting strings from mxArray objects.
std::string getMxArrayString(const mxArray* mxArrayStr) {

    mwSize strLen = mxGetNumberOfElements(mxArrayStr) + 1;
    char* strBuf = (char*) malloc(strLen * sizeof(char));
    mxGetString(mxArrayStr, strBuf, strLen); 
    std::string str = std::string(strBuf);
    free(strBuf);
    return str;
}

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

#define MDL_CHECK_PARAMETERS   /* Change to #undef to remove function */
#if defined(MDL_CHECK_PARAMETERS)
static void mdlCheckParameters(SimStruct *S)
{

    if (ssGetSFcnParamsCount(S) != NUM_PARAMS) {
        ssSetErrorStatus(S,"JSBSim S-function must have 6 parameters.");
        return;
    }
    
    if (!mxIsChar(ac_name)) {
        ssSetErrorStatus(S, "Parameter 1 to JSBSim S-function must be a string.");
        return;
    }
    
    if (!mxIsNumeric(ssGetSFcnParam(S, TIME_STEP_PARAM)) || delta_t < 0) {
        ssSetErrorStatus(S, "Parameter 2 to JSBSim S-function must be a nonnegative number.");
        return;
    }
    
    if (!mxIsNumeric(ssGetSFcnParam(S, USE_SCRIPT_PARAM)) || !(use_script == 1 || use_script == 0)) {
        ssSetErrorStatus(S, "Parameter 3 to JSBSim S-function must be either 0 (disabled) or 1 (enabled).");
        return; 
    }
    
    if (!mxIsChar(script_name)) {
        ssSetErrorStatus(S, "Parameter 4 to JSBSim S-function must be a string.");
        return;
    }
    
    if (!mxIsChar(reset_name)) {
        ssSetErrorStatus(S, "Parameter 5 to JSBSim S-function must be a string.");
        return;
    }
    
    if (!mxIsChar(io_config_file_name)) {
        ssSetErrorStatus(S, "Parameter 6 to JSBSim S-function must be a string.");
        return;
    }
}
#endif /* MDL_CHECK_PARAMETERS */

#define MDL_PROCESS_PARAMETERS   /* Change to #undef to remove function */
#if defined(MDL_PROCESS_PARAMETERS)
static void mdlProcessParameters(SimStruct *S)
{

    if(ssGetErrorStatus(S) != NULL) return;

    // Get the user provided input/output config.
    std::string io_config_file = getMxArrayString(io_config_file_name);
    mexPrintf("I/O config input: %s \n", io_config_file.c_str());

    FGXMLFileRead XMLFileRead;
    Element* document = XMLFileRead.LoadXMLDocument(SGPath(io_config_file));

    // Make sure that the document is valid
    if (!document) {
        ssSetErrorStatus(S, "Input/Output configuration file cannot be read.\n");
    }

    // Check the XML file is a port config file.
    if (document->GetName() != std::string("s_function_config")) {
        ssSetErrorStatus(S, "XML file is not an Input/Output configuration file.\n");
    }

    // Check that there are input and outputs properties.
    Element* inputElement = document->FindElement("input");
    if (!inputElement) {
        ssSetErrorStatus(S, "Please define an <input> property for the I/O config file.\n");
    }
    Element* outputsElement = document->FindElement("outputs");
    if (!outputsElement) {
        ssSetErrorStatus(S, "Please define an <outputs> property for the I/O config file.\n");
    }

    // Check if there is atmosphere data for input.
    Element* weatherElement = document->FindElement("weather");
    useWeather = (bool) weatherElement;

    // Get necessary sizing data for the input/output ports.
    inputSize = inputElement->GetNumElements();
    numOutputs = outputsElement->GetNumElements();

    // Configure the input port.
    if (!useWeather) {
        if (!ssSetNumInputPorts(S, 1)) return;
        ssSetInputPortWidth(S, 0, inputSize);
    } else {
        if (!ssSetNumInputPorts(S, 2)) return;
        ssSetInputPortWidth(S, 0, inputSize);

        weatherInputSize = weatherElement->GetNumElements();
        ssSetInputPortWidth(S, 1, weatherInputSize);
    }

    // Configure the output port(s).
    if (!ssSetNumOutputPorts(S, numOutputs)) return;
    
    int i;
    Element* outputElement = outputsElement->FindElement("output");
    int outputSize;
    for (i = 0; i < numOutputs; i++) {
        outputSize = outputElement->GetNumElements("property");
        ssSetOutputPortWidth(S, i, outputSize);
        // Currently no support for setting the name of the output ports.
        // However, if this feature is supported by MATLAB in the future,
        // the output port's property of "name" in the XML filecan be used
        // to set the name.

        outputElement = outputsElement->FindNextElement("output");
    }   
}
#endif /* MDL_PROCESS_PARAMETERS */

/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */
static void mdlInitializeSizes(SimStruct *S)
{

    /* See sfuntmpl_doc.c for more details on the macros below */
    ssSetNumSFcnParams(S, NUM_PARAMS);  /* Number of expected parameter vectors*/ 
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
        mdlCheckParameters(S);
        mdlProcessParameters(S);
        if(ssGetErrorStatus(S) != NULL) return;
    } else {
        return;
    }

    // Create the work vectors.
    if(!ssSetNumDWork(   S, useWeather ? 2 + numOutputs : 1 + numOutputs)) return; //HW change 

    // Work vector for input port.
    ssSetDWorkWidth(     S, 0, ssGetInputPortWidth(S,0));
    ssSetDWorkDataType(  S, 0, SS_DOUBLE);

    if (useWeather) {
        ssSetDWorkWidth(     S, numOutputs + 1, ssGetInputPortWidth(S,1));
        ssSetDWorkDataType(  S, numOutputs + 1, SS_DOUBLE);
    }

    // Work vector(s) for output port(s).
    int i;
    for (i = 0; i < numOutputs; i++) {
        ssSetDWorkWidth(     S, i+1, ssGetOutputPortWidth(S,i));
        ssSetDWorkDataType(  S, i+1, SS_DOUBLE);
    }
	
    // Reserve element in the pointers vector to store the JSBSimInterface.
	ssSetNumPWork(S, 1);

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
    
    mexPrintf("\nJSBSim S-Function is initializing...\n\n");
    
    // Create new JSBSimInterface object and initialize it with delta_t and num_outputs.
    JSBSimInterface *JII = new JSBSimInterface(delta_t, numOutputs);
    ssGetPWork(S)[0] = (void *) JII;

    // Check if a script file is given in Simulink.
    // If not, initialize an aircraft
    if (use_script) {

        // Keep initfile as empty so that we can use the initialization settings from the script.
        // See bool FGScript::LoadScript(const SGPath&, double, const SGPath&) for details.
        SGPath initfile;
        initfile = "";

        // Get the user provided script.
        std::string script = getMxArrayString(script_name);
        mexPrintf("Script input: %s \n", script.c_str());

        if (!JII->OpenScript(SGPath(script), delta_t, initfile)) {
            ssSetErrorStatus(S, "Flight script could not be loaded.\n");
            return;
        }
    } else {

        // Open the supplied aircraft file.
        std::string aircraft = getMxArrayString(ac_name);
        if (!JII->OpenAircraft(aircraft)){
            ssSetErrorStatus(S, "Aircraft file could not be loaded.\n");
            return;
        }
        mexPrintf("'%s' Aircraft File has been successfully loaded!\n", aircraft.c_str());

        // Get the user provided intialization settings for resetting the aircraft
        // to a default state.
        std::string reset = getMxArrayString(reset_name);
        mexPrintf("Reset file: '%s' .\n", reset.c_str());
        if (!JII->LoadIC(SGPath(reset))) {
            ssSetErrorStatus(S, "Reset file could not be loaded.\n");
            return;
        }
    }
    
    // Get the user provided input/output config.
    std::string io_config_file = getMxArrayString(io_config_file_name);

    FGXMLFileRead XMLFileRead;
    Element* document = XMLFileRead.LoadXMLDocument(SGPath(io_config_file));

    // Add input properties JSBSim should take in.
    int i;
    std::string prop;
    Element* inputElement = document->FindElement("input");
    Element* propElement = inputElement->FindElement("property");
    for (i = 0; i < inputSize; i++) {
        prop = propElement->GetDataLine();
        if (!JII->AddInputPropertyNode(prop)) {
            ssSetErrorStatus(S, "Could not add property from XML file to input port.\n"
                    "HINT: You can only use properties that exist and that are WRITE-only for this port.\n");
                return;
        }

        propElement = inputElement->FindNextElement("property");
    }

    // If the weather element exists, add input properties for atmosphere JSBSim should take in.
    if (useWeather) {
        Element* weatherElement = document->FindElement("weather");
        propElement = weatherElement->FindElement("property");
        for (i = 0; i < inputSize; i++) {
            prop = propElement->GetDataLine();
            if (!JII->AddWeatherPropertyNode(prop)) {
                ssSetErrorStatus(S, "Could not add property from XML file to weather port.\n"
                    "HINT: You can only use properties that exist and that are WRITE-only from \"atmosphere/\" for this port.\n");
                return;
            }

            propElement = weatherElement->FindNextElement("property");
        }
    }

    // Add output properties JSBSim will deliver to each output channel.
    int j;
    int outputSize;
    Element* outputsElement = document->FindElement("outputs");
    Element* outputElement = outputsElement->FindElement("output");
    for (i = 0; i < numOutputs; i++) {
        outputSize = outputElement->GetNumElements();
        propElement = outputElement->FindElement("property");

        for (j = 0; j < outputSize; j++) {
            prop = propElement->GetDataLine();
            if (!JII->AddOutputPropertyNode(prop, i)) {
                ssSetErrorStatus(S, "Could not add property from XML file to output port.\n"
                    "HINT: You can only use properties that exist and that are READ-only for this port.\n");
                return;
            }

            propElement = outputElement->FindNextElement("property");
        }

        outputElement = outputsElement->FindNextElement("output");
    }

    // Load initial conditions into the output work vectors.
    double *dWorkVector;
    for (i = 0; i < numOutputs; i++) {
        dWorkVector = (double *) ssGetDWork(S,i+1);
        if (!JII->CopyOutputsFromJSBSim(dWorkVector, i)) {
            ssSetErrorStatus(S, "Initial conditions could not be loaded into output.\n");
            return;
        }
    }
}
#endif /* MDL_INITIALIZE_CONDITIONS */

/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block.
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{ 
    
    real_T* output;
    double* dWorkVector;
    int i;
    int j;
    for (i = 0; i < numOutputs; i++) {
        output = ssGetOutputPortRealSignal(S, i);
        dWorkVector = (double*) ssGetDWork(S,i+1);
        for (j = 0; j < ssGetDWorkWidth(S, i+1); j++) {
            output[j] = dWorkVector[j];
        }
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

    JSBSimInterface* JII = (JSBSimInterface*) ssGetPWork(S)[0];

    InputRealPtrsType ctrlCmdInput = ssGetInputPortRealSignalPtrs(S, 0);
    double* dWorkCtrlCmdIn = (double*) ssGetDWork(S, 0);
    std::vector<double> ctrlVec(inputSize);
    int i;
    for (i = 0; i < inputSize; i++) {
        ctrlVec[i] = (double) *ctrlCmdInput[i];
        dWorkCtrlCmdIn[i] = *ctrlCmdInput[i];
    }
    
    if (!JII->CopyInputControlsToJSBSim(ctrlVec)) {
        ssSetErrorStatus(S, "Issue copying control inputs to JSBSim.\n");
        return;
    }

    if (useWeather) {
        InputRealPtrsType weatherInput = ssGetInputPortRealSignalPtrs(S, 1);
        double* dWorkWeatherIn = (double*) ssGetDWork(S, 1);
        std::vector<double> weatherVec(weatherInputSize);
        for (i = 0; i < weatherInputSize; i++) {
            weatherVec[i] = (double) *weatherInput[i];
            dWorkWeatherIn[i] = *weatherInput[i];
        }

        if (!JII->CopyInputWeatherToJSBSim(weatherVec)) {
            ssSetErrorStatus(S, "Issue copying weather inputs to JSBSim.\n");
            return;
        }
    }

    JII->Update();
    
    double *dWorkVector;
    for (i = 0; i < numOutputs; i++) {
        dWorkVector = (double *) ssGetDWork(S,i+1);
        JII->CopyOutputsFromJSBSim(dWorkVector, i);
    }
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
