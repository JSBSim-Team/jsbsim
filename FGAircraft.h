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

#define ID_AIRCRAFT "$Id: FGAircraft.h,v 1.85 2002/05/04 15:26:02 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates an Aircraft and its systems.
    Owns all the parts (other classes) which make up this aircraft. This includes
    the Engines, Tanks, Propellers, Nozzles, Aerodynamic and Mass properties,
    landing gear, etc. These constituent parts may actually run as separate
    JSBSim models themselves, but the responsibility for initializing them and
    for retrieving their force and moment contributions falls to FGAircraft.<br>
    
    @author Jon S. Berndt
    @version $Id: FGAircraft.h,v 1.85 2002/05/04 15:26:02 apeden Exp $
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
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGAircraft.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGAircraft.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
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
  double GetWingArea(void) const { return WingArea; }
  /// Gets the wing span
  double GetWingSpan(void) const { return WingSpan; }
  /// Gets the average wing chord
  double Getcbar(void) const { return cbar; }
  inline double GetWingIncidence(void) const { return WingIncidence; }
  inline double GetHTailArea(void) const { return HTailArea; }
  inline double GetHTailArm(void)  const { return HTailArm; }
  inline double GetVTailArea(void) const { return VTailArea; }
  inline double GetVTailArm(void)  const { return VTailArm; }
  inline double Getlbarh(void) const { return lbarh; } // HTailArm / cbar
  inline double Getlbarv(void) const { return lbarv; } // VTailArm / cbar
  inline double Getvbarh(void) const { return vbarh; } // H. Tail Volume
  inline double Getvbarv(void) const { return vbarv; } // V. Tail Volume
  inline FGColumnVector3& GetMoments(void) { return vMoments; }
  inline double GetMoments(int idx) const { return vMoments(idx); }
  inline FGColumnVector3& GetForces(void) { return vForces; }
  inline double GetForces(int idx) const { return vForces(idx); }
  inline FGColumnVector3& GetBodyAccel(void) { return vBodyAccel; }
  inline FGColumnVector3& GetNcg   (void)  { return vNcg; }
  inline FGColumnVector3& GetXYZrp(void) { return vXYZrp; }
  inline FGColumnVector3& GetXYZep(void) { return vXYZep; }
  inline double GetXYZrp(int idx) const { return vXYZrp(idx); }
  inline double GetXYZep(int idx) const { return vXYZep(idx); }
  inline double GetAlphaCLMax(void) const { return alphaclmax; }
  inline double GetAlphaCLMin(void) const { return alphaclmin; }
  
  inline double GetAlphaHystMax(void) const { return alphahystmax; }
  inline double GetAlphaHystMin(void) const { return alphahystmin; }
  inline double GetHysteresisParm(void) const { return stall_hyst; }
  

  inline void SetAlphaCLMax(double tt) { alphaclmax=tt; }
  inline void SetAlphaCLMin(double tt) { alphaclmin=tt; }
  inline void SetAircraftName(string name) {AircraftName = name;}
  
  inline double GetStallWarn(void) const { return impending_stall; }
  
  double GetBI2Vel(void) const { return bi2vel; }
  double GetCI2Vel(void) const { return ci2vel; }
  double GetAlphaW(void) const { return alphaw; }
                                                           
  float GetNlf(void);
  
  inline FGColumnVector3& GetNwcg(void) { return vNwcg; }
  
  void bind(void);
  void unbind(void);

private:
  FGColumnVector3 vMoments;
  FGColumnVector3 vForces;
  FGColumnVector3 vXYZrp;
  FGColumnVector3 vXYZep;
  FGColumnVector3 vEuler;
  FGColumnVector3 vDXYZcg;
  FGColumnVector3 vBodyAccel;
  FGColumnVector3 vNcg;
  FGColumnVector3 vNwcg;

  double WingArea, WingSpan, cbar, WingIncidence;
  double HTailArea, VTailArea, HTailArm, VTailArm;
  double lbarh,lbarv,vbarh,vbarv;
  double alphaclmax,alphaclmin;
  double alphahystmax, alphahystmin;
  double impending_stall, stall_hyst;
  double bi2vel, ci2vel,alphaw;
  string AircraftName;

  void Debug(int from);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

