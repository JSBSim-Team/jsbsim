/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrimAxis.cpp
 Author:       Tony Peden
 Date started: 7/3/00

 --------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) ---------

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


 HISTORY
--------------------------------------------------------------------------------
7/3/00   TP   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef _MSC_VER
#  pragma warning (disable : 4786)
#endif

#include <string>
#include <cstdlib>
#include <iomanip>

#include "FGFDMExec.h"
#include "models/FGAtmosphere.h"
#include "FGInitialCondition.h"
#include "FGTrimAxis.h"
#include "models/FGPropulsion.h"
#include "models/FGAerodynamics.h"
#include "models/FGFCS.h"
#include "models/propulsion/FGEngine.h"
#include "models/FGAuxiliary.h"
#include "models/FGGroundReactions.h"
#include "models/FGPropagate.h"
#include "models/FGAccelerations.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*****************************************************************************/

FGTrimAxis::FGTrimAxis(FGFDMExec* fdex, FGInitialCondition* ic, State st,
                       Control ctrl) {

  fdmex=fdex;
  fgic=ic;
  state=st;
  control=ctrl;
  max_iterations=10;
  control_value=0;
  its_to_stable_value=0;
  total_iterations=0;
  total_stability_iterations=0;
  state_convert=1.0;
  control_convert=1.0;
  state_value=0;
  state_target=0;
  tolerance=DEFAULT_TOLERANCE;
  switch(state) {
    case tUdot: tolerance = DEFAULT_TOLERANCE; break;
    case tVdot: tolerance = DEFAULT_TOLERANCE; break;
    case tWdot: tolerance = DEFAULT_TOLERANCE; break;
    case tQdot: tolerance = DEFAULT_TOLERANCE / 10; break;
    case tPdot: tolerance = DEFAULT_TOLERANCE / 10; break;
    case tRdot: tolerance = DEFAULT_TOLERANCE / 10; break;
    case tHmgt: tolerance = 0.01; break;
    case  tNlf: state_target=1.0; tolerance = 1E-5; break;
    case tAll: break;
  }

  solver_eps=tolerance;
  switch(control) {
  case tThrottle:
    control_min=0;
    control_max=1;
    control_value=0.5;
    break;
  case tBeta:
    control_min=-30*degtorad;
    control_max=30*degtorad;
    control_convert=radtodeg;
    break;
  case tAlpha:
    control_min=fdmex->GetAerodynamics()->GetAlphaCLMin();
    control_max=fdmex->GetAerodynamics()->GetAlphaCLMax();
    if(control_max <= control_min) {
      control_max=20*degtorad;
      control_min=-5*degtorad;
    }
    control_value= (control_min+control_max)/2;
    control_convert=radtodeg;
    solver_eps=tolerance/100;
    break;
  case tPitchTrim:
  case tElevator:
  case tRollTrim:
  case tAileron:
  case tYawTrim:
  case tRudder:
    control_min=-1;
    control_max=1;
    state_convert=radtodeg;
    solver_eps=tolerance/100;
    break;
  case tAltAGL:
    control_min=0;
    control_max=30;
    control_value=ic->GetAltitudeAGLFtIC();
    solver_eps=tolerance/100;
    break;
  case tTheta:
    control_min=ic->GetThetaRadIC() - 5*degtorad;
    control_max=ic->GetThetaRadIC() + 5*degtorad;
    state_convert=radtodeg;
    break;
  case tPhi:
    control_min=ic->GetPhiRadIC() - 30*degtorad;
    control_max=ic->GetPhiRadIC() + 30*degtorad;
    state_convert=radtodeg;
    control_convert=radtodeg;
    break;
  case tGamma:
    solver_eps=tolerance/100;
    control_min=-80*degtorad;
    control_max=80*degtorad;
    control_convert=radtodeg;
    break;
  case tHeading:
    control_min=ic->GetPsiRadIC() - 30*degtorad;
    control_max=ic->GetPsiRadIC() + 30*degtorad;
    state_convert=radtodeg;
    break;
  }


  Debug(0);
}

/*****************************************************************************/

FGTrimAxis::~FGTrimAxis(void)
{
  Debug(1);
}

/*****************************************************************************/

void FGTrimAxis::getState(void) {
  switch(state) {
  case tUdot: state_value=fdmex->GetAccelerations()->GetUVWdot(1)-state_target; break;
  case tVdot: state_value=fdmex->GetAccelerations()->GetUVWdot(2)-state_target; break;
  case tWdot: state_value=fdmex->GetAccelerations()->GetUVWdot(3)-state_target; break;
  case tQdot: state_value=fdmex->GetAccelerations()->GetPQRdot(2)-state_target;break;
  case tPdot: state_value=fdmex->GetAccelerations()->GetPQRdot(1)-state_target; break;
  case tRdot: state_value=fdmex->GetAccelerations()->GetPQRdot(3)-state_target; break;
  case tHmgt: state_value=computeHmgt()-state_target; break;
  case tNlf:  state_value=fdmex->GetAuxiliary()->GetNlf()-state_target; break;
  case tAll: break;
  }
}

/*****************************************************************************/

//States are not settable

