/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGState.cpp
 Author:       Jon Berndt
 Date started: 11/17/98
 Called by:    FGFDMExec and accessed by all models.

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
See header file.

HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include <math.h>
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#include "FGState.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGState.cpp,v 1.7 2008/01/01 15:52:23 jberndt Exp $";
static const char *IdHdr = ID_STATE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
MACROS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGState::FGState(FGFDMExec* fdex)
{
  FDMExec = fdex;

  sim_time = 0.0;
  dt = 1.0/120.0;

  Aircraft     = FDMExec->GetAircraft();
  Propagate    = FDMExec->GetPropagate();
  Auxiliary    = FDMExec->GetAuxiliary();
  FCS          = FDMExec->GetFCS();
  Atmosphere   = FDMExec->GetAtmosphere();
  Aerodynamics = FDMExec->GetAerodynamics();
  GroundReactions = FDMExec->GetGroundReactions();
  Propulsion      = FDMExec->GetPropulsion();
  PropertyManager = FDMExec->GetPropertyManager();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGState::~FGState()
{
  unbind();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::Initialize(FGInitialCondition *FGIC)
{
  sim_time = 0.0;

  Propagate->SetInitialState( FGIC );

  Atmosphere->Run();
  Atmosphere->SetWindNED( FGIC->GetWindNFpsIC(),
                          FGIC->GetWindEFpsIC(),
                          FGIC->GetWindDFpsIC() );

  FGColumnVector3 vAeroUVW;
  vAeroUVW = Propagate->GetUVW() + Propagate->GetTl2b()*Atmosphere->GetWindNED();

  double alpha, beta;
  if (vAeroUVW(eW) != 0.0)
    alpha = vAeroUVW(eU)*vAeroUVW(eU) > 0.0 ? atan2(vAeroUVW(eW), vAeroUVW(eU)) : 0.0;
  else
    alpha = 0.0;
  if (vAeroUVW(eV) != 0.0)
    beta = vAeroUVW(eU)*vAeroUVW(eU)+vAeroUVW(eW)*vAeroUVW(eW) > 0.0 ? atan2(vAeroUVW(eV), (fabs(vAeroUVW(eU))/vAeroUVW(eU))*sqrt(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW))) : 0.0;
  else
    beta = 0.0;

  Auxiliary->SetAB(alpha, beta);

  double Vt = vAeroUVW.Magnitude();
  Auxiliary->SetVt(Vt);

  Auxiliary->SetMach(Vt/Atmosphere->GetSoundSpeed());

  double qbar = 0.5*Vt*Vt*Atmosphere->GetDensity();
  Auxiliary->Setqbar(qbar);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// From Stevens and Lewis, "Aircraft Control and Simulation", 3rd Ed., the
// transformation from body to wind axes is defined (where "a" is alpha and "B"
// is beta):
//
//   cos(a)*cos(B)     sin(B)    sin(a)*cos(B)
//  -cos(a)*sin(B)     cos(B)   -sin(a)*sin(B)
//  -sin(a)              0       cos(a)
//
// The transform from wind to body axes is then,
//
//   cos(a)*cos(B)  -cos(a)*sin(B)  -sin(a)
//          sin(B)          cos(B)     0
//   sin(a)*cos(B)  -sin(a)*sin(B)   cos(a)

FGMatrix33& FGState::GetTw2b(void)
{
  double ca, cb, sa, sb;

  double alpha = Auxiliary->Getalpha();
  double beta  = Auxiliary->Getbeta();

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTw2b(1,1) = ca*cb;
  mTw2b(1,2) = -ca*sb;
  mTw2b(1,3) = -sa;
  mTw2b(2,1) = sb;
  mTw2b(2,2) = cb;
  mTw2b(2,3) = 0.0;
  mTw2b(3,1) = sa*cb;
  mTw2b(3,2) = -sa*sb;
  mTw2b(3,3) = ca;

  return mTw2b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGState::GetTb2w(void)
{
  double alpha,beta;
  double ca, cb, sa, sb;

  alpha = Auxiliary->Getalpha();
  beta  = Auxiliary->Getbeta();

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTb2w(1,1) = ca*cb;
  mTb2w(1,2) = sb;
  mTb2w(1,3) = sa*cb;
  mTb2w(2,1) = -ca*sb;
  mTb2w(2,2) = cb;
  mTb2w(2,3) = -sa*sb;
  mTb2w(3,1) = -sa;
  mTb2w(3,2) = 0.0;
  mTb2w(3,3) = ca;

  return mTb2w;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::bind(void)
{
  PropertyManager->Tie("sim-time-sec", this, &FGState::Getsim_time);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::unbind(void)
{
  PropertyManager->Untie("sim-time-sec");
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

void FGState::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGState" << endl;
    if (from == 1) cout << "Destroyed:    FGState" << endl;
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
