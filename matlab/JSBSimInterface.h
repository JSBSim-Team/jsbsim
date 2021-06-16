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
	JSBSimInterface(FGFDMExec *);
	JSBSimInterface(FGFDMExec *, double dt);
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
    
    bool HW_To_JSBSim(double input); 
    double HW_From_JSBSim(); 

	bool IsAircraftLoaded(){return _ac_model_loaded;}
	/// Set an initial state
	/*
        *prhs1 is a Matlab structure of names/values couples, 
        e.g. 
        ic( 1).name  = 'u-fps';
        ic( 1).value = 80;
        ic( 2).name  = 'v-fps';
        ic( 2).value = 20;
        ic( 3).name  = 'w-fps';
        ic( 3).value = 10;
	*/
	bool Init(const mxArray *prhs1);

	/// put the 16 dotted quantities into statedot:
	/*
	dot of (u,v,w,p,q,r,q1,q2,q3,q4,x,y,z,phi,theta,psi)
	*/
	bool Init(const mxArray *prhs1, vector<double>& statedot);

  /// Set an initial state
  /*
    *prhs1 is a Matlab structure of strings/values couples,
      e.g. ['u','v','w', 'p','q','r', 'phi','tht','psi', 'lat','lon','h' ]
      [ 80,0,0, 0,0,0, 0,2,0, 36,44,1000 ]
      where
      (*rhs1)(4).name = 'p'; (*rhs1)(1).value = 80;
  */
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
