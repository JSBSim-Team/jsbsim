/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Header:       FGTrimAxis.cpp
 Author:       Tony Peden
 Date started: 7/3/00
 
 --------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) ---------
 
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
 
 
 HISTORY
--------------------------------------------------------------------------------
7/3/00   TP   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <stdlib.h>

#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGInitialCondition.h"
#include "FGTrimAxis.h"
#include "FGAircraft.h"
#include "FGPropulsion.h"

static const char *IdSrc = "$Id: FGTrimAxis.cpp,v 1.27 2001/11/30 12:47:39 apeden Exp $";
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
    control_min=fdmex->GetAircraft()->GetAlphaCLMin();
    control_max=fdmex->GetAircraft()->GetAlphaCLMax();
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
    control_value=fdmex->GetPosition()->GetDistanceAGL();
    solver_eps=tolerance/100;
    break;
  case tTheta:
    control_min=fdmex->GetRotation()->Gettht() - 5*degtorad;
    control_max=fdmex->GetRotation()->Gettht() + 5*degtorad;
    state_convert=radtodeg;
    break;
  case tPhi:
    control_min=fdmex->GetRotation()->Getphi() - 30*degtorad;
    control_max=fdmex->GetRotation()->Getphi() + 30*degtorad;
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
    control_min=fdmex->GetRotation()->Getpsi() - 30*degtorad;
    control_max=fdmex->GetRotation()->Getpsi() + 30*degtorad;
    state_convert=radtodeg;
    break;
  }
  
  
  if (debug_lvl & 2) cout << "Instantiated: FGTrimAxis" << endl;
}

/*****************************************************************************/

FGTrimAxis::~FGTrimAxis()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGTrimAxis" << endl;
}

/*****************************************************************************/

void FGTrimAxis::getState(void) {
  switch(state) {
  case tUdot: state_value=fdmex->GetTranslation()->GetUVWdot()(1)-state_target; break;
  case tVdot: state_value=fdmex->GetTranslation()->GetUVWdot()(2)-state_target; break;
  case tWdot: state_value=fdmex->GetTranslation()->GetUVWdot()(3)-state_target; break;
  case tQdot: state_value=fdmex->GetRotation()->GetPQRdot(2)-state_target;break;
  case tPdot: state_value=fdmex->GetRotation()->GetPQRdot(1)-state_target; break;
  case tRdot: state_value=fdmex->GetRotation()->GetPQRdot(3)-state_target; break;
  case tHmgt: state_value=computeHmgt()-state_target; break;
  case tNlf:  state_value=fdmex->GetAircraft()->GetNlf()-state_target; break;
  }
}

/*****************************************************************************/

//States are not settable

void FGTrimAxis::getControl(void) {
  switch(control) {
  case tThrottle:  control_value=fdmex->GetFCS()->GetThrottleCmd(0); break;
  case tBeta:      control_value=fdmex->GetTranslation()->Getalpha(); break;
  case tAlpha:     control_value=fdmex->GetTranslation()->Getbeta();  break;
  case tPitchTrim: control_value=fdmex->GetFCS() -> GetPitchTrimCmd(); break;
  case tElevator:  control_value=fdmex->GetFCS() -> GetDeCmd(); break;
  case tRollTrim:
  case tAileron:   control_value=fdmex->GetFCS() -> GetDaCmd(); break;
  case tYawTrim:
  case tRudder:    control_value=fdmex->GetFCS() -> GetDrCmd(); break;
  case tAltAGL:    control_value=fdmex->GetPosition()->GetDistanceAGL();break;
  case tTheta:     control_value=fdmex->GetRotation()->Gettht(); break;
  case tPhi:       control_value=fdmex->GetRotation()->Getphi(); break;
  case tGamma:     control_value=fdmex->GetPosition()->GetGamma();break;
  case tHeading:   control_value=fdmex->GetRotation()->Getpsi(); break;
  }
}

/*****************************************************************************/

