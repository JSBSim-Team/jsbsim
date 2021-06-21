#include "JSBSimInterface.h"
#include <models/FGAircraft.h>
#include <models/FGAccelerations.h>
#include <math/FGQuaternion.h>

/* 2021-03-21 compiles with JSBSim 1.1.5
 * Compiles with JSBSim checkout a59596f8f0d4c7b4f7ba24e99997bc2071d6cb72
 */

JSBSimInterface::JSBSimInterface(FGFDMExec *fdmex)
{
	_ac_model_loaded = false;
	fdmExec = fdmex;
	propagate = fdmExec->GetPropagate().get();
	accel = fdmExec->GetAccelerations().get();
	accel->InitModel();
	auxiliary = fdmExec->GetAuxiliary().get();
	aerodynamics = fdmExec->GetAerodynamics().get();
	propulsion = fdmExec->GetPropulsion().get();
	fcs = fdmExec->GetFCS().get();
	ic = new FGInitialCondition(fdmExec);
	//verbosityLevel = JSBSimInterface::eSilent;
}
JSBSimInterface::JSBSimInterface(FGFDMExec *fdmex, double dt)
{
  _ac_model_loaded = false;
  fdmExec = fdmex;
  fdmExec->Setdt(dt);
  mexPrintf("Simulation dt set to %f\n",fdmExec->GetDeltaT());
  propagate = fdmExec->GetPropagate().get();
  accel = fdmExec->GetAccelerations().get();
  accel->InitModel();
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
	//fdmExec = 0L;
    //JSBSim::~FGFDMExec();
	//delete ic;
    delete fdmExec;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Open(const string& acName)
{
  string rootDir = ""; // In case JSBSim directory is somewhere else. 
  if ( fdmExec->GetAircraft()->GetAircraftName() != ""  )
  {
    return false;
  }

  // JSBSim stuff

    mexPrintf("\tSetting up JSBSim with standard 'aircraft', 'engine', and 'system' paths.\n");
  
    fdmExec->SetAircraftPath (SGPath("aircraft")); // remove argument (rootDir + "aircraft")    
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
	buflen = min(mxGetNumberOfElements(prhs) + 1, 128);
	mxGetString(prhs, buf, buflen);
	string acName = string(buf);


	//mexEvalString("plot(sin(0:.1:pi))");
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
	buflen = min(mxGetNumberOfElements(prhs1) + 1, 128);
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
	buflen = min(mxGetNumberOfElements(prhs1) + 1, 128);
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
		return 1;
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
		return 1;
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
		return 1;
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
		return 1;
	}
	else if (prop == "elevator-cmd-norm")
	{
		fdmExec->GetFCS()->SetDeCmd(value);
		fdmExec->GetFCS()->Run(false);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: elevator pos (deg) = %f\n",fdmExec->GetFCS()->GetDePos()*180./M_PI);
		return 1;
	}
	else if (prop == "aileron-cmd-norm")
	{
		fdmExec->GetFCS()->SetDaCmd(value);
		fdmExec->GetFCS()->Run(false);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: right aileron pos (deg) = %f\n",fdmExec->GetFCS()->GetDaRPos()*180./M_PI);
		return 1;
	}
	else if (prop == "rudder-cmd-norm")
	{
		fdmExec->GetFCS()->SetDrCmd(value);
		fdmExec->GetFCS()->Run(false);
		propagate->Run(false);
		auxiliary->Run(false);
		//if ( verbosityLevel == eVeryVerbose )
		//	mexPrintf("\tEasy-set: rudder pos (deg) = %f\n",fdmExec->GetFCS()->GetDrPos()*180./M_PI);
		return 1;
	}
	return 0;
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
    return 1;
  }
  else if (prop == "u-fps")
  {
//     if ( verbosityLevel == eVeryVerbose ){
//     	mexPrintf("\tEasy-get: propagate->GetUVW(1);= %f\n", propagate->GetUVW(1));
//     }
    value = propagate->GetUVW(1);
    return 1;
  }
  else if (prop == "v-fps")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: propagate->GetUVW(2);= %f\n", propagate->GetUVW(2));
//     }
    value = propagate->GetUVW(2);
    return 1;
  }
  else if (prop == "w-fps")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: propagate->GetUVW(3);= %f\n", propagate->GetUVW(3));
//     }
    value = propagate->GetUVW(3);
    return 1;
  }
  else if (prop == "p-rad_sec")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: roll rate (rad/s) = %f\n",propagate->GetPQR(1));
