/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
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
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGAIRCRAFT_H
#define FGAIRCRAFT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <vector>
#    include <iterator>
#    include <map>
#  else
#    include <vector.h>
#    include <iterator.h>
#    include <map.h>
#  endif
#else
#  include <vector>
#  include <iterator>
#  include <map>
#endif

#include "FGModel.h"
#include "FGCoefficient.h"
#include "FGPropulsion.h"
#include "FGLGear.h"
#include "FGConfigFile.h"
#include "FGMatrix.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AIRCRAFT "$Id: FGAircraft.h,v 1.46 2001/03/22 14:10:24 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates an Aircraft and its systems.
    Owns all the parts (other classes) which make up this aircraft. This includes
    the Engines, Tanks, Propellers, Nozzles, Aerodynamic and Mass properties,
    landing gear, etc. These constituent parts may actually run as separate
    JSBSim models themselves, but the responsibility for initializing them and
    for retrieving their force and moment contributions falls to FGAircraft.<br>
    When an aircraft model is loaded the config file is parsed and for each of the
    sections of the config file (propulsion, flight control, etc.) the
    corresponding "ReadXXX()" method is called. From within this method the 
    "Load()" method of that system is called (e.g. LoadFCS).
    @author Jon S. Berndt
    @version $Id: FGAircraft.h,v 1.46 2001/03/22 14:10:24 jberndt Exp $
    @see
     <ol><li>Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
	   Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
	   School, January 1994</li>
     <li>D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
     JSC 12960, July 1977</li>
     <li>Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
     NASA-Ames", NASA CR-2497, January 1975</li>
     <li>Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
     Wiley & Sons, 1979 ISBN 0-471-03032-5</li>
     <li>Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
     1982 ISBN 0-471-08936-2</li></ol>
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAircraft : public FGModel {
  enum {eL=1, eM, eN};
  enum {eX=1, eY, eZ};
  enum {eP=1, eQ, eR};
  enum {ePhi=1, eTht, ePsi};

public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGAircraft(FGFDMExec *Executive);
  
  /// Destructor
  ~FGAircraft();

  /** Runs the Aircraft model; called by the Executive
      @see JSBSim.cpp documentation
      @return false if no error */
  bool Run(void);
  
  /** Loads the aircraft.
      The executive calls this method to load the aircraft into JSBSim.
      @param apath path to the aircraft files (e.g. "aircraft/X15/")
      @param epath path to engine files (e.g. "engine/")
      @param acname name of aircraft (e.g. "X15")
      @return true if succesful */
  bool LoadAircraft(string apath, string epath, string acname);
  
  /** Gets the aircraft name
      @return the name of the aircraft as a string type */
  inline string GetAircraftName(void) { return AircraftName; }
  
  /** Gets the gear status
      @return true if gear is not deployed */
  inline bool GetGearUp(void) { return GearUp; }
  /** Gets the number of gear units defined for the aircraft
      @return number of gear units defined */
  inline int GetNumGearUnits(void) { return lGear.size(); }
  /** Gets a gear instance
      @param gear index of gear instance
      @return a pointer to the FGLGear instance of the gear unit requested */
  inline FGLGear* GetGearUnit(int gear) { return &(lGear[gear]); }
  /// Gets the wing area
  inline float GetWingArea(void) { return WingArea; }
  /// Gets the wing span
  inline float GetWingSpan(void) { return WingSpan; }
  /// Gets the average wing chord
  inline float Getcbar(void) { return cbar; }
  inline float GetWeight(void) { return Weight; }
  inline float GetMass(void) { return Mass; }
  inline FGColumnVector GetMoments(void) { return vMoments; }
  inline FGColumnVector GetForces(void) { return vForces; }
  inline FGColumnVector GetvFs(void) { return vFs; }
  inline float GetIxx(void) { return Ixx; }
  inline float GetIyy(void) { return Iyy; }
  inline float GetIzz(void) { return Izz; }
  inline float GetIxz(void) { return Ixz; }
  inline FGColumnVector GetXYZcg(void) { return vXYZcg; }
  inline FGColumnVector GetXYZrp(void) { return vXYZrp; }
  inline FGColumnVector GetXYZep(void) { return vXYZep; }
  inline float GetNlf(void) { return nlf; }
  inline float GetAlphaCLMax(void) { return alphaclmax; }
  inline float GetAlphaCLMin(void) { return alphaclmin; }

  inline void SetGearUp(bool tt) { GearUp = tt; }
  inline void SetAlphaCLMax(float tt) { alphaclmax=tt; }
  inline void SetAlphaCLMin(float tt) { alphaclmin=tt; }

  inline FGCoefficient* GetCoeff(int axis, int idx) { return Coeff[axis][idx]; }
  string GetCoefficientStrings(void);
  string GetCoefficientValues(void);
  string GetGroundReactionStrings(void);
  string GetGroundReactionValues(void);

  /// Subsystem types for specifying which will be output in the FDM data logging
  enum  SubSystems {
    /** Subsystem: Simulation (= 1)          */ ssSimulation      = 1,
    /** Subsystem: Aerosurfaces (= 2)        */ ssAerosurfaces    = 2,
    /** Subsystem: Body rates (= 4)          */ ssRates           = 4,
    /** Subsystem: Velocities (= 8)          */ ssVelocities      = 8,
    /** Subsystem: Forces (= 16)             */ ssForces          = 16,
    /** Subsystem: Moments (= 32)            */ ssMoments         = 32,
    /** Subsystem: Atmosphere (= 64)         */ ssAtmosphere      = 64,
    /** Subsystem: Mass Properties (= 128)   */ ssMassProps       = 128,
    /** Subsystem: Coefficients (= 256)      */ ssCoefficients    = 256,
    /** Subsystem: Position (= 512)          */ ssPosition        = 512,
    /** Subsystem: Ground Reactions (= 1024) */ ssGroundReactions = 1024,
    /** Subsystem: Ground Reactions (= 2048) */ ssFCS             = 2048
  } subsystems;

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
  float alpha, beta;
  float WingArea, WingSpan, cbar;
  float Weight, EmptyWeight;
  float nlf,alphaclmax,alphaclmin;
  float dt;
  string CFGVersion;
  string AircraftName;

  typedef map<string,int> AxisIndex;
  AxisIndex AxisIdx;

  typedef vector<FGCoefficient*> CoeffArray;

  CoeffArray* Coeff;

  void DisplayCoeffFactors(vector <eParam> multipliers);

  bool GearUp;

  string Axis[6];
  vector <FGLGear> lGear;

  string AircraftPath;
  string EnginePath;
  void ReadMetrics(FGConfigFile*);
  void ReadPropulsion(FGConfigFile*);
  void ReadFlightControls(FGConfigFile*);
  void ReadAerodynamics(FGConfigFile*);
  void ReadUndercarriage(FGConfigFile*);
  void ReadPrologue(FGConfigFile*);
  void ReadOutput(FGConfigFile*);
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