double FGTrimAxis::computeHmgt(void) {
  double diff;
  
  diff   = fdmex->GetRotation()->Getpsi() - 
             fdmex->GetPosition()->GetGroundTrack();
  
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
  case tTheta:     fgic->SetPitchAngleRadIC(control_value); break;
  case tPhi:       fgic->SetRollAngleRadIC(control_value); break;
  case tGamma:     fgic->SetFlightPathAngleRadIC(control_value); break;
  case tHeading:   fgic->SetTrueHeadingRadIC(control_value); break;
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
    double sp=fdmex->GetRotation()->GetSinphi();
    double cp=fdmex->GetRotation()->GetCosphi();
    double lx=fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(1);
    double ly=fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(2);
    double lz=fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(3);
    double hagl = -1*lx*sin(ff) +
                    ly*sp*cos(ff) +
                    lz*cp*cos(ff);
   
    fgic->SetAltitudeAGLFtIC(hagl);
    cout << "SetThetaOnGround new alt: " << hagl << endl;
  }                   
  fgic->SetPitchAngleRadIC(ff);  
  cout << "SetThetaOnGround new theta: " << ff << endl;      
}      

/*****************************************************************************/

bool FGTrimAxis::initTheta(void) {
  int i,N,iAft, iForward;
  double zAft,zForward,zDiff,theta;
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
  zAft=fdmex->GetGroundReactions()->GetGearUnit(1)->GetLocalGear(3);
  zForward=fdmex->GetGroundReactions()->GetGearUnit(0)->GetLocalGear(3);
  zDiff = zForward - zAft;
  level=false;
  theta=fgic->GetPitchAngleDegIC(); 
  while(!level && (i < 100)) {
	theta+=2.0*zDiff;
	fgic->SetPitchAngleDegIC(theta);   
	fdmex->RunIC(fgic);
	zAft=fdmex->GetGroundReactions()->GetGearUnit(1)->GetLocalGear(3);
        zForward=fdmex->GetGroundReactions()->GetGearUnit(0)->GetLocalGear(3);
        zDiff = zForward - zAft;
	//cout << endl << theta << "  " << zDiff << endl;
	//cout << "0: " << fdmex->GetGroundReactions()->GetGearUnit(0)->GetLocalGear() << endl;
	//cout << "1: " << fdmex->GetGroundReactions()->GetGearUnit(1)->GetLocalGear() << endl;

	if(fabs(zDiff ) < 0.1) 
	    level=true;
	i++;   
  }	    	    	
  //cout << i << endl;
  cout << "    Initial Theta: " << fdmex->GetRotation()->Gettht()*radtodeg << endl;
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
  while( (ref < 0) && (i < fdmex->GetGroundReactions()->GetNumGearUnits()) ) {
    if( (fdmex->GetGroundReactions()->GetGearUnit(i)->GetWOW()) && 
      (fabs(fdmex->GetGroundReactions()->GetGearUnit(i)->GetBodyLocation(2)) > 0.01))
        ref=i;
    i++; 
  }
  if(ref >= 0) {
    double st=fdmex->GetRotation()->GetSintht();
    double ct=fdmex->GetRotation()->GetCostht();
    double lx=fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(1);
    double ly=fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(2);
    double lz=fdmex->GetGroundReactions()->GetGearUnit(ref)->GetBodyLocation(3);
    double hagl = -1*lx*st +
                    ly*sin(ff)*ct +
                    lz*cos(ff)*ct;
   
    fgic->SetAltitudeAGLFtIC(hagl);
  }                   
  fgic->SetRollAngleRadIC(ff);
           
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
    fdmex->RunIC(fgic);
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
      fdmex->RunIC(fgic); //apply throttle change
      fdmex->GetPropulsion()->GetSteadyState();
  }
}

/*****************************************************************************/

void FGTrimAxis::AxisReport(void) {
  
  char out[80];
  sprintf(out,"  %20s: %6.2f %5s: %9.2e Tolerance: %3.0e\n",
           GetControlName().c_str(), GetControl()*control_convert,
           GetStateName().c_str(), GetState()+state_target, GetTolerance()); 
  cout << out;

}

/*****************************************************************************/

double FGTrimAxis::GetAvgStability( void ) {
  if(total_iterations > 0) {
    return double(total_stability_iterations)/double(total_iterations);
  }
  return 0;
}

/*****************************************************************************/

void FGTrimAxis::Debug(void)
{
}

