/*******************************************************************************
 
 Header:       FGAircraft.h
 Author:       Jon S. Berndt
 Date started: 12/12/98
 
 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------
 
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.
 
 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.
 
 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.
 
 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.
 
HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGAIRCRAFT_H
#define FGAIRCRAFT_H

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
*******************************************************************************/
/*
The aerodynamic coefficients used in this model typically are:
 
Longitudinal
  CL0 - Reference lift at zero alpha
  CD0 - Reference drag at zero alpha
  CDM - Drag due to Mach
  CLa - Lift curve slope (w.r.t. alpha)
  CDa - Drag curve slope (w.r.t. alpha)
  CLq - Lift due to pitch rate
  CLM - Lift due to Mach
  CLadt - Lift due to alpha rate
 
  Cmadt - Pitching Moment due to alpha rate
  Cm0 - Reference Pitching moment at zero alpha
  Cma - Pitching moment slope (w.r.t. alpha)
  Cmq - Pitch damping (pitch moment due to pitch rate)
  CmM - Pitch Moment due to Mach
 
Lateral
  Cyb - Side force due to sideslip
  Cyr - Side force due to yaw rate
 
  Clb - Dihedral effect (roll moment due to sideslip)
  Clp - Roll damping (roll moment due to roll rate)
  Clr - Roll moment due to yaw rate
  Cnb - Weathercocking stability (yaw moment due to sideslip)
  Cnp - Rudder adverse yaw (yaw moment due to roll rate)
  Cnr - Yaw damping (yaw moment due to yaw rate)
 
Control
  CLDe - Lift due to elevator
  CDDe - Drag due to elevator
  CyDr - Side force due to rudder
  CyDa - Side force due to aileron
 
  CmDe - Pitch moment due to elevator
  ClDa - Roll moment due to aileron
  ClDr - Roll moment due to rudder
  CnDr - Yaw moment due to rudder
  CnDa - Yaw moment due to aileron
 
[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
	 Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
	 School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
	 JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
	 NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
	 Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
	 1982 ISBN 0-471-08936-2
*/

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <vector>
#    include <map>
#  else
#    include <vector.h>
#    include <map.h>
#  endif
#else
#  include <vector>
#  include <map>
#endif

#include "FGModel.h"
#include "FGCoefficient.h"
#include "FGEngine.h"
#include "FGTank.h"
#include "FGLGear.h"
#include "FGConfigFile.h"
#include "FGMatrix.h"

/*******************************************************************************
DEFINITIONS
*******************************************************************************/

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAircraft : public FGModel {
  enum {eL=1, eM, eN};
  enum {eX=1, eY, eZ};
  enum {eP=1, eQ, eR};
  enum {ePhi=1, eTht, ePsi};

public:
  FGAircraft(FGFDMExec*);
  ~FGAircraft(void);

  bool Run(void);
  bool LoadAircraft(string, string, string);
  inline string GetAircraftName(void) {
    return AircraftName;
  }
  inline void SetGearUp(bool tt) {
    GearUp = tt;
  }
  inline bool GetGearUp(void) {
    return GearUp;
  }
  inline float GetWingArea(void) {
    return WingArea;
  }
  inline float GetWingSpan(void) {
    return WingSpan;
  }
  inline float Getcbar(void) {
    return cbar;
  }
  inline FGEngine* GetEngine(int tt) {
    return Engine[tt];
  }
  inline FGTank* GetTank(int tt) {
    return Tank[tt];
  }
  inline float GetWeight(void) {
    return Weight;
  }
  inline float GetMass(void) {
    return Mass;
  }
  inline FGColumnVector GetMoments(void) {
    return vMoments;
  }
  inline FGColumnVector GetForces(void) {
    return vForces;
  }
  inline FGColumnVector GetvFs(void) {
    return vFs;
  }
  inline float GetIxx(void) {
    return Ixx;
  }
  inline float GetIyy(void) {
    return Iyy;
  }
  inline float GetIzz(void) {
    return Izz;
  }
  inline float GetIxz(void) {
    return Ixz;
  }
  inline int   GetNumEngines(void) {
    return numEngines;
  }
  inline FGColumnVector GetXYZcg(void) {
    return vXYZcg;
  }
  inline float GetNlf(void) {
    return nlf;
  }
  string GetCoefficientStrings(void);
  string GetCoefficientValues(void);

  enum { ssSimulation   = 1,
         ssAerosurfaces = 2,
         ssRates        = 4,
         ssVelocities   = 8,
         ssForces       = 16,
         ssMoments      = 32,
         ssAtmosphere   = 64,
         ssMassProps    = 128,
         ssCoefficients = 256,
         ssPosition     = 512 } subsystems;

private:
  void GetState(void);
  void FMAero(void);
  void FMGear(void);
  void FMMass(void);
  void FMProp(void);
  void MassChange(void);
  FGColumnVector vMoments;
  FGColumnVector vForces;
  FGColumnVector vFs;
  FGColumnVector vXYZrp;
  FGColumnVector vbaseXYZcg;
  FGColumnVector vXYZcg;
  FGColumnVector vXYZep;
  FGColumnVector vEuler;
  float baseIxx, baseIyy, baseIzz, baseIxz, EmptyMass, Mass;
  float Ixx, Iyy, Izz, Ixz;
  float rho, qbar, Vt;
  float alpha, beta;
  float WingArea, WingSpan, cbar;
  float Weight, EmptyWeight;
  float nlf;
  float dt;
  double CFGVersion;
  string AircraftName;

  int numTanks;
  int numEngines;
  int numSelectedOxiTanks;
  int numSelectedFuelTanks;
  FGTank* Tank[MAX_TANKS];
  FGEngine *Engine[MAX_ENGINES];

  typedef map<string,int> AxisIndex;
  AxisIndex AxisIdx;

  typedef vector<FGCoefficient> CoeffArray;
  typedef vector<CoeffArray> CoeffVector;

  CoeffVector Coeff;

  void DisplayCoeffFactors(int multipliers);

  bool GearUp;

  string Axis[6];
  vector <FGLGear*> lGear;
  string AircraftPath;
  string EnginePath;
  void ReadMetrics(FGConfigFile*);
  void ReadPropulsion(FGConfigFile*);
  void ReadFlightControls(FGConfigFile*);
  void ReadAerodynamics(FGConfigFile*);
  void ReadUndercarriage(FGConfigFile*);
  void ReadPrologue(FGConfigFile*);
  void ReadOutput(FGConfigFile*);
};

/******************************************************************************/
#endif
