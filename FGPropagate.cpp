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

static const char *IdSrc = "$Id: FGPropagate.cpp,v 1.21 2005/04/23 18:16:14 jberndt Exp $";
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

  VState.vLocation.SetRadius( SeaLevelRadius + 4.0 );

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetInitialState(const FGInitialCondition *FGIC)
{
  SeaLevelRadius = FGIC->GetSeaLevelRadiusFtIC();
  RunwayRadius = FGIC->GetSeaLevelRadiusFtIC() + FGIC->GetTerrainAltitudeFtIC();

  // Set the position lat/lon/radius
  VState.vLocation = FGLocation( FGIC->GetLongitudeRadIC(),
                          FGIC->GetLatitudeRadIC(),
                          FGIC->GetAltitudeFtIC() + FGIC->GetSeaLevelRadiusFtIC() );

  // Set the Orientation from the euler angles
  VState.vQtrn = FGQuaternion( FGIC->GetPhiRadIC(),
                        FGIC->GetThetaRadIC(),
                        FGIC->GetPsiRadIC() );

  // Set the velocities in the instantaneus body frame
  VState.vUVW = FGColumnVector3( FGIC->GetUBodyFpsIC(),
                          FGIC->GetVBodyFpsIC(),
                          FGIC->GetWBodyFpsIC() );

  // Set the angular velocities in the instantaneus body frame.
  VState.vPQR = FGColumnVector3( FGIC->GetPRadpsIC(),
                          FGIC->GetQRadpsIC(),
                          FGIC->GetRRadpsIC() );

  // Compute some derived values.
  vVel = VState.vQtrn.GetTInv()*VState.vUVW;

  // Finaly make shure that the quaternion stays normalized.
  VState.vQtrn.Normalize();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
Purpose: Called on a schedule to perform EOM integration
Notes:   [JB] Run in standalone mode, SeaLevelRadius will be reference radius.
         In FGFS, SeaLevelRadius is stuffed from FGJSBSim in JSBSim.cxx each pass.

At the top of this Run() function, see several "shortcuts" (or, aliases) being
set up for use later, rather than using the longer class->function() notation.

Here, propagation of state is done using a simple explicit Euler scheme (see the
bottom of the function). This propagation is done using the current state values
and current derivatives. Based on these values we compute an approximation to the
state values for (now + dt).

*/

bool FGPropagate::Run(void)
{
  if (FGModel::Run()) return true;  // Fast return if we have nothing to do ...

  double dt = State->Getdt()*rate;  // The 'stepsize'
  const FGColumnVector3 omega( 0.0, 0.0, Inertial->omega() ); // earth rotation
  const FGColumnVector3& vForces = Aircraft->GetForces();     // current forces
  const FGColumnVector3& vMoments = Aircraft->GetMoments();   // current moments

  double mass = MassBalance->GetMass();             // mass
  const FGMatrix33& J = MassBalance->GetJ();        // inertia matrix
  const FGMatrix33& Jinv = MassBalance->GetJinv();  // inertia matrix inverse
  double r = GetRadius();                           // radius
  if (r == 0.0) {cerr << "radius = 0 !" << endl; r = 1e-16;} // radius check
  double rInv = 1.0/r;
  FGColumnVector3 gAccel( 0.0, 0.0, Inertial->GetGAccel(r) );

  // The rotation matrices:
  const FGMatrix33& Tl2b = GetTl2b();  // local to body frame
  const FGMatrix33& Tb2l = GetTb2l();  // body to local frame
  const FGMatrix33& Tec2l = VState.vLocation.GetTec2l();  // earth centered to local frame
  const FGMatrix33& Tl2ec = VState.vLocation.GetTl2ec();  // local to earth centered frame

  // Inertial angular velocity measured in the body frame.
  const FGColumnVector3 pqri = VState.vPQR + Tl2b*(Tec2l*omega);

  // Compute vehicle velocity wrt EC frame, expressed in Local horizontal frame.
  vVel = Tb2l * VState.vUVW;

  // First compute the time derivatives of the vehicle state values:

  // Compute body frame rotational accelerations based on the current body moments
  vPQRdot = Jinv*(vMoments - pqri*(J*pqri));

  // Compute body frame accelerations based on the current body forces
  vUVWdot = VState.vUVW*VState.vPQR + vForces/mass;

  // Coriolis acceleration.
  FGColumnVector3 ecVel = Tl2ec*vVel;
  FGColumnVector3 ace = 2.0*omega*ecVel;
  vUVWdot -= Tl2b*(Tec2l*ace);

  // Centrifugal acceleration.
  FGColumnVector3 aeec = omega*(omega*VState.vLocation);
  vUVWdot -= Tl2b*(Tec2l*aeec);

  // Gravitation accel
  vUVWdot += Tl2b*gAccel;

  // Compute vehicle velocity wrt EC frame, expressed in EC frame
  FGColumnVector3 vLocationDot = Tl2ec * vVel;

  FGColumnVector3 omegaLocal( rInv*vVel(eEast),
                              -rInv*vVel(eNorth),
                              -rInv*vVel(eEast)*VState.vLocation.GetTanLatitude() );

  // Compute quaternion orientation derivative on current body rates
  FGQuaternion vQtrndot = VState.vQtrn.GetQDot( VState.vPQR - Tl2b*omegaLocal );

  // Propagate velocities
  VState.vPQR += dt*vPQRdot;
  VState.vUVW += dt*vUVWdot;

  // Propagate positions
  VState.vQtrn += dt*vQtrndot;
  VState.vLocation += dt*vLocationDot;

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::Seth(double tt)
{
  VState.vLocation.SetRadius( tt + SeaLevelRadius );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::SetDistanceAGL(double tt)
{
  VState.vLocation.SetRadius( tt + RunwayRadius );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropagate::bind(void)
{
  typedef double (FGPropagate::*PMF)(int) const;
  PropertyManager->Tie("velocities/h-dot-fps", this, &FGPropagate::Gethdot);

  PropertyManager->Tie("velocities/v-north-fps", this, eNorth, (PMF)&FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-east-fps", this, eEast, (PMF)&FGPropagate::GetVel);
  PropertyManager->Tie("velocities/v-down-fps", this, eDown, (PMF)&FGPropagate::GetVel);

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

  PropertyManager->Tie("attitude/phi-rad", this, (int)ePhi, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/theta-rad", this, (int)eTht, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/psi-rad", this, (int)ePsi, (PMF)&FGPropagate::GetEuler);

  PropertyManager->Tie("attitude/roll-rad", this, (int)ePhi, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/pitch-rad", this, (int)eTht, (PMF)&FGPropagate::GetEuler);
  PropertyManager->Tie("attitude/heading-true-rad", this, (int)ePsi, (PMF)&FGPropagate::GetEuler);
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
