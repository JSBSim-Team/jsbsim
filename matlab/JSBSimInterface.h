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


#ifndef JSBSIMINTERFACE_HEADER_H
#define JSBSIMINTERFACE_HEADER_H

#include "mex.h"
#include <FGFDMExec.h>
#include <initialization/FGInitialCondition.h>
#include <models/FGAuxiliary.h>
#include <models/FGPropulsion.h>
#include <models/FGFCS.h>

using namespace JSBSim;

class JSBSimInterface
{
public:
  JSBSimInterface(void);
  JSBSimInterface(double dt);
	~JSBSimInterface(void);
	/// Open an aircraft model from Matlab
	bool Open(const mxArray *prhs0);
	bool Open(const std::string& acName); 
	/// Get a property from the catalog
	bool GetPropertyValue(const mxArray *prhs1, double& value);
    bool GetPropertyValue(const std::string& prop, double& value);
	/// Set a property in the catalog
	bool SetPropertyValue(const mxArray *prhs1, const mxArray *prhs2);
	/// Set a property in the catalog
	bool SetPropertyValue(const std::string& prop, const double value);
	/// Enables a number of commonly used settings
	bool EasySetValue(const std::string& prop, const double value);
	// Get a commonly used value
	double EasyGetValue(const std::string& prop, double& value);
	/// Check if the given string is present in the catalog
	bool QueryJSBSimProperty(const std::string& prop);
    /// Copy control inputs to JSBSim 
    bool Copy_Controls_To_JSBSim(double controls[]);
    bool Copy_Init_To_JSBSim(double init_values[]);
    bool Copy_States_From_JSBSim(double *state_array);
    bool Copy_Pilot_From_JSBSim(double *state_array);
    bool Copy_Control_From_JSBSim(double *state_array);
    /// Script handling 
    //bool OpenScript(const mxArray *prhs);
    bool OpenScript(const SGPath& script, double delta_t, const SGPath& initfile);
	/// Print the aircraft catalog
	void PrintCatalog();
    /// Update the simulation
    void Update();
    void RunIC();
    void LoadIC(SGPath ResetName);

	bool IsAircraftLoaded(){return _ac_model_loaded;}
  bool ResetToInitialCondition(void);

	// Wrapper functions to the FGFDMExec class
	bool RunFDMExec() {return fdmExec->Run();}

	// false - never hold the simulation time from advancing
	// TODO: make setable
	bool RunPropagate() {return propagate->Run(false);}

	// false - never hold the simulation time from advancing
	// TODO: make setable
	bool RunAuxiliary() {return auxiliary->Run(false);}

	// false - never hold the simulation time from advancing
	// TODO: make setable
	bool RunPropulsion() {return propulsion->Run(false);}

	// false - never hold the simulation time from advancing
	// TODO: make setable
	bool RunFCS() {return fcs->Run(false);}


private:
	FGFDMExec *fdmExec;
	FGPropagate *propagate;
	FGAccelerations *accel;
	FGAuxiliary *auxiliary;
	std::vector<std::string> catalog;
	FGInitialCondition *ic;
	FGAerodynamics *aerodynamics;
	FGPropulsion *propulsion;
	FGFCS *fcs;

	bool _ac_model_loaded;

	double _u, _v, _w;
	double _p, _q, _r;
	double _alpha, _beta;
	double _phi, _theta, _psi, _gamma;

	double _stheta, _sphi, _spsi;
	double _ctheta, _cphi, _cpsi;

	double _udot, _vdot, _wdot, _pdot, _qdot, _rdot;
	double _q1dot, _q2dot, _q3dot, _q4dot;
	double _xdot, _ydot, _zdot;
	double _phidot, _thetadot, _psidot;
	double _alphadot,_betadot,_hdot;

};
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
