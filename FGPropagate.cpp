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

static const char *IdSrc = "$Id: FGPropagate.cpp,v 1.13 2004/05/21 16:01:09 frohlich Exp $";
static const char *IdHdr = ID_PROPAGATE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropagate::FGPropagate(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGPropagate";

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

  SeaLevelRadius = Inertial->RefRadius();          // For initialization ONLY
  RunwayRadius   = SeaLevelRadius;

  vLocation.SetRadius( SeaLevelRadius + 4.0 );

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetInitialState(const FGInitialCondition *FGIC)
{
  SeaLevelRadius = FGIC->GetSeaLevelRadiusFtIC();
  RunwayRadius = FGIC->GetSeaLevelRadiusFtIC() + FGIC->GetTerrainAltitudeFtIC();

  // Set the position lat/lon/radius
  vLocation = FGLocation( FGIC->GetLongitudeRadIC(),
                          FGIC->GetLatitudeRadIC(),
                          FGIC->GetAltitudeFtIC() + FGIC->GetSeaLevelRadiusFtIC() );

  // Set the Orientation from the euler angles
  vQtrn = FGQuaternion( FGIC->GetPhiRadIC(),
                        FGIC->GetThetaRadIC(),
                        FGIC->GetPsiRadIC() );

  // Set the velocities in the instantaneus body frame
  vUVW = FGColumnVector3( FGIC->GetUBodyFpsIC(),
                          FGIC->GetVBodyFpsIC(),
                          FGIC->GetWBodyFpsIC() );

  // Set the angular velocities in the instantaneus body frame.
  vPQR = FGColumnVector3( FGIC->GetPRadpsIC(),
                          FGIC->GetQRadpsIC(),
                          FGIC->GetRRadpsIC() );

  // Compute some derived values.
  vVel = vQtrn.GetTInv()*vUVW;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
Purpose: Called on a schedule to perform EOM integration
Notes:   [JB] Run in standalone mode, SeaLevelRadius will be reference radius.
         In FGFS, SeaLevelRadius is stuffed from FGJSBSim in JSBSim.cxx each pass.
*/

bool FGPropagate::Run(void)
{
  // Fast return if we have nothing to do ...
  if (FGModel::Run()) return true;

  // The 'stepsize'
  double dt = State->Getdt()*rate;

  // Get earth rotation angular velocity
  const FGColumnVector3 omega( 0.0, 0.0, Inertial->omega() );

  // Get current forces and moments.
  const FGColumnVector3& vForces = Aircraft->GetForces();
  const FGColumnVector3& vMoments = Aircraft->GetMoments();

  // Get the mass and inertia values.
  double mass = MassBalance->GetMass();
  const FGMatrix33& J = MassBalance->GetJ();
  const FGMatrix33& Jinv = MassBalance->GetJinv();

  // The rotation matrices:
  // From the local horizontal frame to the body frame
  const FGMatrix33& Tl2b = GetTl2b();
  // From the body frame to the local horizontal
  const FGMatrix33& Tb2l = GetTb2l();
  // From the earth centered frame to the local horizontal frame
  const FGMatrix33& Tec2l = vLocation.GetTec2l();
  // From the local horizontal frame to the earth centered frame
  const FGMatrix33& Tl2ec = vLocation.GetTl2ec();

  // Inertial angular velocity measured in the body frame.
  const FGColumnVector3 pqri = vPQR + Tl2b*(Tec2l*omega);

  // Compute the velocity of the vehicle with respect to the earth centered
  // frame expressed in the horizontal local frame.
  vVel = Tb2l * vUVW;



  // First compute the time derivatives of the vehicles' state values.

  // Compute the body rotational accelerations based on the current body moments
  vPQRdot = Jinv*(vMoments - pqri*(J*pqri));

  // Compute the body frame accelerations based on the current body forces
  vUVWdot = vUVW*vPQR + vForces/mass;

  // Centrifugal acceleration.
  FGColumnVector3 ecVel = Tl2ec*vVel;
  FGColumnVector3 ace = 2.0*omega*ecVel;
  vUVWdot -= Tl2b*(Tec2l*ace);
  
  // Coriolis acceleration.
  FGColumnVector3 aeec = omega*(omega*vLocation);
  vUVWdot -= Tl2b*(Tec2l*aeec);

  // Gravitation accel
  double r = GetRadius();
  FGColumnVector3 gAccel( 0.0, 0.0, Inertial->GetGAccel(r) );
  vUVWdot += Tl2b*gAccel;

 

  // Compute the velocity of the vehicle with respect to the earth centered
  // frame expressed in the earth centered frame.
  FGColumnVector3 vLocationDot = Tl2ec * vVel;

  // Compute the angular rate of the local horizontal frame.
  if (r == 0.0) {
    PutMessage( "Fatal error in FGTimestep: "
                "You traversed exactly the middle of the earth, "
                "we tried our best, but this will most likely fail!" );
    r = 1e-16;
  }
  double rInv = 1.0/r;
  FGColumnVector3 omegaLocal( rInv*vVel(eEast),
                              -rInv*vVel(eNorth),
                              -rInv*vVel(eEast)*vLocation.GetTanLatitude() );

  // Compute the quaternion orientation derivative on the current
  // body rotational rates
  FGQuaternion vQtrndot = vQtrn.GetQDot( vPQR - Tl2b*omegaLocal );



  // Now do propagation.
  // This is for now done with a simple explicit euler scheme.
  // That means use the current state values and their current derivatives.
  // Based on these values compute an approximatioin to the state values at the
  // time (now + dt).

  // Propagate the velocities
  vPQR += dt*vPQRdot;
  vUVW += dt*vUVWdot;

  // Propagate the positions
  vQtrn += dt*vQtrndot;
  vLocation += dt*vLocationDot;

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::Seth(double tt)
{
  vLocation.SetRadius( tt + SeaLevelRadius );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetDistanceAGL(double tt)
{
  vLocation.SetRadius( tt + RunwayRadius );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::bind(void)
{
  typedef double (FGPropagate::*PMF)(int) const;
  PropertyManager->Tie("velocities/h-dot-fps", this, &FGPropagate::Gethdot);

  PropertyManager->Tie("velocities/v-north-fps", this, eNorth, (PMF)&FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-east-fps", this, eEast, &FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-down-fps", this, eDown, &FGPropagate::GetVel);

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
  PropertyManager->Tie("position/long-gc-rad", this, &FGPropagate::GetLongitude, &FGPropagate::SetLongitude);
  PropertyManager->Tie("position/h-agl-ft", this,  &FGPropagate::GetDistanceAGL, &FGPropagate::SetDistanceAGL);
  PropertyManager->Tie("position/radius-to-vehicle-ft", this, &FGPropagate::GetRadius);

  PropertyManager->Tie("metrics/runway-radius", this, &FGPropagate::GetRunwayRadius, &FGPropagate::SetRunwayRadius);

  PropertyManager->Tie("attitude/phi-rad", this, &FGPropagate::Getphi);
  PropertyManager->Tie("attitude/theta-rad", this, &FGPropagate::Gettht);
  PropertyManager->Tie("attitude/psi-rad", this, &FGPropagate::Getpsi);

  PropertyManager->Tie("attitude/roll-rad", this, &FGPropagate::Getphi);
  PropertyManager->Tie("attitude/pitch-rad", this, &FGPropagate::Gettht);
  PropertyManager->Tie("attitude/heading-true-rad", this, &FGPropagate::Getpsi);
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
  PropertyManager->Untie("position/long-gc-rad");
  PropertyManager->Untie("position/h-agl-ft");
  PropertyManager->Untie("position/radius-to-vehicle-ft");
  PropertyManager->Untie("metrics/runway-radius");
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
