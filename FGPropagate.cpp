/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropagate.cpp
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

#include "FGPropagate.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGInertial.h"
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGPropagate.cpp,v 1.4 2004/04/17 21:16:19 jberndt Exp $";
static const char *IdHdr = ID_PROPAGATE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropagate::FGPropagate(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGPropagate";

  vVRPoffset.InitMatrix();

  vUVWdot.InitMatrix();
  vUVWdot_prev[0].InitMatrix();
  vUVWdot_prev[1].InitMatrix();
  vUVWdot_prev[2].InitMatrix();
  vUVWdot_prev[3].InitMatrix();

  vPQRdot.InitMatrix();
  vPQRdot_prev[0].InitMatrix();
  vPQRdot_prev[1].InitMatrix();
  vPQRdot_prev[2].InitMatrix();
  vPQRdot_prev[3].InitMatrix();

  LongitudeVRP = LatitudeVRP = 0.0;
  hoverbmac = hoverbcg = 0.0;
  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropagate::~FGPropagate(void)
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropagate::InitModel(void)
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
Purpose: Called on a schedule to perform Propagating algorithms
Notes:   [TP] Make sure that -Vt <= hdot <= Vt, which, of course, should always
         be the case
         [JB] Run in standalone mode, SeaLevelRadius will be reference radius.
               In FGFS, SeaLevelRadius is stuffed from FGJSBSim in JSBSim.cxx each pass.
*/

bool FGPropagate::Run(void)
{
  if (!FGModel::Run()) {
    double dt = State->Getdt()*rate;
    const FGColumnVector3& vMoments = Aircraft->GetMoments();

    // Propagate body rotational rates based on body moments
    vPQRdot = MassBalance->GetJinv()*(vMoments - vPQR*(MassBalance->GetJ()*vPQR));
    vPQR += State->Integrate(FGState::TRAPZ, dt, vPQRdot, vPQRdot_prev);

    // Propagate quaternion orientation based on body rotational rates
    FGQuaternion vQtrndot = vQtrn.GetQDot( vPQR );
    vQtrn += State->Integrate(FGState::TRAPZ, dt, vQtrndot, vQtrndot_prev);

    // Propagate body frame velocity based on aircraft accelerations
    vUVWdot = vUVW*vPQR + Aircraft->GetBodyAccel();
    vUVW += State->Integrate(FGState::TRAPZ, dt, vUVWdot, vUVWdot_prev);

    // Convert local frame velocity vector based on body frame velocity
    vVel = vQtrn.GetTInv() * vUVW;

    // Propagate globe ("map") location based on local frame velocity
    vLocationDot = toGlobe(vVel);
    vLocation += State->Integrate(FGState::TRAPZ, dt, vLocationDot, vLocationDot_prev);

    // Update altitude parameter
    h = vLocation(eRad) - SeaLevelRadius;           // Geocentric

    vVRPoffset = vQtrn.GetTInv() * MassBalance->StructuralToBody(Aircraft->GetXYZvrp());

    // vVRP  - the vector to the Visual Reference Point - now contains the
    // offset from the CG to the VRP, in units of feet, in the Local coordinate
    // frame, where X points north, Y points East, and Z points down. This needs
    // to be converted to Lat/Lon/Alt, now.

    if (cos(vLocation(eLat)) != 0)
      LongitudeVRP = vVRPoffset(eEast) / (vLocation(eRad) * cos(vLocation(eLat))) + vLocation(eLong);

    LatitudeVRP = vVRPoffset(eNorth) / vLocation(eRad) + vLocation(eLat);
    hVRP = h - vVRPoffset(eDown);

    DistanceAGL = vLocation(eRad) - RunwayRadius;   // Geocentric

    b = Aircraft->GetWingSpan();
    hoverbcg = DistanceAGL/b;

    vMac = vQtrn.GetTInv()*MassBalance->StructuralToBody(Aircraft->GetXYZrp());
    hoverbmac = (DistanceAGL + vMac(3)) / b;

    return false;

  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::Seth(double tt)
{
 h = tt;
 Radius    = h + SeaLevelRadius;
 DistanceAGL = Radius - RunwayRadius;   // Geocentric
 hoverbcg = DistanceAGL/b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetDistanceAGL(double tt)
{
  DistanceAGL=tt;
  Radius = RunwayRadius + DistanceAGL;
  h = Radius - SeaLevelRadius;
  hoverbcg = DistanceAGL/b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGPropagate::toGlobe(FGColumnVector3& V)
{
  vLocation(eRad) = h + SeaLevelRadius;

  if (cos(vLocation(eLat)) != 0) vLocationDot(eLong) = V(eEast) / (Radius * cos(vLocation(eLat)));
  vLocationDot(eLat) = V(eNorth) / vLocation(eRad);
  vLocationDot(eRad) = -V(eDown);

  return vLocationDot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::bind(void)
{
  typedef double (FGPropagate::*PMF)(int) const;
  PropertyManager->Tie("velocities/h-dot-fps", this, &FGPropagate::Gethdot);

  PropertyManager->Tie("velocities/v-north-fps", this, &FGPropagate::GetVn);
  PropertyManager->Tie("velocities/v-east-fps", this, &FGPropagate::GetVe);
  PropertyManager->Tie("velocities/v-down-fps", this, &FGPropagate::GetVd);

  PropertyManager->Tie("velocities/u-fps", this, eU, (PMF)&FGPropagate::GetUVW);
  PropertyManager->Tie("velocities/v-fps", this, eV, (PMF)&FGPropagate::GetUVW);
  PropertyManager->Tie("velocities/w-fps", this, eW, (PMF)&FGPropagate::GetUVW);

  PropertyManager->Tie("velocities/p-rad_sec", this, eP, (PMF)&FGPropagate::GetPQR);
  PropertyManager->Tie("velocities/q-rad_sec", this, eQ, (PMF)&FGPropagate::GetPQR);
  PropertyManager->Tie("velocities/r-rad_sec", this, eR, (PMF)&FGPropagate::GetPQR);

  PropertyManager->Tie("accelerations/pdot-rad_sec", this, eP, (PMF)&FGPropagate::GetPQRdot);
  PropertyManager->Tie("accelerations/qdot-rad_sec", this, eQ, (PMF)&FGPropagate::GetPQRdot);
  PropertyManager->Tie("accelerations/rdot-rad_sec", this, eR, (PMF)&FGPropagate::GetPQRdot);

  PropertyManager->Tie("accelerations/udot-fps", this, eU, (PMF)&FGPropagate::GetUVWdot);
  PropertyManager->Tie("accelerations/vdot-fps", this, eV, (PMF)&FGPropagate::GetUVWdot);
  PropertyManager->Tie("accelerations/wdot-fps", this, eW, (PMF)&FGPropagate::GetUVWdot);

  PropertyManager->Tie("position/h-sl-ft", this, &FGPropagate::Geth, &FGPropagate::Seth, true);
  PropertyManager->Tie("position/lat-gc-rad", this, &FGPropagate::GetLatitude, &FGPropagate::SetLatitude);
  PropertyManager->Tie("position/lat-dot-gc-rad", this, &FGPropagate::GetLatitudeDot);
  PropertyManager->Tie("position/long-gc-rad", this, &FGPropagate::GetLongitude, &FGPropagate::SetLongitude, true);
  PropertyManager->Tie("position/long-dot-gc-rad", this, &FGPropagate::GetLongitudeDot);
  PropertyManager->Tie("position/h-agl-ft", this,  &FGPropagate::GetDistanceAGL, &FGPropagate::SetDistanceAGL);
  PropertyManager->Tie("position/radius-to-vehicle-ft", this, &FGPropagate::GetRadius);

  PropertyManager->Tie("metrics/runway-radius", this, &FGPropagate::GetRunwayRadius, &FGPropagate::SetRunwayRadius);

  PropertyManager->Tie("aero/h_b-cg-ft", this, &FGPropagate::GetHOverBCG);
  PropertyManager->Tie("aero/h_b-mac-ft", this, &FGPropagate::GetHOverBMAC);

  PropertyManager->Tie("attitude/phi-rad", this, &FGPropagate::Getphi);
  PropertyManager->Tie("attitude/theta-rad", this, &FGPropagate::Gettht);
  PropertyManager->Tie("attitude/psi-rad", this, &FGPropagate::Getpsi);

  PropertyManager->Tie("attitude/roll-rad", this, ePhi, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/pitch-rad", this, eTht, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/heading-true-rad", this, ePsi, (PMF)&FGPropagate::GetEuler);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::unbind(void)
{
  PropertyManager->Untie("velocities/v-north-fps");
  PropertyManager->Untie("velocities/v-east-fps");
  PropertyManager->Untie("velocities/v-down-fps");
  PropertyManager->Untie("velocities/h-dot-fps");
  PropertyManager->Untie("velocities/u-fps");
  PropertyManager->Untie("velocities/v-fps");
  PropertyManager->Untie("velocities/w-fps");
  PropertyManager->Untie("velocities/p-rad_sec");
  PropertyManager->Untie("velocities/q-rad_sec");
  PropertyManager->Untie("velocities/r-rad_sec");
  PropertyManager->Untie("accelerations/udot-fps");
  PropertyManager->Untie("accelerations/vdot-fps");
  PropertyManager->Untie("accelerations/wdot-fps");
  PropertyManager->Untie("accelerations/pdot-rad_sec");
  PropertyManager->Untie("accelerations/qdot-rad_sec");
  PropertyManager->Untie("accelerations/rdot-rad_sec");
  PropertyManager->Untie("position/h-sl-ft");
  PropertyManager->Untie("position/lat-gc-rad");
  PropertyManager->Untie("position/lat-dot-gc-rad");
  PropertyManager->Untie("position/long-gc-rad");
  PropertyManager->Untie("position/long-dot-gc-rad");
  PropertyManager->Untie("position/h-agl-ft");
  PropertyManager->Untie("position/radius-to-vehicle-ft");
  PropertyManager->Untie("metrics/runway-radius");
  PropertyManager->Untie("aero/h_b-cg-ft");
  PropertyManager->Untie("aero/h_b-mac-ft");
  PropertyManager->Untie("attitude/phi-rad");
  PropertyManager->Untie("attitude/theta-rad");
  PropertyManager->Untie("attitude/psi-rad");
  PropertyManager->Untie("attitude/roll-rad");
  PropertyManager->Untie("attitude/pitch-rad");
  PropertyManager->Untie("attitude/heading-true-rad");
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

void FGPropagate::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPropagate" << endl;
    if (from == 1) cout << "Destroyed:    FGPropagate" << endl;
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
