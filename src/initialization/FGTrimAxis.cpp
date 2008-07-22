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
#include <FGFDMExec.h>
#include <models/FGAtmosphere.h>
#include "FGInitialCondition.h"
#include "FGTrimAxis.h"
#include <models/FGAircraft.h>
#include <models/FGPropulsion.h>
#include <models/FGAerodynamics.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGTrimAxis.cpp,v 1.7 2008/07/22 02:42:17 jberndt Exp $";
static const char *IdHdr = ID_TRIMAXIS;

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
    control_value=fdmex->GetPropagate()->GetDistanceAGL();
    solver_eps=tolerance/100;
    break;
  case tTheta:
    control_min=fdmex->GetPropagate()->GetEuler(eTht) - 5*degtorad;
    control_max=fdmex->GetPropagate()->GetEuler(eTht) + 5*degtorad;
    state_convert=radtodeg;
    break;
  case tPhi:
    control_min=fdmex->GetPropagate()->GetEuler(ePhi) - 30*degtorad;
    control_max=fdmex->GetPropagate()->GetEuler(ePhi) + 30*degtorad;
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
    control_min=fdmex->GetPropagate()->GetEuler(ePsi) - 30*degtorad;
    control_max=fdmex->GetPropagate()->GetEuler(ePsi) + 30*degtorad;
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
  case tUdot: state_value=fdmex->GetPropagate()->GetUVWdot(1)-state_target; break;
  case tVdot: state_value=fdmex->GetPropagate()->GetUVWdot(2)-state_target; break;
  case tWdot: state_value=fdmex->GetPropagate()->GetUVWdot(3)-state_target; break;
  case tQdot: state_value=fdmex->GetPropagate()->GetPQRdot(2)-state_target;break;
  case tPdot: state_value=fdmex->GetPropagate()->GetPQRdot(1)-state_target; break;
  case tRdot: state_value=fdmex->GetPropagate()->GetPQRdot(3)-state_target; break;
  case tHmgt: state_value=computeHmgt()-state_target; break;
  case tNlf:  state_value=fdmex->GetAircraft()->GetNlf()-state_target; break;
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

// the aircraft center of rotation is no longer the cg once the gear
// contact the ground so the altitude needs to be changed when pitch
// and roll angle are adjusted.  Instead of attempting to calculate the
// new center of rotation, pick a gear unit as a reference and use its
// location vector to calculate the new height change. i.e. new altitude =
// earth z component of that vector (which is in body axes )
void FGTrimAxis::SetThetaOnGround(double ff) {
  int center,i,ref;

  // favor an off-center unit so that the same one can be used for both
  // pitch and roll.  An on-center unit is used (for pitch)if that's all
  // that's in contact with the ground.
  i=0; ref=-1; center=-1;
  while( (ref < 0) && (i < fdmex->GetGroundReactions()->GetNumGearUnits()) ) {
    if(fdmex->GetGroundReactions()->GetGearUnit(i)->GetWOW()) {
      if(fabs(fdmex->GetGroundReactions()->GetGearUnit(i)->GetBodyLocation(2)) > 0.01)
        ref=i;
      else
        center=i;
    }
    i++;
  }
  if((ref < 0) && (center >= 0)) {
    ref=center;
  }
  cout << "SetThetaOnGround ref gear: " << ref << endl;
  if(ref >= 0) {
    double sp = fdmex->GetPropagate()->GetSinEuler(ePhi);
    double cp = fdmex->GetPropagate()->GetCosEuler(ePhi);
    double lx = fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(1);
    double ly = fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(2);
    double lz = fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(3);
    double hagl = -1*lx*sin(ff) +
                    ly*sp*cos(ff) +
                    lz*cp*cos(ff);

    fgic->SetAltitudeAGLFtIC(hagl);
    cout << "SetThetaOnGround new alt: " << hagl << endl;
  }
  fgic->SetThetaRadIC(ff);
  cout << "SetThetaOnGround new theta: " << ff << endl;
}

/*****************************************************************************/

