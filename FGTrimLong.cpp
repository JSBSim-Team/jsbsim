/*******************************************************************************
 
 Header:       FGTrimLong.cpp
 Author:       Tony Peden
 Date started: 9/8/99
 
 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------
 
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
9/8/99   TP   Created
 
 
FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
 
This class takes the given set of IC's and finds the angle of attack, elevator,
and throttle setting required to fly steady level. This is currently for in-air
conditions only.  It is implemented using an iterative, one-axis-at-a-time 
scheme. */




/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGInitialCondition.h"
#include "FGTrimLong.h"
#include "FGAircraft.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/





FGTrimLong::FGTrimLong(FGFDMExec *FDMExec,FGInitialCondition *FGIC ) {

  Ncycles=100;
  Naxis=40;
  Tolerance=1E-3;
  A_Tolerance = Tolerance / 10;
  alphaMin=-5;
  alphaMax=20;
  Debug=0;
  fdmex=FDMExec;
  fgic=FGIC;
  udotf=&FGTrimLong::udot_func;
  wdotf=&FGTrimLong::wdot_func;
  qdotf=&FGTrimLong::qdot_func;
  total_its=0;
  udot_subits=wdot_subits=qdot_subits=0;
  trimudot=true;


}

FGTrimLong::~FGTrimLong(void) {}



void FGTrimLong::TrimStats() {
  cout << endl << "Trim Statistics: " << endl;
  cout << "  Total Iterations: " << total_its << endl;
  if(total_its > 0) {
    cout << "  Sub-iterations:" << endl;
    cout << "    wdot: " << wdot_subits << " average: " << wdot_subits/total_its << endl;
    cout << "    udot: " << udot_subits << " average: " << udot_subits/total_its << endl;
    cout << "    qdot: " << qdot_subits << " average: " << qdot_subits/total_its << endl;
  }
}

void FGTrimLong::Report(void) {
  cout << endl << "JSBSim Trim Report" << endl;
  cout << "  Weight: " << fdmex->GetAircraft()->GetWeight()
  << " lbs.  CG x,y,z: " << fdmex->GetAircraft()->GetXYZcg()
  << " inches " << endl;

  cout << "  Flaps: ";
  float flaps=fdmex->GetFCS()->GetDfPos();
  if(flaps <= 0.01)
    cout << "Up";
  else
    cout << flaps;

  cout << "  Gear: ";
  if(fdmex->GetAircraft()->GetGearUp() == true)
    cout << "Up" << endl;
  else
    cout << "Down" << endl;

  cout << "  Speed: " << fdmex->GetAuxiliary()->GetVcalibratedKTS()
  << " KCAS  Mach: " << fdmex->GetState()->GetParameter(FG_MACH)
  << endl;

  cout << "  Altitude: " << fdmex->GetPosition()->Geth() << " ft" << endl;


  cout << "  Pitch Angle: " << fdmex->GetRotation()->Gettht()*RADTODEG
  << " deg  Angle of Attack: " << fdmex->GetState()->GetParameter(FG_ALPHA)*RADTODEG
  << " deg" << endl;

  float vt=fdmex->GetTranslation()->GetVt();
  if(vt > 0 ) {
    cout << "  Flight Path Angle: "
    << asin(fdmex->GetPosition()->Gethdot()/vt)*RADTODEG
    << " deg" << endl;
  }

  cout << "  Pitch Rate: " << fdmex->GetState()->GetParameter(FG_PITCHRATE)*RADTODEG
  << " deg/s" << endl;

  cout << "  Roll Angle: " << fdmex->GetRotation()->Getphi()*RADTODEG
  << " deg  Roll Rate: " << fdmex->GetState()->GetParameter(FG_ROLLRATE)
  << " deg/s"
  << endl ;

  cout << "  Sideslip: " << fdmex->GetState()->GetParameter(FG_BETA) *RADTODEG
  << " deg  Yaw Rate: " << fdmex->GetState()->GetParameter(FG_YAWRATE)*RADTODEG
  << " deg/s " << endl;

  cout << "  Elevator: " << fdmex->GetState()->GetParameter(FG_ELEVATOR_POS)*RADTODEG
  << "  deg  Ailerons: " << fdmex->GetState()->GetParameter(FG_AILERON_POS)*RADTODEG
  << "  deg  Rudder: " << fdmex->GetState()->GetParameter(FG_RUDDER_POS)*RADTODEG
  << " deg" << endl;
}

