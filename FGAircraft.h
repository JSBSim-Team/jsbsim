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
#  ifdef SG_HAVE_STD_INCLUDES
#    include <vector>
#    include <iterator>
#  else
#    include <vector.h>
#    include <iterator.h>
#  endif
#else
#  include <vector>
#  include <iterator>
#endif

#include "FGModel.h"
#include "FGPropulsion.h"
#include "FGConfigFile.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"
#include "FGLGear.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AIRCRAFT "$Id: FGAircraft.h,v 1.70 2001/10/31 12:33:44 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
    @version $Id: FGAircraft.h,v 1.70 2001/10/31 12:33:44 apeden Exp $
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
      @param AC_cfg a pointer to the config file instance
      @return true if successful */
  bool Load(FGConfigFile* AC_cfg);
  
  /** Gets the aircraft name
      @return the name of the aircraft as a string type */
  inline string GetAircraftName(void) { return AircraftName; }
  
  /// Gets the wing area
  inline float GetWingArea(void) { return WingArea; }
  /// Gets the wing span
  inline float GetWingSpan(void) { return WingSpan; }
  /// Gets the average wing chord
  inline float Getcbar(void) { return cbar; }
  inline float GetWingIncidence(void) { return WingIncidence; }
  inline float GetHTailArea(void) { return HTailArea; }
  inline float GetHTailArm(void)  { return HTailArm; }
  inline float GetVTailArea(void) { return VTailArea; }
  inline float GetVTailArm(void)  { return VTailArm; }
  inline float Getlbarh(void) { return lbarh; } // HTailArm / cbar
  inline float Getlbarv(void) { return lbarv; } // VTailArm / cbar
  inline float Getvbarh(void) { return vbarh; } // H. Tail Volume
  inline float Getvbarv(void) { return vbarv; } // V. Tail Volume
  inline FGColumnVector3& GetMoments(void) { return vMoments; }
  inline FGColumnVector3& GetForces(void) { return vForces; }
  inline FGColumnVector3& GetBodyAccel(void) { return vBodyAccel; }
  inline FGColumnVector3& GetNcg   (void)    { return vNcg; }
  inline FGColumnVector3& GetXYZrp(void) { return vXYZrp; }
  inline FGColumnVector3& GetXYZep(void) { return vXYZep; }
  inline float GetXYZrp(int idx) { return vXYZrp(idx); }
  inline float GetXYZep(int idx) { return vXYZep(idx); }
  inline float GetAlphaCLMax(void) { return alphaclmax; }
  inline float GetAlphaCLMin(void) { return alphaclmin; }

  inline void SetAlphaCLMax(float tt) { alphaclmax=tt; }
  inline void SetAlphaCLMin(float tt) { alphaclmin=tt; }
  
  inline bool GetStallWarn(void) { return impending_stall; }

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
    /** Subsystem: FCS (= 2048)              */ ssFCS             = 2048,
    /** Subsystem: Propulsion (= 4096)       */ ssPropulsion      = 4096
  } subsystems;

private:
  FGColumnVector3 vMoments;
  FGColumnVector3 vForces;
  FGColumnVector3 vXYZrp;
  FGColumnVector3 vXYZep;
  FGColumnVector3 vEuler;
  FGColumnVector3 vDXYZcg;
  FGColumnVector3 vBodyAccel;
  FGColumnVector3 vNcg;

  float WingArea, WingSpan, cbar, WingIncidence;
  float HTailArea, VTailArea, HTailArm, VTailArm;
  float lbarh,lbarv,vbarh,vbarv;
  float alphaclmax,alphaclmin;
  bool impending_stall;
  string CFGVersion;
  string AircraftName;

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