void FGTrimAxis::getControl(void) {
  switch(control) {
  case tThrottle:  control_value=fdmex->GetFCS()->GetThrottleCmd(0); break;
  case tBeta:      control_value=fdmex->GetAuxiliary()->Getbeta(); break;
  case tAlpha:     control_value=fdmex->GetAuxiliary()->Getalpha();  break;
  case tPitchTrim: control_value=fdmex->GetFCS() -> GetPitchTrimCmd(); break;
  case tElevator:  control_value=fdmex->GetFCS() -> GetDeCmd(); break;
  case tRollTrim:
  case tAileron:   control_value=fdmex->GetFCS() -> GetDaCmd(); break;
  case tYawTrim:
  case tRudder:    control_value=fdmex->GetFCS() -> GetDrCmd(); break;
  case tAltAGL:    control_value=fdmex->GetPropagate()->GetDistanceAGL();break;
  case tTheta:     control_value=fdmex->GetPropagate()->GetEuler(eTht); break;
  case tPhi:       control_value=fdmex->GetPropagate()->GetEuler(ePhi); break;
  case tGamma:     control_value=fdmex->GetAuxiliary()->GetGamma();break;
  case tHeading:   control_value=fdmex->GetPropagate()->GetEuler(ePsi); break;
  }
}

/*****************************************************************************/

double FGTrimAxis::computeHmgt(void) {
  double diff;

  diff   = fdmex->GetPropagate()->GetEuler(ePsi) -
             fdmex->GetAuxiliary()->GetGroundTrack();

  if( diff < -M_PI ) {
     return (diff + 2*M_PI);
  } else if( diff > M_PI ) {
     return (diff - 2*M_PI);
  } else {
     return diff;
  }

}

/*****************************************************************************/


void FGTrimAxis::setControl(void) {
  switch(control) {
  case tThrottle:  setThrottlesPct(); break;
  case tBeta:      fgic->SetBetaRadIC(control_value); break;
  case tAlpha:     fgic->SetAlphaRadIC(control_value);  break;
  case tPitchTrim: fdmex->GetFCS()->SetPitchTrimCmd(control_value); break;
  case tElevator:  fdmex->GetFCS()->SetDeCmd(control_value); break;
  case tRollTrim:
  case tAileron:   fdmex->GetFCS()->SetDaCmd(control_value); break;
  case tYawTrim:
  case tRudder:    fdmex->GetFCS()->SetDrCmd(control_value); break;
  case tAltAGL:    fgic->SetAltitudeAGLFtIC(control_value); break;
  case tTheta:     fgic->SetThetaRadIC(control_value); break;
  case tPhi:       fgic->SetPhiRadIC(control_value); break;
  case tGamma:     fgic->SetFlightPathAngleRadIC(control_value); break;
  case tHeading:   fgic->SetPsiRadIC(control_value); break;
  }
}

/*****************************************************************************/

void FGTrimAxis::Run(void) {

  double last_state_value;
  int i;
  setControl();
  //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
  //log << "FGTrimAxis::Run: " << control_value << "\n";
  i=0;
  bool stable=false;
  while(!stable) {
    i++;
    last_state_value=state_value;
    fdmex->Initialize(fgic);
    fdmex->Run();
    getState();
    if(i > 1) {
      if((fabs(last_state_value - state_value) < tolerance) || (i >= 100) )
        stable=true;
    }
  }

  its_to_stable_value=i;
  total_stability_iterations+=its_to_stable_value;
  total_iterations++;
}

/*****************************************************************************/

void FGTrimAxis::setThrottlesPct(void) {
  double tMin,tMax;
  for(unsigned i=0;i<fdmex->GetPropulsion()->GetNumEngines();i++) {
      tMin=fdmex->GetPropulsion()->GetEngine(i)->GetThrottleMin();
      tMax=fdmex->GetPropulsion()->GetEngine(i)->GetThrottleMax();

      // Both the main throttle setting in FGFCS and the copy of the position
      // in the Propulsion::Inputs structure need to be set at this time.
      fdmex->GetFCS()->SetThrottleCmd(i,tMin+control_value*(tMax-tMin));
      fdmex->GetPropulsion()->in.ThrottlePos[i] = tMin +control_value*(tMax - tMin);

      fdmex->Initialize(fgic);
      fdmex->Run(); //apply throttle change
      fdmex->GetPropulsion()->GetSteadyState();
  }
}

/*****************************************************************************/

void FGTrimAxis::AxisReport(void) {
  FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
  log << "  " << left << setw(20) << GetControlName() << ": ";
  log << setw(6) << setprecision(2) << GetControl()*control_convert << ' ';
  log << setw(5) << GetStateName() << ": ";
  log << setw(9) << setprecision(2) << scientific << GetState()+state_target;
  log << " Tolerance: " << setw(3) << setprecision(0) << scientific << GetTolerance();

  if( fabs(GetState()+state_target) < fabs(GetTolerance()) )
     log << "  Passed\n";
  else
     log << "  Failed\n";
}

/*****************************************************************************/

double FGTrimAxis::GetAvgStability( void ) {
  if(total_iterations > 0) {
    return double(total_stability_iterations)/double(total_iterations);
  }
  return 0;
}

/*****************************************************************************/
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

void FGTrimAxis::Debug(int from)
{

  if (debug_lvl <= 0) return;
  if (debug_lvl & 1 ) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGTrimAxis\n";
    if (from == 1) log << "Destroyed:    FGTrimAxis\n";
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
