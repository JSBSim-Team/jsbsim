/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrimAnalysisControl.cpp
 Author:       Agostino De Marco
 Date started: Dec/14/2006

 ------------- Copyright (C) 2006  Agostino De Marco (agodemar@unina.it) -------

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
12/14/06   ADM   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef _MSC_VER
#  pragma warning (disable : 4786)
#endif

#include <string>
#include <stdlib.h>

#include "FGFDMExec.h"
#include "models/FGAtmosphere.h"
#include "initialization/FGInitialCondition.h"
#include "FGTrimAnalysisControl.h"
#include "models/FGAircraft.h"
#include "models/FGPropulsion.h"
#include "models/FGAerodynamics.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGTrimAnalysisControl.cpp,v 1.5 2012/09/05 21:49:19 bcoconni Exp $";
static const char *IdHdr = ID_TRIMANALYSISCONTROL;

/*****************************************************************************/

FGTrimAnalysisControl::FGTrimAnalysisControl(FGFDMExec* fdex, FGInitialCondition* ic,
                                             //State st,
                                             TaControl ctrl) {
  fdmex=fdex;
  fgic=ic;
  state=taAll;
  control=ctrl;
  control_initial_value = 0.;
  control_value=0;
  state_convert=1.0;
  control_convert=1.0;
  state_value=0;
  state_target=0;
  control_tolerance = DEFAULT_TOLERANCE;

  switch(control) {
  case taThrottle:
    control_min=0;
    control_max=1;
    control_step=0.2;
    control_initial_value = 0.5;
    control_value=control_initial_value;
    control_name = "Throttle (cmd,norm)";
    break;
  case taBeta:
    control_min=-30*degtorad;
    control_max=30*degtorad;
    control_step=1*degtorad;
    control_convert=radtodeg;
    break;
  case taAlpha:
    control_min=fdmex->GetAerodynamics()->GetAlphaCLMin();
    control_max=fdmex->GetAerodynamics()->GetAlphaCLMax();
    if(control_max <= control_min) {
      control_max=20*degtorad;
      control_min=-5*degtorad;
    }
    control_step=1*degtorad;
    control_initial_value = (control_min+control_max)/2;
    control_value= control_initial_value;
    control_convert=radtodeg;
    break;
  case taPitchTrim:
    control_name = "Pitch Trim (cmd,norm)";
    control_min=-1;
    control_max=1;
    control_step=0.1;
    state_convert=radtodeg;
    break;
  case taElevator:
    control_name = "Elevator (cmd,norm)";
    control_min=-1;
    control_max=1;
    control_step=0.1;
    state_convert=radtodeg;
    break;
  case taRollTrim:
    control_name = "Roll Trim (cmd,norm)";
    control_min=-1;
    control_max=1;
    control_step=0.1;
    state_convert=radtodeg;
    break;
  case taAileron:
    control_name = "Ailerons (cmd,norm)";
    control_min=-1;
    control_max=1;
    control_step=0.1;
    state_convert=radtodeg;
    break;
  case taYawTrim:
    control_name = "Yaw Trim (cmd,norm)";
    control_min=-1;
    control_max=1;
    control_step=0.1;
    state_convert=radtodeg;
    break;
  case taRudder:
    control_name = "Rudder (cmd,norm)";
    control_min=-1;
    control_max=1;
    control_step=0.1;
    state_convert=radtodeg;
    break;
  case taAltAGL:
    control_name = "Altitude (ft)";
    control_min=0;
    control_max=30;
    control_step=2;
    control_initial_value = fdmex->GetPropagate()->GetDistanceAGL();
    control_value = control_initial_value;
    break;
  case taPhi:
    control_name = "Phi (rad)";
    control_min=fdmex->GetPropagate()->GetEuler(ePhi) - 30*degtorad;
    control_max=fdmex->GetPropagate()->GetEuler(ePhi) + 30*degtorad;
    control_step=1*degtorad;
    state_convert=radtodeg;
    control_convert=radtodeg;
    break;
  case taTheta:
    control_name = "Theta (rad)";
    control_min=fdmex->GetPropagate()->GetEuler(eTht) - 5*degtorad;
    control_max=fdmex->GetPropagate()->GetEuler(eTht) + 5*degtorad;
    control_step=1*degtorad;
    state_convert=radtodeg;
    break;
  case taHeading:
    control_name = "Heading (rad)";
    control_min=fdmex->GetPropagate()->GetEuler(ePsi) - 30*degtorad;
    control_max=fdmex->GetPropagate()->GetEuler(ePsi) + 30*degtorad;
    control_step=1*degtorad;
    state_convert=radtodeg;
    break;
  case taGamma:
    control_name = "Gamma (rad)";
    control_min=-80*degtorad;
    control_max=80*degtorad;
    control_step=1*degtorad;
    control_convert=radtodeg;
    break;
  }

//  if (debug_lvl > 0)
//    cout << "FGTrimAnalysisControl created: "<< control_name << endl;

  Debug(0);
}

/*****************************************************************************/

FGTrimAnalysisControl::~FGTrimAnalysisControl(void)
{
  Debug(1);
}

/*****************************************************************************/

