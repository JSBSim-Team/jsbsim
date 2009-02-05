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

#include <cmath>

#include "FGState.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGState.cpp,v 1.12 2009/02/05 04:59:53 jberndt Exp $";
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
  //dt = 1.0/120.0;

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
  vAeroUVW = Propagate->GetUVW() + Propagate->GetTl2b()*Atmosphere->GetTotalWindNED();

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

void FGState::bind(void)
{
  PropertyManager->Tie("sim-time-sec", this, &FGState::Getsim_time);
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