//     }
    value = propagate->GetPQR(1);
    return 1;
  }
  else if (prop == "q-rad_sec")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: pitch rate (rad/s) = %f\n",propagate->GetPQR(2));
//     }
    value = propagate->GetPQR(2);
    return 1;
  }
  else if (prop == "r-rad_sec")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: yaw rate (rad/s) = %f\n",propagate->GetPQR(3));
//     }
    value = propagate->GetPQR(3);
    return 1;
  }
  else if (prop == "h-sl-ft")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: altitude over sea level (mt) = %f\n",propagate->GetAltitudeASLmeters());
//     }
    value = propagate->GetAltitudeASLmeters();
    return 1;
  }
  else if (prop == "long-gc-deg")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: geocentric longitude (deg) = %f\n",propagate->GetLongitudeDeg());
//     }
    value = propagate->GetLongitudeDeg();
    return 1;
  }
  else if (prop == "lat-gc-deg")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: geocentric latitude (deg) = %f\n",propagate->GetLatitudeDeg());
//     }
    value = propagate->GetLatitudeDeg();
    return 1;
  }
  else if (prop == "phi-rad")
  {
    FGColumnVector3 euler = propagate->GetVState().qAttitudeLocal.GetEuler();
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: phi-rad = %f\n",euler.Entry(1));
//     }
    value = euler.Entry(1);
    return 1;
  }
  else if (prop == "theta-rad")
  {
    FGColumnVector3 euler = propagate->GetVState().qAttitudeLocal.GetEuler();
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: theta-rad = %f\n",euler.Entry(2));
//     }
    value = euler.Entry(2);
    return 1;
  }
  else if (prop == "psi-rad")
  {
    FGColumnVector3 euler = propagate->GetVState().qAttitudeLocal.GetEuler();
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: psi-rad = %f\n",euler.Entry(3));
//     }
    value = euler.Entry(3);
    return 1;
  }
  else if (prop == "elevator-pos-rad")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: elevator pos (rad) = %f\n",fdmExec->GetFCS()->GetDePos());
//     }
    value = fdmExec->GetFCS()->GetDePos();
    return 1;
  }
  else if (prop == "aileron-pos-rad")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-get: right aileron pos (rad) = %f\n",fdmExec->GetFCS()->GetDaRPos());
