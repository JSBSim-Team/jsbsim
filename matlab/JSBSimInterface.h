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
  	JSBSimInterface(int numOutputPorts);
  	JSBSimInterface(double dt, int numOutputPorts);
	~JSBSimInterface(void);

	/// Open an aircraft model from Matlab
	bool OpenAircraft(const std::string& acName);

	/// Script handling
    bool OpenScript(const SGPath& script, double delta_t, const SGPath& initfile);

	bool LoadIC(SGPath ResetName);

	/// Update the simulation
    void Update();

	/// Dynamic updating of the property nodes for input and output
	bool AddInputPropertyNode(std::string property);
	bool AddWeatherPropertyNode(std::string property);
	bool AddOutputPropertyNode(std::string property, const int outputPort);

	/// Copy control inputs to JSBSim
    bool CopyInputControlsToJSBSim(std::vector<double> controls);

	/// Copy weather inputs to JSBSim
	bool CopyInputWeatherToJSBSim(std::vector<double> weather);

	/// Copy the flight state outputs from JSBSim
    bool CopyOutputsFromJSBSim(double *stateArray, const int outputPort);

	bool IsAircraftLoaded(){return _ac_model_loaded;}

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
	FGPropertyManager* pm;
	FGPropagate *propagate;
	FGAccelerations *accel;
	FGAuxiliary *auxiliary;
	std::vector<std::string> catalog;
	FGInitialCondition *ic;
	FGAerodynamics *aerodynamics;
	FGPropulsion *propulsion;
	FGFCS *fcs;

	std::vector<std::vector<SGPropertyNode*>> outputPorts;
	std::vector<SGPropertyNode*> inputPort;
	std::vector<SGPropertyNode*> weatherPort;

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