void FGTrimLong::setThrottlesPct(float tt) {

  float tMin,tMax;
  for(int i=0;i<fdmex->GetAircraft()->GetNumEngines();i++) {
    tMin=fdmex->GetAircraft()->GetEngine(i)->GetThrottleMin();
    tMax=fdmex->GetAircraft()->GetEngine(i)->GetThrottleMax();
    fdmex -> GetFCS() -> SetThrottleCmd(i,tMin+tt*(tMax-tMin));
  }
}


bool FGTrimLong::checkLimits(trimfp fp, float current, float min, float max) {
  float lo,hi;
  bool result;
  lo=(this->*fp)(min);
  hi=(this->*fp)(max);

  if(lo*hi >= 0) {
    cout << "Lo: " << lo << " Hi: " << hi << endl;
    result=false;
  } else
    result=true;
  //put the control back to where we found it.
  //the value of lo is meaningless
  lo=(this->*fp)(current);
  return result;
}

bool FGTrimLong::solve(trimfp fp,float guess,float desired, float *result, float eps, int max_iterations, int *actual_its) {

  float x1,x2,x3,f1,f2,f3,d,d0;
  float const relax =0.9;

  int i;
  d=1;
  bool success=false;
  //initializations
  if(findInterval(fp,&x1,&x3,guess,desired,max_iterations)) {

    f1=(this->*fp)(x1)-desired;
    f3=(this->*fp)(x3)-desired;
    d0=fabs(x3-x1);

    //iterations
    i=0;
    while ((fabs(d) > eps) && (i < max_iterations)) {
      if(Debug > 1)
        cout << "FGTrimLong::solve i,x1,x2,x3: " << i << ", " << x1 << ", " << x2 << ", " << x3 << endl;

      d=(x3-x1)/d0;
      x2=x1-d*d0*f1/(f3-f1);
      f2=(this->*fp)(x2)-desired;
      if(f1*f2 <= 0.0) {
        x3=x2;
        f3=f2;
        f1=relax*f1;
      } else if(f2*f3 <= 0) {
        x1=x2;
        f1=f2;
        f3=relax*f3;
      }
      //cout << i << endl;
      i++;
    }//end while
    if(i < max_iterations) {
      success=true;
      *result=x2;
    }
    *actual_its=i;
  } else {
    *actual_its=0;
  }
  return success;
}

bool FGTrimLong::findInterval(trimfp fp, float *lo, float *hi,float guess,float desired,int max_iterations) {

  int i=0;
  bool found=false;
  float flo,fhi,fguess;
  float xlo,xhi,step;
  step=0.1*guess;
  fguess=(this->*fp)(guess)-desired;
  xlo=xhi=guess;
  do {
    step=2*step;
    xlo-=step;
    xhi+=step;
    i++;
    flo=(this->*fp)(xlo)-desired;
    fhi=(this->*fp)(xhi)-desired;
    if(flo*fhi <=0) {  //found interval with root
      found=true;
      if(flo*fguess <= 0) {  //narrow interval down a bit
        xhi=xlo+step;    //to pass solver interval that is as
        //small as possible
      }
      else if(fhi*fguess <= 0) {
        xlo=xhi-step;
      }
    }
    if(Debug > 1)
      cout << "FGTrimLong::findInterval: i=" << i << " Lo= " << xlo << " Hi= " << xhi << " flo*fhi: " << flo*fhi << endl;
  } while((found == 0) && (i <= max_iterations));
  *lo=xlo;
  *hi=xhi;
  return found;
}