void FGTrimAnalysisControl::getState(void) {
  switch(state) {
  case taUdot: state_value=fdmex->GetPropagate()->GetUVWdot(1)-state_target; break;
  case taVdot: state_value=fdmex->GetPropagate()->GetUVWdot(2)-state_target; break;
  case taWdot: state_value=fdmex->GetPropagate()->GetUVWdot(3)-state_target; break;
  case taPdot: state_value=fdmex->GetPropagate()->GetPQRdot(1)-state_target; break;
  case taQdot: state_value=fdmex->GetPropagate()->GetPQRdot(2)-state_target;break;
  case taRdot: state_value=fdmex->GetPropagate()->GetPQRdot(3)-state_target; break;
  case taHmgt: state_value=computeHmgt()-state_target; break;
  case taNlf:  state_value=fdmex->GetAircraft()->GetNlf()-state_target; break;
  case taAll: break;
  }
}

/*****************************************************************************/

//States are not settable

void FGTrimAnalysisControl::getControl(void) {
  switch(control) {
  case taThrottle:  control_value=fdmex->GetFCS()->GetThrottleCmd(0); break;
  case taBeta:      control_value=fdmex->GetAuxiliary()->Getbeta(); break;
  case taAlpha:     control_value=fdmex->GetAuxiliary()->Getalpha();  break;
  case taPitchTrim: control_value=fdmex->GetFCS()->GetPitchTrimCmd(); break;
  case taElevator:  control_value=fdmex->GetFCS()->GetDeCmd(); break;
  case taRollTrim:
  case taAileron:   control_value=fdmex->GetFCS()->GetDaCmd(); break;
  case taYawTrim:
  case taRudder:    control_value=fdmex->GetFCS()->GetDrCmd(); break;
  case taAltAGL:    control_value=fdmex->GetPropagate()->GetDistanceAGL();break;
  case taTheta:     control_value=fdmex->GetPropagate()->GetEuler(eTht); break;
  case taPhi:       control_value=fdmex->GetPropagate()->GetEuler(ePhi); break;
  case taGamma:     control_value=fdmex->GetAuxiliary()->GetGamma();break;
  case taHeading:   control_value=fdmex->GetPropagate()->GetEuler(ePsi); break;
  }
}

/*****************************************************************************/

double FGTrimAnalysisControl::computeHmgt(void) {
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


void FGTrimAnalysisControl::setControl(void) {
  switch(control) {
  case taThrottle:  setThrottlesPct(); break;
  case taBeta:      fgic->SetBetaRadIC(control_value); break;
  case taAlpha:     fgic->SetAlphaRadIC(control_value);  break;
  case taPitchTrim: fdmex->GetFCS()->SetPitchTrimCmd(control_value); break;
  case taElevator:  fdmex->GetFCS()->SetDeCmd(control_value); break;
  case taRollTrim:
  case taAileron:   fdmex->GetFCS()->SetDaCmd(control_value); break;
  case taYawTrim:
  case taRudder:    fdmex->GetFCS()->SetDrCmd(control_value); break;
  case taAltAGL:    fgic->SetAltitudeAGLFtIC(control_value); break;
  case taTheta:     fgic->SetThetaRadIC(control_value); break;
  case taPhi:       fgic->SetPhiRadIC(control_value); break;
  case taGamma:     fgic->SetFlightPathAngleRadIC(control_value); break;
  case taHeading:   fgic->SetPsiRadIC(control_value); break;
  }
}





/*****************************************************************************/

// the aircraft center of rotation is no longer the cg once the gear
// contact the ground so the altitude needs to be changed when pitch
// and roll angle are adjusted.  Instead of attempting to calculate the
// new center of rotation, pick a gear unit as a reference and use its
// location vector to calculate the new height change. i.e. new altitude =
// earth z component of that vector (which is in body axes )
void FGTrimAnalysisControl::SetThetaOnGround(double ff) {
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

bool FGTrimAnalysisControl::initTheta(void) {
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
    fdmex->SuspendIntegration();
    fdmex->Initialize(fgic);
    fdmex->Run();
    fdmex->ResumeIntegration();
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

void FGTrimAnalysisControl::SetPhiOnGround(double ff) {
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

void FGTrimAnalysisControl::Run(void) {

    // ... what's going on here ??
}

/*****************************************************************************/

void FGTrimAnalysisControl::setThrottlesPct(void) {
  double tMin,tMax;
  for(unsigned i=0;i<fdmex->GetPropulsion()->GetNumEngines();i++) {
      tMin=fdmex->GetPropulsion()->GetEngine(i)->GetThrottleMin();
      tMax=fdmex->GetPropulsion()->GetEngine(i)->GetThrottleMax();
      //cout << "setThrottlespct: " << i << ", " << control_min << ", " << control_max << ", " << control_value;
      fdmex->GetFCS()->SetThrottleCmd(i,tMin+control_value*(tMax-tMin));
      //cout << "setThrottlespct: " << fdmex->GetFCS()->GetThrottleCmd(i) << endl;
      fdmex->SuspendIntegration();
      fdmex->Initialize(fgic);
      fdmex->Run();
      fdmex->ResumeIntegration();
      fdmex->GetPropulsion()->GetSteadyState();
  }
}

/*****************************************************************************/


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

void FGTrimAnalysisControl::Debug(int from)
{

  if (debug_lvl <= 0) return;
  if (debug_lvl & 1 ) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTrimAnalysisControl" << endl;
    if (from == 1) cout << "Destroyed:    FGTrimAnalysisControl" << endl;
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
