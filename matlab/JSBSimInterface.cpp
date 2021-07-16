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


#include "JSBSimInterface.h"
#include <models/FGAircraft.h>
#include <models/FGAccelerations.h>
#include <math/FGQuaternion.h>

/* 2021-07-08 compiles with JSBSim 1.1.6
 */

JSBSimInterface::JSBSimInterface(void)
{
	_ac_model_loaded = false;
	fdmExec = new FGFDMExec;
	propagate = fdmExec->GetPropagate().get();
	accel = fdmExec->GetAccelerations().get();
	auxiliary = fdmExec->GetAuxiliary().get();
	aerodynamics = fdmExec->GetAerodynamics().get();
	propulsion = fdmExec->GetPropulsion().get();
	fcs = fdmExec->GetFCS().get();
	ic = new FGInitialCondition(fdmExec);
	//verbosityLevel = JSBSimInterface::eSilent;
}
JSBSimInterface::JSBSimInterface(double dt)
{
  _ac_model_loaded = false;
	fdmExec = new FGFDMExec;
  fdmExec->Setdt(dt);
  mexPrintf("Simulation dt set to %f\n",fdmExec->GetDeltaT());
  propagate = fdmExec->GetPropagate().get();
  accel = fdmExec->GetAccelerations().get();
  auxiliary = fdmExec->GetAuxiliary().get();
  aerodynamics = fdmExec->GetAerodynamics().get();
  propulsion = fdmExec->GetPropulsion().get();
  fcs = fdmExec->GetFCS().get();
  ic = new FGInitialCondition(fdmExec);
  //verbosityLevel = JSBSimInterface::eSilent;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
JSBSimInterface::~JSBSimInterface(void)
{
    delete fdmExec;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Open(const string& acName)
{
  if (!fdmExec->GetAircraft()->GetAircraftName().empty())
  {
    return false;
  }

    mexPrintf("\tSetting up JSBSim with standard 'aircraft', 'engine', and 'system' paths.\n");  
    fdmExec->SetAircraftPath (SGPath("aircraft")); 
    fdmExec->SetEnginePath   (SGPath("engine"));
    fdmExec->SetSystemsPath  (SGPath("systems"));

    mexPrintf("\tLoading aircraft '%s' ...\n",acName.c_str());

    if ( ! fdmExec->LoadModel(SGPath("aircraft"),
                               SGPath("engine"),
                               SGPath("systems"),
                               acName)) {
      mexPrintf("\tERROR: JSBSim could not load the aircraft model.\n");
    return false;
    }
  _ac_model_loaded = true;
  // Print AC name
    mexPrintf("\tModel %s loaded.\n", fdmExec->GetModelName().c_str() );

//***********************************************************************
  // populate aircraft catalog
  catalog = fdmExec->GetPropertyCatalog();

//   if ( verbosityLevel == eVeryVerbose )
//   {
//     for (unsigned i=0; i<catalog.size(); i++)
//       mexPrintf("%s\n",catalog[i].c_str());
//   }
//***********************************************************************/

  return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Open(const mxArray *prhs)
{
	if (!fdmExec) return false;

	char buf[128];
	mwSize buflen;
	buflen = std::min<size_t>(mxGetNumberOfElements(prhs) + 1, 128);
	mxGetString(prhs, buf, buflen);
	string acName = string(buf);
    return Open(acName);

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::ResetToInitialCondition()
{
  //mexEvalString("clearSF");
  fdmExec->Setsim_time(0.0);
  fdmExec->ResetToInitialConditions(0);
  fdmExec->GetIC()->ResetIC(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  //delete fdmExec;   
  mexPrintf("Aircraft states are reset to IC\n");
  return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::GetPropertyValue(const mxArray *prhs1, double& value)
{
	if (!fdmExec) return false;
	if (!IsAircraftLoaded()) return false;

	char buf[128];
	mwSize buflen;
	buflen = std::min<size_t>(mxGetNumberOfElements(prhs1) + 1, 128);
	mxGetString(prhs1, buf, buflen);
	const string prop = string(buf);

	if (!EasyGetValue(prop, value)) // first check if an easy way of setting is implemented
	{
		if ( !QueryJSBSimProperty(prop) )
		{
				mexPrintf("\tERROR: JSBSim could not find the property '%s' in the aircraft catalog.\n",prop.c_str());
			return false;
		}
		value = fdmExec->GetPropertyValue(prop);
	}
	return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::SetPropertyValue(const mxArray *prhs1, const mxArray *prhs2)
{
	if (!fdmExec) return false;
	if (!IsAircraftLoaded()) return false;

	char buf[128];
	mwSize buflen;
	buflen = std::min<size_t>(mxGetNumberOfElements(prhs1) + 1, 128);
	mxGetString(prhs1, buf, buflen);
	const string prop = string(buf);
	double value = *mxGetPr(prhs2);

	if (!EasySetValue(prop,value)) // first check if an easy way of setting is implemented
	{
		if ( !QueryJSBSimProperty(prop) ) // then try to set the full-path property, e.g. '/fcs/elevator-cmd-norm'
		{
				mexPrintf("\tERROR: JSBSim could not find the property '%s' in the aircraft catalog.\n",prop.c_str());
			return false;
		}
		fdmExec->SetPropertyValue( prop, value );
	}
	return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::SetPropertyValue(const string& prop, const double value)
{
		mxArray *p1 = mxCreateString(prop.c_str());
		mxArray *p2 = mxCreateDoubleMatrix(1, 1, mxREAL);
		*mxGetPr(p2) = value;
		return SetPropertyValue(p1,p2);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::GetPropertyValue(const string& prop, double& value)
{
		mxArray *p1 = mxCreateString(prop.c_str());
		return GetPropertyValue(p1,value);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::EasySetValue(const string& prop, const double value)
{
	if (prop == "set-running")
	{
		bool isrunning = false;
		if (value > 0) isrunning = true;
		for(unsigned i=0;i<fdmExec->GetPropulsion()->GetNumEngines();i++)
			fdmExec->GetPropulsion()->GetEngine(i)->SetRunning(isrunning);
		fdmExec->GetPropulsion()->GetSteadyState();
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: engine(s) running = %d\n",(int)isrunning);
		return true;
	}
	else if (prop == "u-fps")
	{
		propagate->SetUVW(1,value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: true flight speed (ft/s) = %f\n",auxiliary->GetVt());
		return true;
	}
	else if (prop == "v-fps")
	{
		propagate->SetUVW(2,value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: true flight speed (ft/s) = %f\n",auxiliary->GetVt());
		return true;
	}
	else if (prop == "w-fps")
	{
		propagate->SetUVW(3,value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: true flight speed (ft/s) = %f\n",auxiliary->GetVt());
		return true;
	}
	else if (prop == "p-rad_sec")
	{
		propagate->SetPQR(1,value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: roll rate (rad/s) = %f\n",propagate->GetPQR(1));
		return true;
	}
	else if (prop == "q-rad_sec")
	{
		propagate->SetPQR(2,value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: pitch rate (rad/s) = %f\n",propagate->GetPQR(2));
		return true;
	}
	else if (prop == "r-rad_sec")
	{
		propagate->SetPQR(3,value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: yaw rate (rad/s) = %f\n",propagate->GetPQR(3));
		return true;
	}
	else if (prop == "h-sl-ft")
	{
		propagate->SetAltitudeASL(value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: altitude over sea level (mt) = %f\n",propagate->GetAltitudeASLmeters());
		return true;
	}
	else if (prop == "long-gc-deg")
	{
		propagate->SetLongitudeDeg(value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: geocentric longitude (deg) = %f\n",propagate->GetLongitudeDeg());
		return true;
	}
	else if (prop == "lat-gc-deg")
	{
		propagate->SetLatitudeDeg(value);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: geocentric latitude (deg) = %f\n",propagate->GetLatitudeDeg());
		return true;
	}
	else if (prop == "phi-rad")
	{
		FGQuaternion Quat( value, propagate->GetEuler(2), propagate->GetEuler(3) );
		Quat.Normalize();
		FGPropagate::VehicleState vstate = propagate->GetVState();
		vstate.qAttitudeLocal = Quat;
		propagate->SetVState(vstate);
		propagate->Run(false); // vVel => gamma
		auxiliary->Run(false); // alpha, beta, gamma
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: phi -> quaternion = (%f,%f,%f,%f)\n",
		//		propagate->GetVState().qAttitudeLocal(1),propagate->GetVState().qAttitudeLocal(2),
		//		propagate->GetVState().qAttitudeLocal(3),propagate->GetVState().qAttitudeLocal(4));
		return true;
	}
	else if (prop == "theta-rad")
	{
		FGQuaternion Quat( propagate->GetEuler(1), value, propagate->GetEuler(3) );
		Quat.Normalize();
		FGPropagate::VehicleState vstate = propagate->GetVState();
		vstate.qAttitudeLocal = Quat;
		propagate->SetVState(vstate);
		propagate->Run(false); // vVel => gamma
		auxiliary->Run(false); // alpha, beta, gamma
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: theta -> quaternion = (%f,%f,%f,%f)\n",
		//		propagate->GetVState().qAttitudeLocal(1),propagate->GetVState().qAttitudeLocal(2),
		//		propagate->GetVState().qAttitudeLocal(3),propagate->GetVState().qAttitudeLocal(4));
		return true;
	}
	else if (prop == "psi-rad")
	{
		FGQuaternion Quat( propagate->GetEuler(1), propagate->GetEuler(2), value );
		Quat.Normalize();
		FGPropagate::VehicleState vstate = propagate->GetVState();
		vstate.qAttitudeLocal = Quat;
		propagate->SetVState(vstate);
		propagate->Run(false); // vVel => gamma
		auxiliary->Run(false); // alpha, beta, gamma
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: psi -> quaternion = (%f,%f,%f,%f)\n",
		//		propagate->GetVState().qAttitudeLocal(1),propagate->GetVState().qAttitudeLocal(2),
		//		propagate->GetVState().qAttitudeLocal(3),propagate->GetVState().qAttitudeLocal(4));
		return true;
	}
	else if (prop == "elevator-cmd-norm")
	{
		fdmExec->GetFCS()->SetDeCmd(value);
		fdmExec->GetFCS()->Run(false);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: elevator pos (deg) = %f\n",fdmExec->GetFCS()->GetDePos()*180./M_PI);
		return true;
	}
	else if (prop == "aileron-cmd-norm")
	{
		fdmExec->GetFCS()->SetDaCmd(value);
		fdmExec->GetFCS()->Run(false);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: right aileron pos (deg) = %f\n",fdmExec->GetFCS()->GetDaRPos()*180./M_PI);
		return true;
	}
	else if (prop == "rudder-cmd-norm")
	{
		fdmExec->GetFCS()->SetDrCmd(value);
		fdmExec->GetFCS()->Run(false);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: rudder pos (deg) = %f\n",fdmExec->GetFCS()->GetDrPos()*180./M_PI);
		return true;
	}
	return false;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
double JSBSimInterface::EasyGetValue(const string& prop, double& value)
{
  if (prop == "set-running")
  {
    //if ( verbosityLevel == eVeryVerbose ){
    //	mexPrintf("\tEasy-Get: engine(s) running = %i\n",fdmExec->GetPropulsion()->GetEngine(0)->GetRunning());
    //}
    value = (double)(fdmExec->GetPropulsion()->GetEngine(0)->GetRunning());
    return true;
  }
  else if (prop == "u-fps")
  {
//     if ( verbosityLevel == eVeryVerbose ){
//     	mexPrintf("\tEasy-get: propagate->GetUVW(1);= %f\n", propagate->GetUVW(1));
//     }
    value = propagate->GetUVW(1);
    return true;
  }
  else if (prop == "v-fps")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: propagate->GetUVW(2);= %f\n", propagate->GetUVW(2));
//     }
    value = propagate->GetUVW(2);
    return true;
  }
  else if (prop == "w-fps")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: propagate->GetUVW(3);= %f\n", propagate->GetUVW(3));
//     }
    value = propagate->GetUVW(3);
    return true;
  }
  else if (prop == "p-rad_sec")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: roll rate (rad/s) = %f\n",propagate->GetPQR(1));
//     }
    value = propagate->GetPQR(1);
    return true;
  }
  else if (prop == "q-rad_sec")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: pitch rate (rad/s) = %f\n",propagate->GetPQR(2));
//     }
    value = propagate->GetPQR(2);
    return true;
  }
  else if (prop == "r-rad_sec")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: yaw rate (rad/s) = %f\n",propagate->GetPQR(3));
//     }
    value = propagate->GetPQR(3);
    return true;
  }
  else if (prop == "h-sl-ft")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: altitude over sea level (mt) = %f\n",propagate->GetAltitudeASLmeters());
//     }
    value = propagate->GetAltitudeASLmeters();
    return true;
  }
  else if (prop == "long-gc-deg")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: geocentric longitude (deg) = %f\n",propagate->GetLongitudeDeg());
//     }
    value = propagate->GetLongitudeDeg();
    return true;
  }
  else if (prop == "lat-gc-deg")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: geocentric latitude (deg) = %f\n",propagate->GetLatitudeDeg());
//     }
    value = propagate->GetLatitudeDeg();
    return true;
  }
  else if (prop == "phi-rad")
  {
    FGColumnVector3 euler = propagate->GetVState().qAttitudeLocal.GetEuler();
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: phi-rad = %f\n",euler.Entry(1));
//     }
    value = euler.Entry(1);
    return true;
  }
  else if (prop == "theta-rad")
  {
    FGColumnVector3 euler = propagate->GetVState().qAttitudeLocal.GetEuler();
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: theta-rad = %f\n",euler.Entry(2));
//     }
    value = euler.Entry(2);
    return true;
  }
  else if (prop == "psi-rad")
  {
    FGColumnVector3 euler = propagate->GetVState().qAttitudeLocal.GetEuler();
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: psi-rad = %f\n",euler.Entry(3));
//     }
    value = euler.Entry(3);
    return true;
  }
  else if (prop == "elevator-pos-rad")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: elevator pos (rad) = %f\n",fdmExec->GetFCS()->GetDePos());
//     }
    value = fdmExec->GetFCS()->GetDePos();
    return true;
  }
  else if (prop == "aileron-pos-rad")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: right aileron pos (rad) = %f\n",fdmExec->GetFCS()->GetDaRPos());
//     }
    value = fdmExec->GetFCS()->GetDaRPos();
    return true;
  }
  else if (prop == "rudder-pos-rad")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-set: rudder pos (deg) = %f\n",fdmExec->GetFCS()->GetDrPos());
//     }
    value = fdmExec->GetFCS()->GetDrPos();
    return true;
  }
  return false;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::QueryJSBSimProperty(const string& prop)
{
	// mexPrintf("catalog size: %d\n",catalog.size());
	for (unsigned i=0; i<catalog.size(); i++)
	{
		//mexPrintf("__%s__\n",catalog[i].c_str());
		if (catalog[i].find(prop) != std::string::npos) return true;
	}
	return false;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void JSBSimInterface::PrintCatalog()
{
    const std::string& name = fdmExec->GetAircraft()->GetAircraftName();
    mexPrintf("-- Property catalog for current aircraft ('%s'):\n", name.c_str());
    for (unsigned i=0; i<catalog.size(); i++)
        mexPrintf("%s\n",catalog[i].c_str());
    mexPrintf("-- end of catalog\n");
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void JSBSimInterface::Update()
{
    fdmExec->Run();
    
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void JSBSimInterface::LoadIC(SGPath ResetName)
{
    auto IC = fdmExec->GetIC(); 
    if(!IC->Load(ResetName)){
        mexPrintf("Could not load reset file \n");
    }
    fdmExec->RunIC();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void JSBSimInterface::RunIC()
{
    fdmExec->RunIC();        
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Copy_Controls_To_JSBSim(double controls[]){
    // throttle, aileron, elevator, rudder, mixture, runset, flap, gear
    // TODO: error handling if controls is not correct size. 
    if(!fdmExec) return false; 
    /*
     if(SetPropertyValue("fcs/throttle-cmd-norm", controls[0])){
         mexPrintf("1\n");
     }
     if(SetPropertyValue("aileron-cmd-norm", controls[1])){
         mexPrintf("2\n");
     }
     if(SetPropertyValue("elevator-cmd-norm", controls[2])){
         mexPrintf("3\n");
     }
     if(SetPropertyValue("rudder-cmd-norm", controls[3])){
         mexPrintf("4\n");
     }
     if(SetPropertyValue("fcs/mixture-cmd-norm", controls[4])){
         mexPrintf("5\n");
     }
     if(SetPropertyValue("set-running", controls[5])){
         mexPrintf("6\n");
     }
     if(SetPropertyValue("fcs/flap-cmd-norm", controls[6])){
         mexPrintf("7\n");
     }
     if(SetPropertyValue("gear/gear-cmd-norm", controls[7])){
         mexPrintf("8\n");
     }   */  
    
     SetPropertyValue("fcs/throttle-cmd-norm", controls[0]);
     fcs->SetDaCmd(controls[1]);
     fcs->SetDeCmd(controls[2]);
     fcs->SetDrCmd(controls[3]);
     SetPropertyValue("fcs/mixture-cmd-norm", controls[4]);
     SetPropertyValue("set-running", controls[5]);
     fcs->SetDfCmd(controls[6]);
     SetPropertyValue("gear/gear-cmd-norm", controls[7]); 
     
     return true; 
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Copy_Init_To_JSBSim(double init_values[]){
    if(!fdmExec) return false; 
    
    /*
     if(SetPropertyValue("u-fps", init_values[0])) {
         mexPrintf("1");
     }
     if(SetPropertyValue("v-fps", init_values[1])){
         mexPrintf("2");
     }         
     if(SetPropertyValue("w-fps", init_values[2])){
         mexPrintf("3");
     }         
     if(SetPropertyValue("p-rad_sec", init_values[3])){
         mexPrintf("4");
     }         
     if(SetPropertyValue("q-rad_sec", init_values[4])){
         mexPrintf("5");
     }         
     if(SetPropertyValue("r-rad_sec", init_values[5])){
         mexPrintf("6");
     }         
     if(SetPropertyValue("h-sl-ft", init_values[6])){
         mexPrintf("7");
     }         
     if(SetPropertyValue("long-gc-deg", init_values[7])){
         mexPrintf("8");
     }         
     if(SetPropertyValue("lat-gc-deg", init_values[8])){
         mexPrintf("9");
     }         
     if(SetPropertyValue("phi-rad", init_values[9])){
         mexPrintf("10");
     }         
     if(SetPropertyValue("lat-gc-deg", init_values[10])){
         mexPrintf("11");
     } */        

     SetPropertyValue("u-fps", init_values[0]);
     SetPropertyValue("v-fps", init_values[1]);
     SetPropertyValue("w-fps", init_values[2]);
     SetPropertyValue("p-rad_sec", init_values[3]);
     SetPropertyValue("q-rad_sec", init_values[4]);
     SetPropertyValue("r-rad_sec", init_values[5]);
     SetPropertyValue("h-sl-ft", init_values[6]);
     SetPropertyValue("long-gc-deg", init_values[7]);
     SetPropertyValue("lat-gc-deg", init_values[8]); 
     SetPropertyValue("phi-rad", init_values[9]); 
     SetPropertyValue("lat-gc-deg", init_values[10]); 
     
     fdmExec->RunIC();
    return true; 
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::OpenScript(const SGPath& script, double delta_t, const SGPath& initfile){
    
    fdmExec->SetAircraftPath (SGPath("aircraft")); // remove argument (rootDir + "aircraft")    
    fdmExec->SetEnginePath   (SGPath("engine"));
    fdmExec->SetSystemsPath  (SGPath("systems"));
    
    if(!fdmExec->LoadScript(script, delta_t, initfile)){
        mexPrintf("Could not open a script \n"); 
        return false;
    }
    fdmExec->RunIC();
    return true;
    
    //Script = std::make_shared<FGScript>(this);
    //return Script->LoadScript(GetFullPath(script), delta_t, initfile);
    
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Copy_States_From_JSBSim(double *state_array){
    // [u-fps v-fps w-fps p-radsec q-radsec r-radsec h-sl-ft long-gc-deg lat-gc-deg phi-rad theta-rad psi-rad]
    
    // Possibly add some error handling here? 
    
    state_array[0] = propagate->GetUVW(1); 
    state_array[1] = propagate->GetUVW(2); 
    state_array[2] = propagate->GetUVW(3); 
    
    state_array[3] = propagate->GetPQR(1);
    state_array[4] = propagate->GetPQR(2);
    state_array[5] = propagate->GetPQR(3);
    
    state_array[6] = propagate->GetAltitudeASLmeters();
    state_array[7] = propagate->GetLongitudeDeg();
    state_array[8] = propagate->GetLatitudeDeg();
    
    FGColumnVector3 euler = propagate->GetVState().qAttitudeLocal.GetEuler();
    state_array[9] = euler.Entry(1);
    state_array[10] = euler.Entry(2);
    state_array[11] = euler.Entry(3);
    
    return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Copy_Pilot_From_JSBSim(double *state_array){
    // [N alpha alphadot beta betadot vc-fps vc-kts vt-fps vg-fps mach climb-rate qdyn-psf]
    
    // Possibly add some error handling here? 
    
    state_array[0] = auxiliary->GetNlf(); // Normal load factor  
    
    state_array[1] = auxiliary->Getalpha(); 
    state_array[2] = auxiliary->Getadot(); 
    state_array[3] = auxiliary->Getbeta();
    state_array[4] = auxiliary->Getbdot();
    
    state_array[5] = auxiliary->GetVcalibratedFPS(); //valibrated airspeed, fps
    state_array[6] = auxiliary->GetVcalibratedKTS(); //calibrated airspeed, knots
    state_array[7] = auxiliary->GetVtrueFPS(); //true airspeed, fps
    state_array[8] = auxiliary->GetVground(); //ground speed, fps    
    state_array[9] = auxiliary->GetMach();
    
    state_array[10] = propagate->Gethdot();    
    state_array[11] = auxiliary->Getqbar(); 
    
    state_array[12] = fcs->GetDeCmd(); //elevator cmd norm 
    return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Copy_Control_From_JSBSim(double *state_array){
    // [thr-pos-norm left-ail-pos-rad right-ail-pos-rad el-pos-rad rud-pos-rad flap-pos-norm ]
    // speedbrake-pos-rad spoiler-pos-rad gear-pos-norm]
    // Possibly add some error handling here? 
    
    state_array[0] = fcs->GetThrottlePos(0); //should be an input command, NEEDS FIX   
    
    state_array[1] = fcs->GetDaLPos(); //left aileron 
    state_array[2] = fcs->GetDaRPos(); //right aileron
    state_array[3] = fcs->GetDePos(); //elevator
    state_array[4] = fcs->GetDrPos(); //rudder
    state_array[5] = fcs->GetDfPos(); //flaps
    state_array[6] = fcs->GetDsbPos(); //speedbrake
    state_array[7] = fcs->GetDspPos(); // spoiler
    
    state_array[8] = fcs->GetGearPos(); //gear
    
    
    return true;
}