float FGTrimLong::udot_func(float x) {
  setThrottlesPct(x);
  fdmex->RunIC(fgic);
  return fdmex->GetTranslation()->GetUVWdot()(1);
}

float FGTrimLong::wdot_func(float x) {
  fgic->SetAlphaDegIC(x);
  fdmex->RunIC(fgic);
  return fdmex->GetTranslation()->GetUVWdot()(3);
}

float FGTrimLong::qdot_func(float x) {
  fdmex->GetFCS()->SetDeCmd(x);
  fdmex->RunIC(fgic);
  return fdmex->GetRotation()->GetPQRdot()(2);
}

bool FGTrimLong::DoTrim(void) {
  int k=0,j=0,sum=0,trim_failed=0,jmax=Naxis;
  int its;
  float step,temp,min,max;
  float alpha,de,dth;

  trimfp fp;

  alpha=de=dth=0;
  fgic -> SetAlphaDegIC(alpha);
  fdmex -> GetFCS() -> SetDeCmd(de);
  setThrottlesPct(dth);
  fdmex -> RunIC(fgic);

  min=alphaMin;
  max=alphaMax;
  cout << "Entering Main trim loop" << endl;
  if(trimudot == false)
    udot=0;
  do {
    solve(wdotf,5,0,&wdot,Tolerance,Naxis,&its);
    wdot_subits+=its;
    if(Debug > 0) {
      cout << "Alpha: " << fdmex->GetTranslation()->Getalpha()*RADTODEG
      << " wdot: " << fdmex->GetTranslation()->GetUVWdot()(3)
      << endl;
    }

    if(trimudot == true) {
      solve(udotf,0.5,0,&udot,Tolerance,Naxis,&its);
      udot_subits+=its;
      if(Debug > 0) {
        cout << "Throttle: " << fdmex->GetFCS()->GetThrottlePos(0)
        << " udot: " << fdmex->GetTranslation()->GetUVWdot()(1)
        << endl;
      }
    }
    solve(qdotf,0.5,0,&qdot,A_Tolerance,Naxis,&its);
    qdot_subits+=its;
    if(Debug > 0) {
      cout << "Elevator: " << fdmex->GetFCS()->GetDePos()
      << " qdot: " << fdmex->GetRotation()->GetPQRdot()(2)*RADTODEG
      << endl;
    }
    wdot=fdmex->GetTranslation()->GetUVWdot()(3);
    qdot=fdmex->GetRotation()->GetPQRdot()(2);
    if(trimudot == true)
      udot=fdmex->GetTranslation()->GetUVWdot()(1);

    if((Ncycles > 3) && (k == Ncycles-2) ) {

      if((fabs(wdot) > Tolerance) || (fabs(udot) > Tolerance) || (fabs(qdot) > A_Tolerance)) {
        //failure is imminent, try to show the user why
        cout << endl << "Trimming routine failure is imminent" << endl;
        cout << "Hmm...let's see here" << endl;
        if(wdot > Tolerance)
          cout << "  wdot is too high: " << wdot << " Too slow? " << endl;
        else
          cout << "  wdot is ok" << endl;
        if((udot > Tolerance) && (trimudot == true))
          cout << "  udot is too high: " << udot << " Too fast? " << endl;
        else
          cout << "  udot is ok" << endl;
        if(qdot > A_Tolerance)
          cout << "  qdot is too high: " << qdot << " Pitch Stability?  Elevator Power? " << endl;
        else
          cout << "  qdot is ok" << endl;

        trim_failed=true;

      }
    }
    k++;
  } while(((fabs(wdot) > Tolerance) || (fabs(udot) > Tolerance) || (fabs(qdot) > A_Tolerance)) && (k < Ncycles));
  total_its=k;
  if(!trim_failed)
    cout << endl << "Trim successful" << endl;

  return trim_failed;
}