bool FGTrimAxis::initTheta(void) {
  int i,N;
  int iForward = 0;
  int iAft = 1;
  double zAft,zForward,zDiff,theta;
  double xAft,xForward,xDiff;
  bool level;
  double saveAlt;

  saveAlt=fgic->GetAltitudeAGLFtIC();
  fgic->SetAltitudeAGLFtIC(100);


  N=fdmex->GetGroundReactions()->GetNumGearUnits();

  //find the first wheel unit forward of the cg
  //the list is short so a simple linear search is fine
  for( i=0; i<N; i++ ) {
    if(fdmex->GetGroundReactions()->GetGearUnit(i)->GetBodyLocation(1) > 0 ) {
        iForward=i;
        break;
    }
  }
  //now find the first wheel unit aft of the cg
  for( i=0; i<N; i++ ) {
    if(fdmex->GetGroundReactions()->GetGearUnit(i)->GetBodyLocation(1) < 0 ) {
        iAft=i;
        break;
    }
  }

  // now adjust theta till the wheels are the same distance from the ground
  xAft=fdmex->GetGroundReactions()->GetGearUnit(iAft)->GetBodyLocation(1);
  xForward=fdmex->GetGroundReactions()->GetGearUnit(iForward)->GetBodyLocation(1);
  xDiff = xForward - xAft;
  zAft=fdmex->GetGroundReactions()->GetGearUnit(iAft)->GetLocalGear(3);
  zForward=fdmex->GetGroundReactions()->GetGearUnit(iForward)->GetLocalGear(3);
  zDiff = zForward - zAft;
  level=false;
  theta=fgic->GetThetaDegIC();
  while(!level && (i < 100)) {
    theta+=radtodeg*atan(zDiff/xDiff);
    fgic->SetThetaDegIC(theta);
    fdmex->RunIC();
    zAft=fdmex->GetGroundReactions()->GetGearUnit(iAft)->GetLocalGear(3);
    zForward=fdmex->GetGroundReactions()->GetGearUnit(iForward)->GetLocalGear(3);
    zDiff = zForward - zAft;
    //cout << endl << theta << "  " << zDiff << endl;
    //cout << "0: " << fdmex->GetGroundReactions()->GetGearUnit(0)->GetLocalGear() << endl;
    //cout << "1: " << fdmex->GetGroundReactions()->GetGearUnit(1)->GetLocalGear() << endl;
    if(fabs(zDiff ) < 0.1)
        level=true;
    i++;
  }
  //cout << i << endl;
  if (debug_lvl > 0) {
      cout << "    Initial Theta: " << fdmex->GetPropagate()->GetEuler(eTht)*radtodeg << endl;
      cout << "    Used gear unit " << iAft << " as aft and " << iForward << " as forward" << endl;
  }
  control_min=(theta+5)*degtorad;
  control_max=(theta-5)*degtorad;
  fgic->SetAltitudeAGLFtIC(saveAlt);
  if(i < 100)
    return true;
  else
    return false;
}

/*****************************************************************************/

void FGTrimAxis::SetPhiOnGround(double ff) {
  int i,ref;

  i=0; ref=-1;
  //must have an off-center unit here
  while ( (ref < 0) && (i < fdmex->GetGroundReactions()->GetNumGearUnits()) ) {
    if ( (fdmex->GetGroundReactions()->GetGearUnit(i)->GetWOW()) &&
      (fabs(fdmex->GetGroundReactions()->GetGearUnit(i)->GetBodyLocation(2)) > 0.01))
        ref=i;
    i++;
  }
  if (ref >= 0) {
    double st = fdmex->GetPropagate()->GetSinEuler(eTht);
    double ct = fdmex->GetPropagate()->GetCosEuler(eTht);
    double lx = fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(1);
    double ly = fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(2);
    double lz = fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(3);
    double hagl = -1*lx*st +
                    ly*sin(ff)*ct +
                    lz*cos(ff)*ct;

    fgic->SetAltitudeAGLFtIC(hagl);
  }
  fgic->SetPhiRadIC(ff);

}

/*****************************************************************************/

void FGTrimAxis::Run(void) {

  double last_state_value;
  int i;
  setControl();
  //cout << "FGTrimAxis::Run: " << control_value << endl;
  i=0;
  bool stable=false;
  while(!stable) {
    i++;
    last_state_value=state_value;
    fdmex->RunIC();
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
      //cout << "setThrottlespct: " << i << ", " << control_min << ", " << control_max << ", " << control_value;
      fdmex->GetFCS()->SetThrottleCmd(i,tMin+control_value*(tMax-tMin));
      //cout << "setThrottlespct: " << fdmex->GetFCS()->GetThrottleCmd(i) << endl;
      fdmex->RunIC(); //apply throttle change
      fdmex->GetPropulsion()->GetSteadyState();
  }
}

/*****************************************************************************/

void FGTrimAxis::AxisReport(void) {

  char out[80];

  sprintf(out,"  %20s: %6.2f %5s: %9.2e Tolerance: %3.0e",
           GetControlName().c_str(), GetControl()*control_convert,
           GetStateName().c_str(), GetState()+state_target, GetTolerance());
  cout << out;

  if( fabs(GetState()+state_target) < fabs(GetTolerance()) )
     cout << "  Passed" << endl;
  else
     cout << "  Failed" << endl;
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
    if (from == 0) cout << "Instantiated: FGTrimAxis" << endl;
    if (from == 1) cout << "Destroyed:    FGTrimAxis" << endl;
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