//     }
    value = fdmExec->GetFCS()->GetDaRPos();
    return 1;
  }
  else if (prop == "rudder-pos-rad")
  {
//     if ( verbosityLevel == eVeryVerbose )
//     {
//     	mexPrintf("\tEasy-set: rudder pos (deg) = %f\n",fdmExec->GetFCS()->GetDrPos());
//     }
    value = fdmExec->GetFCS()->GetDrPos();
    return 1;
  }
  return 0;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::QueryJSBSimProperty(const string& prop)
{
	// mexPrintf("catalog size: %d\n",catalog.size());
	for (unsigned i=0; i<catalog.size(); i++)
	{
		//mexPrintf("__%s__\n",catalog[i].c_str());
		if (catalog[i].find(prop) != std::string::npos) return 1;
	}
	return 0;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void JSBSimInterface::PrintCatalog()
{
    const std::string& name = fdmExec->GetAircraft()->GetAircraftName();
    mexPrintf("-- Property catalog for current aircraft ('%s'):\n", name.c_str());
    for (unsigned i=0; i<catalog.size(); i++)
        mexPrintf("%s\n",catalog[i].c_str());
    mexPrintf("-- end of catalog\n");
	return;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Init(const mxArray *prhs1)
{
	// Inspired by "refbook.c"
	// The argument prhs1 is pointer to a Matlab structure with two fields: name, value.
	// The Matlab user is forced to build such a structure first, then to pass it to the
	// mex-file. Example:
	//    >> ic(1).name = 'u'; ic(1).value = 80; (ft/s)
	//    >> ic(2).name = 'v'; ic(1).value =  0; (ft/s)
	//    >> ic(3).name = 'w'; ic(1).value =  0; (ft/s)
	//    >> ic(4).name = 'p'; ic(1).value =  0; (rad/s) % etc ...
	//    >> MexJSBSim('init', ic)

	//*************************************************
	// Set dt=0 first

	fdmExec->SuspendIntegration();
	
	//*************************************************

	bool success = 1;

	const char **fnames;       /* pointers to field names */
	mxArray    *tmp;
	int        ifield, nfields;
	mxClassID  *classIDflags;
	mwIndex    jstruct;
	mwSize     NStructElems;

    // get input arguments
    nfields = mxGetNumberOfFields(prhs1);
    NStructElems = mxGetNumberOfElements(prhs1);
    // allocate memory  for storing classIDflags
    classIDflags = (mxClassID*)mxCalloc(nfields, sizeof(mxClassID));
    //mexPrintf("Very Verbose: %f", eVeryVerbose); 

    // check empty field, proper data type, and data type consistency;
	// and get classID for each field (see "refbook.c")
    for(ifield=0; ifield<nfields; ifield++) 
	{
		for(jstruct = 0; jstruct < NStructElems; jstruct++) 
		{
			tmp = mxGetFieldByNumber(prhs1, jstruct, ifield);
			if(tmp == NULL) 
			{
				return false;
			} 
			if(jstruct==0) 
			{
				if( (!mxIsChar(tmp) && !mxIsNumeric(tmp)) || mxIsSparse(tmp)) 
				{
					return false;
				}
				classIDflags[ifield]=mxGetClassID(tmp); 
			} 
			else 
			{
				if (mxGetClassID(tmp) != classIDflags[ifield]) 
				{
					return false;
				} 
				else if(!mxIsChar(tmp) && 
					  ((mxIsComplex(tmp) || mxGetNumberOfElements(tmp)!=1)))
				{
					return false;
				}
			}
		}
    }
    /* allocate memory  for storing pointers */
    fnames = (const char **)mxCalloc(nfields, sizeof(*fnames));
    /* get field name pointers */
    for (ifield=0; ifield< nfields; ifield++)
	{
		fnames[ifield] = mxGetFieldNameByNumber(prhs1,ifield);
    }
	// At this point we have extracted from prhs1 the vector of 
	// field names fnames of nfields elements.
	// nfields is the number of fields in the passed Matlab struct (ic).
	// It may have more fields, but the first two must be "name" and "value"
	// The structure possesses generally a number of NStructElems elements.

    ndim = mxGetNumberOfDimensions(prhs1);
    dims = mxGetDimensions(prhs1);

	// loop on the element of the structure
	for (jstruct=0; jstruct<NStructElems; jstruct++) 
	{
		string prop = "";
		double value = -99.;

		// scan the fields
		// the first two must be "name" and "value"
		for(ifield=0; ifield<2; ifield++) // for(ifield=0; ifield<nfields; ifield++) // nfields=>2
		{
			tmp = mxGetFieldByNumber(prhs1,jstruct,ifield);
			if( mxIsChar(tmp) ) //  && (fnames[ifield]=="name") the "name" field
			{
				// mxSetCell(fout, jstruct, mxDuplicateArray(tmp));
				char buf[128];
				mwSize buflen;
				buflen = mxGetNumberOfElements(tmp) + 1;
				mxGetString(tmp, buf, buflen);
				prop = string(buf);
				//mexPrintf("field name: %s\n",prop.c_str());
			}
			else  // the "value" field
			{
				value = *mxGetPr(tmp);
				//mexPrintf("field value %f\n",value);
			}
		}
		//----------------------------------------------------
		// now we have a string in prop and a double in value
		// we got to set the property value accordingly
		//----------------------------------------------------
		//if ( verbosityLevel == eVeryVerbose )
			mexPrintf("Property name: '%s'; to be set to value: %f\n",prop.c_str(),value);

		//----------------------------------------------------
		// Note: the time step is set to zero at this point, so that all calls 
		//       to propagate->Run() will not advance the vehicle state in time
		//----------------------------------------------------
		// Now pass prop and value to the member function
		success = success && SetPropertyValue(prop,value); // EasySet called here

		mexPrintf("success '%d'; \n",(int)success);
    }
	// free memory
    mxFree(classIDflags);
	mxFree((void *)fnames);

	//---------------------------------------------------------------
	// see "FGInitialConditions.h"
	// NOTE:



	//mexPrintf("Vt = %f\n",fdmExec->GetAuxiliary()->GetVt());


	fdmExec->Run(); // is using RunIC(), derivatives are not calculated. Why do it this way? 
	fdmExec->ResumeIntegration(); // this is included if we do RunIC. Could possibly replace this later. 


	// Calculate state derivatives
	//fdmExec->GetAccelerations()->CalculatePQRdot();      // Angular rate derivative (should be done automatically)
	//fdmExec->GetAccelerations()->CalculateUVWdot();      // Translational rate derivative (should be done automatically)
	//fdmExec->GetPropagate()->CalculateQuatdot();     // Angular orientation derivative (should be done automatically)
	//fdmExec->GetPropagate()->CalculateLocationdot(); // Translational position derivative (should be done automatically)

	// FDMExec.GetAuxiliary()->Run();
	FGColumnVector3 euler_rates = auxiliary->GetEulerRates();
	FGColumnVector3 position_rates = propagate->GetUVW();

	/*
	 * dunno what is wrong here
	_udot = accel->GetUVWdot(1);
	_vdot = fdmExec->GetAccelerations()->GetUVWdot(2);
	_wdot = fdmExec->GetAccelerations()->GetUVWdot(3);
	_pdot = fdmExec->GetAccelerations()->GetPQRdot(1);
	_qdot = fdmExec->GetAccelerations()->GetPQRdot(2);
	_rdot = fdmExec->GetAccelerations()->GetPQRdot(3);
	*/
    /*
	FGQuaternion Quatdot = accel->GetQuaterniondot();
	_q1dot = Quatdot(1);
	_q2dot = Quatdot(2);
	_q3dot = Quatdot(3);
	_q4dot = Quatdot(4);
     */
	_phidot   = euler_rates(1);
	_thetadot = euler_rates(2);
	_psidot   = euler_rates(3);
	_xdot = position_rates(1);
	_ydot = position_rates(2);
	_zdot = position_rates(3);
	_hdot = propagate->Gethdot();
	_alphadot = auxiliary->Getadot();
	_betadot = auxiliary->Getbdot();

	if (!success)
	{
		return 0;
	}
	else
		return 1;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Init(const mxArray *prhs1, vector<double>& statedot)
{
	if (statedot.size() != 19) return 0;
	if (!Init(prhs1)){
		return 0;
	}
	statedot[ 0] = _udot;
	statedot[ 1] = _vdot;
	statedot[ 2] = _wdot;
	statedot[ 3] = _pdot;
	statedot[ 4] = _qdot;
	statedot[ 5] = _rdot;
	statedot[ 6] = _q1dot;
	statedot[ 7] = _q2dot;
	statedot[ 8] = _q3dot;
	statedot[ 9] = _q4dot;
	statedot[10] = _xdot;
	statedot[11] = _ydot;
	statedot[12] = _zdot;
	statedot[13] = _phidot;
	statedot[14] = _thetadot;
	statedot[15] = _psidot;
	statedot[16] = _hdot;
	statedot[17] = _alphadot;
	statedot[18] = _betadot;

	return 1;
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
    if(!fdmExec) return 0; 
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
     
     return 1; 
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Copy_Init_To_JSBSim(double init_values[]){
    if(!fdmExec) return 0; 
    
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
    return 1; 
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::OpenScript(const SGPath& script, double delta_t, const SGPath& initfile){
    
    fdmExec->SetAircraftPath (SGPath("aircraft")); // remove argument (rootDir + "aircraft")    
    fdmExec->SetEnginePath   (SGPath("engine"));
    fdmExec->SetSystemsPath  (SGPath("systems"));
    
    if(!fdmExec->LoadScript(script, delta_t, initfile)){
        mexPrintf("Could not open a script \n"); 
        return 0; 
    }
    fdmExec->RunIC();
    return 1; 
    
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
    
    return 1;     
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
    return 1;     
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool JSBSimInterface::Copy_Control_From_JSBSim(double *state_array){
    // [thr-pos-norm left-ail-pos-rad right-ail-pos-rad el-pos-rad rud-pos-rad flap-pos-norm ]
    // speedbrake-pos-rad spoiler-pos-rad gear-pos-norm]
    // Possibly add some error handling here? 
    
    state_array[0] = fcs->GetThrottlePos(0); //should have an input command? NEEDS FIX   
    
    state_array[1] = fcs->GetDaLPos(); //left aileron 
    state_array[2] = fcs->GetDaRPos(); //right aileron
    state_array[3] = fcs->GetDePos(); //elevator
    state_array[4] = fcs->GetDrPos(); //rudder
    state_array[5] = fcs->GetDfPos(); //flaps
    state_array[6] = fcs->GetDsbPos(); //speedbrake
    state_array[7] = fcs->GetDspPos(); // spoiler
    
    state_array[8] = fcs->GetGearPos(); //gear, 0 is up, 1 is down. Down is default. 
    
    
    return 1;     
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void JSBSimInterface::HW_To_JSBSim(double input){
    //fcs->SetDePos(ofNorm, input); //rads
    SetPropertyValue("elevator-cmd-norm", input); //norm cmd 
    //fdmExec->GetFCS()->SetDePos(input);
}

double JSBSimInterface::HW_From_JSBSim(){
    double output;  
    output = fcs->GetDeCmd(); // this is the elevator cmd, between -1 and 1
    return output; 
    //fdmExec->GetFCS()->SetDePos(input);
}
