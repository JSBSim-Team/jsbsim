/*******************************************************************************

 Header:       FGTrimLong.cpp
 Author:       Tony Peden
 Date started: 9/8/99

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
9/8/99   TP   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

This class takes the given set of IC's and finds the angle of attack, elevator,
and throttle setting required to fly steady level. This is currently for in-air
conditions only.  It is implemented using an iterative, one-axis-at-a-time
scheme. */

//  !!!!!!! BEWARE ALL YE WHO ENTER HERE !!!!!!!


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

  Ncycles=40;
  Naxis=10;
  Tolerance=1E-3;
  A_Tolerance = Tolerance / 10;

  Debug=0;
  fdmex=FDMExec;
  fgic=FGIC;
  alphaMin=fdmex->GetAircraft()->GetAlphaCLMin()*RADTODEG;
  alphaMax=fdmex->GetAircraft()->GetAlphaCLMax()*RADTODEG;
  if(alphaMax <= alphaMin) {
    alphaMax=20;
    alphaMin=-5;
  }
  udotf=&FGTrimLong::udot_func;
  wdotf=&FGTrimLong::wdot_func;
  qdotf=&FGTrimLong::qdot_func;
  total_its=0;
  udot_subits=wdot_subits=qdot_subits=0;
  trimudot=true;
  axis_count=0;

}

/******************************************************************************/

FGTrimLong::~FGTrimLong(void) {}

/******************************************************************************/

void FGTrimLong::TrimStats() {
  cout << endl << "  Trim Statistics: " << endl;
  cout << "    Total Iterations: " << total_its << endl;
  if(total_its > 0) {
    cout << "    Sub-iterations:" << endl;
    cout << "      wdot: " << wdot_subits << " average: " << wdot_subits/total_its << endl;
    cout << "      udot: " << udot_subits << " average: " << udot_subits/total_its << endl;
    cout << "      qdot: " << qdot_subits << " average: " << qdot_subits/total_its << endl;
  }
}

/******************************************************************************/

void FGTrimLong::Report(void) {
  cout << endl << "  Trim Results" << endl;
  cout << "  Alpha: " << fdmex->GetTranslation()->Getalpha()*RADTODEG
  << " wdot: " << fdmex->GetTranslation()->GetUVWdot()(3)
  << " Tolerance " << Tolerance << endl;

  cout << "  Throttle: " << fdmex->GetFCS()->GetThrottlePos(0)
  << " udot: " << fdmex->GetTranslation()->GetUVWdot()(1)
  << " Tolerance " << Tolerance << endl;

  cout << "  Elevator: " << fdmex->GetFCS()->GetDePos()*RADTODEG
  << " qdot: " << fdmex->GetRotation()->GetPQRdot()(2)
  << " Tolerance " << A_Tolerance << endl;
}

/******************************************************************************/

void FGTrimLong::ReportState(void) {
  cout << endl << "  JSBSim Trim Report" << endl;
  cout << "    Weight: " << fdmex->GetAircraft()->GetWeight()
  << " lbs.  CG x,y,z: " << fdmex->GetAircraft()->GetXYZcg()
  << " inches " << endl;

  cout << "    Flaps: ";
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

  cout << "    Speed: " << fdmex->GetAuxiliary()->GetVcalibratedKTS()
  << " KCAS  Mach: " << fdmex->GetState()->GetParameter(FG_MACH)
  << endl;

  cout << "    Altitude: " << fdmex->GetPosition()->Geth() << " ft" << endl;


  cout << "    Pitch Angle: " << fdmex->GetRotation()->Gettht()*RADTODEG
  << " deg  Angle of Attack: " << fdmex->GetState()->GetParameter(FG_ALPHA)*RADTODEG
  << " deg" << endl;


  cout << "    Flight Path Angle: "
  << fdmex->GetPosition()->GetGamma()*RADTODEG
  << " deg" << endl;


  cout << "    Normal Load Factor: " << fdmex->GetAircraft()->GetNlf() << endl;

  cout << "    Pitch Rate: " << fdmex->GetState()->GetParameter(FG_PITCHRATE)*RADTODEG
  << " deg/s" << endl;

  cout << "    Roll Angle: " << fdmex->GetRotation()->Getphi()*RADTODEG
  << " deg  Roll Rate: " << fdmex->GetState()->GetParameter(FG_ROLLRATE)
  << " deg/s"
  << endl ;

  cout << "    Sideslip: " << fdmex->GetState()->GetParameter(FG_BETA) *RADTODEG
  << " deg  Yaw Rate: " << fdmex->GetState()->GetParameter(FG_YAWRATE)*RADTODEG
  << " deg/s " << endl;

  cout << "    Elevator: " << fdmex->GetState()->GetParameter(FG_ELEVATOR_POS)*RADTODEG
  << " deg  Left Aileron: " << fdmex->GetState()->GetParameter(FG_AILERON_POS)*RADTODEG
  << " deg  Rudder: " << fdmex->GetState()->GetParameter(FG_RUDDER_POS)*RADTODEG
  << " deg" << endl;

  cout << "    Throttle: " << fdmex->GetFCS()->GetThrottlePos(0)/100 << endl;
}

