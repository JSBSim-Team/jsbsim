/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGRotation.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Integrates the rotational EOM
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
This class integrates the rotational EOM.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

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

  The order of rotations used in this class corresponds to a 3-2-1 sequence,
  or Y-P-R, or Z-Y-X, if you prefer.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGTranslation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGPropertyManager.h"


static const char *IdSrc = "$Id: FGRotation.cpp,v 1.32 2002/03/09 11:57:55 apeden Exp $";
static const char *IdHdr = ID_ROTATION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGRotation::FGRotation(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGRotation";
  cTht=cPhi=cPsi=1.0;
  sTht=sPhi=sPsi=0.0;
  
  bind();
  
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGRotation::~FGRotation()
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGRotation::Run(void)
{
  double L2, N1;
  double tTheta;

  if (!FGModel::Run()) {
    GetState();

    L2 = vMoments(eL) + Ixz*vPQR(eP)*vPQR(eQ) - (Izz-Iyy)*vPQR(eR)*vPQR(eQ);
    N1 = vMoments(eN) - (Iyy-Ixx)*vPQR(eP)*vPQR(eQ) - Ixz*vPQR(eR)*vPQR(eQ);

    vPQRdot(eP) = (L2*Izz - N1*Ixz) / (Ixx*Izz - Ixz*Ixz);
    vPQRdot(eQ) = (vMoments(eM) - (Ixx-Izz)*vPQR(eP)*vPQR(eR) - Ixz*(vPQR(eP)*vPQR(eP) - vPQR(eR)*vPQR(eR)))/Iyy;
    vPQRdot(eR) = (N1*Ixx + L2*Ixz) / (Ixx*Izz - Ixz*Ixz);

    vPQR += dt*rate*(vlastPQRdot + vPQRdot)/2.0;
    vAeroPQR = vPQR + Atmosphere->GetTurbPQR();

    State->IntegrateQuat(vPQR, rate);
    State->CalcMatrices();
    vEuler = State->CalcEuler();

    cTht = cos(vEuler(eTht));   sTht = sin(vEuler(eTht));
    cPhi = cos(vEuler(ePhi));   sPhi = sin(vEuler(ePhi));
    cPsi = cos(vEuler(ePsi));   sPsi = sin(vEuler(ePsi));

    vEulerRates(eTht) = vPQR(2)*cPhi - vPQR(3)*sPhi;
    if (cTht != 0.0) {
      tTheta = sTht/cTht;       // what's cheaper: / or tan() ?
      vEulerRates(ePhi) = vPQR(1) + (vPQR(2)*sPhi + vPQR(3)*cPhi)*tTheta;
      vEulerRates(ePsi) = (vPQR(2)*sPhi + vPQR(3)*cPhi)/cTht;
    }

    vlastPQRdot = vPQRdot;

    if (debug_lvl > 1) Debug(2);

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGRotation::GetState(void)
{
  dt = State->Getdt();
  vMoments = Aircraft->GetMoments();

  Ixx = MassBalance->GetIxx();
  Iyy = MassBalance->GetIyy();
  Izz = MassBalance->GetIzz();
  Ixz = MassBalance->GetIxz();
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

void FGRotation::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGRotation" << endl;
    if (from == 1) cout << "Destroyed:    FGRotation" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity check variables
    if (from == 2) {
      if (fabs(vPQR(eP)) > 100)
        cout << "FGRotation::P (Roll Rate) out of bounds: " << vPQR(eP) << endl;
      if (fabs(vPQR(eQ)) > 100)
        cout << "FGRotation::Q (Pitch Rate) out of bounds: " << vPQR(eQ) << endl;
      if (fabs(vPQR(eR)) > 100)
        cout << "FGRotation::R (Yaw Rate) out of bounds: " << vPQR(eR) << endl;
    }
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

void FGRotation::bind(void){
  PropertyManager->Tie("velocities/p-rad_sec", this,1,
                       &FGRotation::GetPQR);
  PropertyManager->Tie("velocities/q-rad_sec", this,2,
                       &FGRotation::GetPQR);
  PropertyManager->Tie("velocities/r-rad_sec", this,3,
                       &FGRotation::GetPQR);
  PropertyManager->Tie("velocities/p-aero-rad_sec", this,1,
                       &FGRotation::GetAeroPQR);
  PropertyManager->Tie("velocities/q-aero-rad_sec", this,2,
                       &FGRotation::GetAeroPQR);
  PropertyManager->Tie("velocities/r-aero-rad_sec", this,3,
                       &FGRotation::GetAeroPQR);
  PropertyManager->Tie("accelerations/pdot-rad_sec", this,1,
                       &FGRotation::GetPQRdot);
  PropertyManager->Tie("accelerations/qdot-rad_sec", this,2,
                       &FGRotation::GetPQRdot);
  PropertyManager->Tie("accelerations/rdot-rad_sec", this,3,
                       &FGRotation::GetPQRdot);
  PropertyManager->Tie("attitude/roll-rad", this,1,
                       &FGRotation::GetEuler);
  PropertyManager->Tie("attitude/pitch-rad", this,2,
                       &FGRotation::GetEuler);
  PropertyManager->Tie("attitude/heading-true-rad", this,3,
                       &FGRotation::GetEuler);
  PropertyManager->Tie("velocities/phidot-rad_sec", this,1,
                       &FGRotation::GetEulerRates);
  PropertyManager->Tie("velocities/thetadot-rad_sec", this,2,
                       &FGRotation::GetEulerRates);
  PropertyManager->Tie("velocities/psidot-rad_sec", this,3,
                       &FGRotation::GetEulerRates);
  PropertyManager->Tie("attitude/phi-rad", this,
                       &FGRotation::Getphi);
  PropertyManager->Tie("attitude/theta-rad", this,
                       &FGRotation::Gettht);
  PropertyManager->Tie("attitude/psi-true-rad", this,
                       &FGRotation::Getpsi);
}

void FGRotation::unbind(void){
  PropertyManager->Untie("velocities/p-rad_sec");
  PropertyManager->Untie("velocities/q-rad_sec");
  PropertyManager->Untie("velocities/r-rad_sec");
  PropertyManager->Untie("velocities/p-aero-rad_sec");
  PropertyManager->Untie("velocities/q-aero-rad_sec");
  PropertyManager->Untie("velocities/r-aero-rad_sec");
  PropertyManager->Untie("accelerations/pdot-rad_sec");
  PropertyManager->Untie("accelerations/qdot-rad_sec");
  PropertyManager->Untie("accelerations/rdot-rad_sec");
  PropertyManager->Untie("attitude/roll-rad");
  PropertyManager->Untie("attitude/pitch-rad");
  PropertyManager->Untie("attitude/heading-true-rad");
  PropertyManager->Untie("velocities/phidot-rad_sec");
  PropertyManager->Untie("velocities/thetadot-rad_sec");
  PropertyManager->Untie("velocities/psidot-rad_sec");
  PropertyManager->Untie("attitude/phi-rad");
  PropertyManager->Untie("attitude/theta-rad");
  PropertyManager->Untie("attitude/psi-true-rad");
}
