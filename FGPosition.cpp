/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Module:       FGPosition.cpp
 Author:       Jon S. Berndt
 Date started: 01/05/99
 Purpose:      Integrate the EOM to determine instantaneous position
 Called by:    FGFDMExec
 
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
 
FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This class encapsulates the integration of rates and accelerations to get the
current position of the aircraft.

HISTORY
--------------------------------------------------------------------------------
01/05/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <cmath>
#    include <iomanip>
#  else
#    include <math.h>
#    include <iomanip.h>
#  endif
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#    if (_COMPILER_VERSION < 740)
#      include <iomanip.h>
#    else
#      include <iomanip>
#    endif
#  else
#    include <cmath>
#    include <iomanip>
#  endif
#endif

#include "FGPosition.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGPropertyManager.h"


namespace JSBSim {

static const char *IdSrc = "$Id: FGPosition.cpp,v 1.62 2004/01/11 19:46:02 jberndt Exp $";
static const char *IdHdr = ID_POSITION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

extern double globalTriNormal[3];
extern double globalSceneryAltitude;
extern double globalSeaLevelRadius;

FGPosition::FGPosition(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGPosition";
  LongitudeDot = LatitudeDot = RadiusDot = 0.0;
  
  for (int i=0;i<4;i++) {
    LatitudeDot_prev[i]  = 0.0;
    LongitudeDot_prev[i] = 0.0;
    RadiusDot_prev[i]    = 0.0;
  }

  vVRPoffset.InitMatrix();

  Longitude = Latitude = 0.0;
  LongitudeVRP = LatitudeVRP = 0.0;
  gamma = Vt = Vground = 0.0;
  hoverbmac = hoverbcg = 0.0;
  psigt = 0.0;
  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPosition::~FGPosition(void)
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPosition::InitModel(void)
{
  FGModel::InitModel();

  h = 3.0;                                 // Est. height of aircraft cg off runway
  SeaLevelRadius = Inertial->RefRadius();  // For initialization ONLY
  Radius         = SeaLevelRadius + h;
  RunwayRadius   = SeaLevelRadius;
  DistanceAGL    = Radius - RunwayRadius;  // Geocentric
  vRunwayNormal(3) = -1.0;                 // Initialized for standalone mode
  b = 1;
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
Purpose: Called on a schedule to perform Positioning algorithms
Notes:   [TP] Make sure that -Vt <= hdot <= Vt, which, of course, should always
         be the case
         [JB] Run in standalone mode, SeaLevelRadius will be reference radius.
	       In FGFS, SeaLevelRadius is stuffed from FGJSBSim in JSBSim.cxx each pass.
*/

bool FGPosition::Run(void)
{
  double cosLat;
  double hdot_Vt;

  if (!FGModel::Run()) {
    GetState();

    Vground = sqrt( vVel(eNorth)*vVel(eNorth) + vVel(eEast)*vVel(eEast) );
    psigt =  atan2(vVel(eEast), vVel(eNorth));
    if (psigt < 0.0)
      psigt += 2*M_PI;

    Radius    = h + SeaLevelRadius;

    cosLat = cos(Latitude);
    if (cosLat != 0) LongitudeDot = vVel(eEast) / (Radius * cosLat);
    LatitudeDot = vVel(eNorth) / Radius;
    RadiusDot   = -vVel(eDown);

    Longitude += State->Integrate(FGState::TRAPZ, dt*rate, LongitudeDot, LongitudeDot_prev);
    Latitude  += State->Integrate(FGState::TRAPZ, dt*rate, LatitudeDot, LatitudeDot_prev);
    Radius    += State->Integrate(FGState::TRAPZ, dt*rate, RadiusDot, RadiusDot_prev);

    h = Radius - SeaLevelRadius;           // Geocentric

    vVRPoffset = State->GetTb2l() * (vVRP - MassBalance->GetXYZcg());
    vVRPoffset /= 12.0; // converted to feet

    // vVRP  - the vector to the Visual Reference Point - now contains the 
    // offset from the CG to the VRP, in units of feet, in the Local coordinate
    // frame, where X points north, Y points East, and Z points down. This needs
    // to be converted to Lat/Lon/Alt, now.

    DistanceAGL = Radius - RunwayRadius;   // Geocentric
    
    hoverbcg = DistanceAGL/b;
    
    vMac = State->GetTb2l()*Aircraft->GetXYZrp();
    
    vMac *= inchtoft;
    hoverbmac = (DistanceAGL + vMac(3))/b;

    if (Vt > 0) {
      hdot_Vt = RadiusDot/Vt;
      if (fabs(hdot_Vt) <= 1) gamma = asin(hdot_Vt);
    } else {
      gamma = 0.0;
    }

    return false;

  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::GetState(void)
{
  dt = State->Getdt();

  Vt        = Translation->GetVt();
  vVel      = State->GetTb2l() * Translation->GetUVW();
  vVelDot   = State->GetTb2l() * Translation->GetUVWdot();
  
  b = Aircraft->GetWingSpan();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::Seth(double tt)
{
 h = tt;
 Radius    = h + SeaLevelRadius;
 DistanceAGL = Radius - RunwayRadius;   // Geocentric
 hoverbcg = DistanceAGL/b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::SetDistanceAGL(double tt)
{
  DistanceAGL=tt;
  Radius = RunwayRadius + DistanceAGL;
  h = Radius - SeaLevelRadius;
  hoverbcg = DistanceAGL/b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::bind(void)
{
  PropertyManager->Tie("velocities/v-north-fps", this,
                       &FGPosition::GetVn);
  PropertyManager->Tie("velocities/v-east-fps", this,
                       &FGPosition::GetVe);
  PropertyManager->Tie("velocities/v-down-fps", this,
                       &FGPosition::GetVd);
  PropertyManager->Tie("velocities/vg-fps", this,
                       &FGPosition::GetVground);
  PropertyManager->Tie("flight-path/psi-gt-rad", this,
                       &FGPosition::GetGroundTrack);
  PropertyManager->Tie("position/h-sl-ft", this,
                       &FGPosition::Geth,
                       &FGPosition::Seth,
                       true);
  PropertyManager->Tie("velocities/h-dot-fps", this,
                       &FGPosition::Gethdot);
  PropertyManager->Tie("position/lat-gc-rad", this,
                       &FGPosition::GetLatitude,
                       &FGPosition::SetLatitude);
  PropertyManager->Tie("position/lat-dot-gc-rad", this,
                       &FGPosition::GetLatitudeDot);
  PropertyManager->Tie("position/long-gc-rad", this,
                       &FGPosition::GetLongitude,
                       &FGPosition::SetLongitude,
                       true);
  PropertyManager->Tie("position/long-dot-gc-rad", this,
                       &FGPosition::GetLongitudeDot);
  PropertyManager->Tie("metrics/runway-radius", this,
                       &FGPosition::GetRunwayRadius,
                       &FGPosition::SetRunwayRadius);
  PropertyManager->Tie("position/h-agl-ft", this,
                       &FGPosition::GetDistanceAGL,
                       &FGPosition::SetDistanceAGL);
  PropertyManager->Tie("position/radius-to-vehicle-ft", this,
                       &FGPosition::GetRadius);
  PropertyManager->Tie("flight-path/gamma-rad", this,
                       &FGPosition::GetGamma,
                       &FGPosition::SetGamma);
  PropertyManager->Tie("aero/h_b-cg-ft", this,
                       &FGPosition::GetHOverBCG);
  PropertyManager->Tie("aero/h_b-mac-ft", this,
                       &FGPosition::GetHOverBMAC);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::unbind(void)
{
  PropertyManager->Untie("velocities/v-north-fps");
  PropertyManager->Untie("velocities/v-east-fps");
  PropertyManager->Untie("velocities/v-down-fps");
  PropertyManager->Untie("velocities/vg-fps");
  PropertyManager->Untie("flight-path/psi-gt-rad");
  PropertyManager->Untie("position/h-sl-ft");
  PropertyManager->Untie("velocities/h-dot-fps");
  PropertyManager->Untie("position/lat-gc-rad");
  PropertyManager->Untie("position/lat-dot-gc-rad");
  PropertyManager->Untie("position/long-gc-rad");
  PropertyManager->Untie("position/long-dot-gc-rad");
  PropertyManager->Untie("metrics/runway-radius");
  PropertyManager->Untie("position/h-agl-ft");
  PropertyManager->Untie("position/radius-to-vehicle-ft");
  PropertyManager->Untie("flight-path/gamma-rad");
  PropertyManager->Untie("aero/h_b-cg-ft");
  PropertyManager->Untie("aero/h_b-mac-ft");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGPosition::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPosition" << endl;
    if (from == 1) cout << "Destroyed:    FGPosition" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