/******************************************************************************/

void FGTrimLong::setThrottlesPct(float tt) {

  float tMin,tMax;
  for(int i=0;i<fdmex->GetAircraft()->GetNumEngines();i++) {
    tMin=fdmex->GetAircraft()->GetEngine(i)->GetThrottleMin();
    tMax=fdmex->GetAircraft()->GetEngine(i)->GetThrottleMax();
    dth=tt;
    //cout << "setThrottlespct: " << i << ", " << tMin << ", " << tMax << ", " << dth << endl;
    fdmex -> GetFCS() -> SetThrottleCmd(i,tMin+dth*(tMax-tMin));
  }
}

/******************************************************************************/

int FGTrimLong::checkLimits(trimfp fp, float current, float min, float max) {
  float lo,hi;
  int result=0;
  //cout << "Min: " << min << " Max: " << max << endl;
  lo=(this->*fp)(min);
  hi=(this->*fp)(max);

  if(lo*hi >= 0) {
    //cout << "Lo: " << lo << " Hi: " << hi << endl;
    result=0;
  } else {
    lo=(this->*fp)(0);
    if(lo*hi >= 0)
      result=-1;
    else
      result=1;
  }

  return result;
}

/******************************************************************************/

bool FGTrimLong::solve(trimfp fp,float guess,float desired, float *result,
         float eps, float min, float max, int max_iterations, int *actual_its) {

  float x1,x2,x3,f1,f2,f3,d,d0;
  float const relax =0.9;
  int i;
  x1 = x3 = x2 = 0;
  d=1;
  bool success=false;
  //initializations
  int side=checkLimits(fp,guess,min,max);
  if(side != 0) {
    if (side < 0)
      x3=min;
    else
      x1=max;

    f1=(this->*fp)(x1)-desired;
    f3=(this->*fp)(x3)-desired;
    d0=fabs(x3-x1);

    //iterations
    i=0;
    while ((fabs(d) > eps) && (i < max_iterations)) {
      if(Debug > 1)
        cout << "FGTrimLong::solve i,x1,x2,x3: " << i << ", " << x1
                                            << ", " << x2 << ", " << x3 << endl;
      d=(x3-x1)/d0;
      x2=x1-d*d0*f1/(f3-f1);
      // if(x2 < min)
      //         x2=min;
      //       else if(x2 > max)
      //         x2=max;
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

/******************************************************************************/

bool FGTrimLong::findInterval(trimfp fp, float *lo, float *hi,float guess,
                                             float desired,int max_iterations) {
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
      cout << "FGTrimLong::findInterval: i=" << i << " Lo= " << xlo
                           << " Hi= " << xhi << " flo*fhi: " << flo*fhi << endl;
  } while((found == 0) && (i <= max_iterations));
  *lo=xlo;
  *hi=xhi;
  return found;
}

/******************************************************************************/

float FGTrimLong::udot_func(float x) {
  setThrottlesPct(x);
  fdmex->RunIC(fgic);
  return fdmex->GetTranslation()->GetUVWdot()(1);
}

/******************************************************************************/

float FGTrimLong::wdot_func(float x) {
  fgic->SetAlphaDegIC(x);
  fdmex->RunIC(fgic);
  return fdmex->GetTranslation()->GetUVWdot()(3);
}

/******************************************************************************/

float FGTrimLong::qdot_func(float x) {
  fdmex->GetFCS()->SetPitchTrimCmd(x);
  fdmex->RunIC(fgic);
  return fdmex->GetRotation()->GetPQRdot()(2);
}

/******************************************************************************/

bool FGTrimLong::DoTrim(void) {
  int k=0,j=0,sum=0,trim_failed=0,jmax=Naxis;
  int its;
  float step,temp,min,max;

  if(fgic->GetVtrueKtsIC() < 1) {
    cout << "Trim failed, on-ground trimming not yet implemented." << endl;
    cout << "Or did you *really* mean to start in-air"
         << " with less than 1 knot airspeed?" << endl;
    return false;
  }

  trimfp fp;

  fgic -> SetAlphaDegIC((alphaMin+alphaMax)/2);
  fdmex -> GetFCS() -> SetDeCmd(0);
  fdmex -> GetFCS() -> SetPitchTrimCmd(0);
  setThrottlesPct(0.5);
  fdmex -> RunIC(fgic);

  if(trimudot == false)
    udot=0;
  do {
    axis_count=0;
    solve(wdotf,fgic->GetAlphaDegIC(),0,&wdot,Tolerance,alphaMin, alphaMax,Naxis,&its);
    wdot_subits+=its;
    if(Debug > 0) {
      cout << "Alpha: " << fdmex->GetTranslation()->Getalpha()*RADTODEG
      << " wdot: " << fdmex->GetTranslation()->GetUVWdot()(3)
      << endl;
    }
    solve(udotf,dth,0,&udot,Tolerance,0,1,Naxis,&its);
    udot_subits+=its;
    if(Debug > 0) {
      cout << "Throttle: " << fdmex->GetFCS()->GetThrottlePos(0)
      << " udot: " << fdmex->GetTranslation()->GetUVWdot()(1)
      << endl;
    }
    solve(qdotf,fdmex->GetFCS()->GetPitchTrimCmd(),0,&qdot,A_Tolerance,-1,1,Naxis,&its);
    qdot_subits+=its;
    if(Debug > 0) {
      cout << "Elevator: " << fdmex->GetFCS()->GetDePos()*RADTODEG
      << " qdot: " << fdmex->GetRotation()->GetPQRdot()(2)
      << endl;
    }
    wdot=fabs(fdmex->GetTranslation()->GetUVWdot()(3));
    qdot=fabs(fdmex->GetRotation()->GetPQRdot()(2));
    udot=fabs(fdmex->GetTranslation()->GetUVWdot()(1));

    //these checks need to be done after all the axes have run
    if(udot < Tolerance)
      axis_count++;
    if(wdot < Tolerance)
      axis_count++;
    if(qdot < A_Tolerance)
      axis_count++;
    if(axis_count == 2) {

      //At this point we can check the input limits of the failed axis
      //and declare the trim failed if there is no sign change. If there
      //is, keep going until success or max iteration count

      //Oh, well: two out of three ain't bad
      if(wdot > Tolerance) {
        if(checkLimits(wdotf,fgic->GetAlphaDegIC(),alphaMin,alphaMax) == false) {
          cout << "    Sorry, wdot doesn't appear to be trimmable" << endl;
          total_its=k;
          k=Ncycles; //force the trim to fail
        }



      }
      if( udot > Tolerance ) {
        if(checkLimits(udotf,dth,0,1) == false) {
          cout << "    Sorry, udot doesn't appear to be trimmable" << endl;
          cout << "    Resetting throttles to zero" << endl;
          fdmex->GetFCS()->SetThrottleCmd(-1,0);
          total_its=k;
          k=Ncycles; //force the trim to fail
        }



      }
      if(qdot > A_Tolerance) {

        if(checkLimits(qdotf,fdmex->GetFCS()->GetPitchTrimCmd(),-1,1) == false) {
          cout << "    Sorry, qdot doesn't appear to be trimmable" << endl;
          total_its=k;
          k=Ncycles; //force the trim to fail
        }



      }
    }
    k++;
  } while((axis_count < 3) && (k < Ncycles));
  if(axis_count >= 3) {
    total_its=k;
    cout << endl << "  Trim successful" << endl;
    return true;
  } else {
    total_its=k;
    cout << endl << "  Trim failed" << endl;
    return false;
  }

}

//YOU WERE WARNED, BUT YOU DID IT ANYWAY.

