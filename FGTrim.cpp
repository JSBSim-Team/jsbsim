/*******************************************************************************
 
 Header:       FGTrim.cpp
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

#include <stdlib.h>

#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGInitialCondition.h"
#include "FGTrim.h"
#include "FGAircraft.h"

/*******************************************************************************/

FGTrim::FGTrim(FGFDMExec *FDMExec,FGInitialCondition *FGIC, TrimMode tt ) {

  max_iterations=40;
  max_sub_iterations=10;
  Tolerance=1E-3;
  A_Tolerance = Tolerance / 10;

  Debug=0;
  fdmex=FDMExec;
  fgic=FGIC;
  total_its=0;
  trimudot=true;
  gamma_fallback=true;
  axis_count=0;
  mode=tt;
  switch(mode) {
  case tFull:
    cout << "  Full 6-DOF Trim" << endl;
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tWdot,tAlpha,Tolerance));
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tUdot,tThrottle,Tolerance));
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tElevator,A_Tolerance));
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tVdot,tPhi,Tolerance));
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tPdot,tAileron,A_Tolerance));
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tRdot,tRudder,A_Tolerance));
    break;
  case tLongitudinal:
    cout << "  Longitudinal Trim" << endl;
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tWdot,tAlpha,Tolerance));
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tUdot,tThrottle,Tolerance));
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tElevator,A_Tolerance));
    break;
  case tGround:
    cout << "  Ground Trim" << endl;
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tWdot,tAltAGL,Tolerance));
    //TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tPdot,tPhi,A_Tolerance));
    //TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tTheta,A_Tolerance));
    break;
  }
  //cout << "NumAxes: " << TrimAxes.size() << endl;
  NumAxes=TrimAxes.size();
  sub_iterations=new float[NumAxes];
  successful=new float[NumAxes];
  current_axis=0;
}

/******************************************************************************/

FGTrim::~FGTrim(void) {
  for(current_axis=0; current_axis<NumAxes; current_axis++) {
    delete TrimAxes[current_axis];
  }
  delete[] sub_iterations;
  delete[] successful;
}

/******************************************************************************/

void FGTrim::TrimStats() {
  char out[80];
  cout << endl << "  Trim Statistics: " << endl;
  cout << "    Total Iterations: " << total_its << endl;
  if(total_its > 0) {
    cout << "    Sub-iterations:" << endl;
    for(current_axis=0; current_axis<NumAxes; current_axis++) {
      
      sprintf(out,"   %5s: %3.0f average: %5.2f stability: %5.2f  successful: %3.0f\n",
                  TrimAxes[current_axis]->GetAccelName().c_str(),
                  sub_iterations[current_axis],
                  sub_iterations[current_axis]/float(total_its),
                  TrimAxes[current_axis]->GetAvgStability(),
                  successful[current_axis] );
      cout << out;
    }
  }
}

/******************************************************************************/

void FGTrim::Report(void) {
  cout << "  Trim Results: " << endl;
  for(current_axis=0; current_axis<NumAxes; current_axis++)
    TrimAxes[current_axis]->AxisReport();

}

/******************************************************************************/

void FGTrim::ReportState(void) {
  char out[80], flap[10], gear[10];
  
  cout << endl << "  JSBSim State" << endl;
  sprintf(out,"    Weight: %7.0f lbs.  CG: %5.1f, %5.1f, %5.1f inches\n",
                   fdmex->GetAircraft()->GetWeight(),
                   fdmex->GetAircraft()->GetXYZcg()(1),
                   fdmex->GetAircraft()->GetXYZcg()(2),
                   fdmex->GetAircraft()->GetXYZcg()(3) );
  cout << out;             
  if( fdmex->GetFCS()->GetDfPos() <= 0.01)
    sprintf(flap,"Up");
  else
    sprintf(flap,"%2.0f",fdmex->GetFCS()->GetDfPos());
  if(fdmex->GetAircraft()->GetGearUp() == true)
    sprintf(gear,"Up");
  else
    sprintf(gear,"Down");
  sprintf(out, "    Flaps: %3s  Gear: %4s\n",flap,gear);
  cout << out;
  sprintf(out, "    Speed: %4.0f KCAS  Mach: %5.2f  Altitude: %7.0f ft.\n",
                    fdmex->GetAuxiliary()->GetVcalibratedKTS(),
                    fdmex->GetState()->GetParameter(FG_MACH),
                    fdmex->GetPosition()->Geth() );
  cout << out;
  sprintf(out, "    Angle of Attack: %6.2f deg  Pitch Angle: %6.2f deg\n",
                    fdmex->GetState()->GetParameter(FG_ALPHA)*RADTODEG,
                    fdmex->GetRotation()->Gettht()*RADTODEG );
  cout << out;
  sprintf(out, "    Flight Path Angle: %6.2f deg  Climb Rate: %5.0f ft/min\n",
                    fdmex->GetPosition()->GetGamma()*RADTODEG,
                    fdmex->GetPosition()->Gethdot()*60 );
  cout << out;                  
  sprintf(out, "    Normal Load Factor: %4.2f g's  Pitch Rate: %5.2f deg/s\n",
                    fdmex->GetAircraft()->GetNlf(),
                    fdmex->GetState()->GetParameter(FG_PITCHRATE)*RADTODEG );
  cout << out;
  sprintf(out, "    True Heading: %3.0f deg  Sideslip: %5.2f deg\n",
                    fdmex->GetRotation()->Getpsi()*RADTODEG,
                    fdmex->GetState()->GetParameter(FG_BETA)*RADTODEG );                  
  cout << out;
  sprintf(out, "    Bank Angle: %3.0f deg\n",
                    fdmex->GetRotation()->Getphi()*RADTODEG );
  cout << out;
  sprintf(out, "    Elevator: %5.2f deg  Left Aileron: %5.2f deg  Rudder: %5.2f deg\n",
                    fdmex->GetState()->GetParameter(FG_ELEVATOR_POS)*RADTODEG,
                    fdmex->GetState()->GetParameter(FG_AILERON_POS)*RADTODEG,
                    fdmex->GetState()->GetParameter(FG_RUDDER_POS)*RADTODEG );
  cout << out;                  
  sprintf(out, "    Throttle: %5.2f\%\n",
                    fdmex->GetFCS()->GetThrottlePos(0) );
  cout << out;                                  
  
  
  /* cout << "    Weight: " << fdmex->GetAircraft()->GetWeight()
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

  cout << "    Throttle: " << fdmex->GetFCS()->GetThrottlePos(0)/100 << endl; */

}

/******************************************************************************/

bool FGTrim::solve(int *actual_its) {

  float x1,x2,x3,f1,f2,f3,d,d0;
  float const relax =0.9;
  int i,solutionDomain=0;
  float eps=TrimAxes[current_axis]->GetSolverEps();

  x1=x2=x3=0;
  d=1;
  bool success=false;
  solutionDomain=TrimAxes[current_axis]->GetSolutionDomain();
  //initializations
  if( solutionDomain != 0) {
    if(solutionDomain > 0)
      x1=TrimAxes[current_axis]->GetControlMax();
    else
      x3=TrimAxes[current_axis]->GetControlMin();
    TrimAxes[current_axis]->SetControl(x1);
    f1=TrimAxes[current_axis]->Run();
    //cout << "FGRotation::theta: " << fdmex->GetRotation()->Gettht()*RADTODEG << endl;
    TrimAxes[current_axis]->SetControl(x3);
    f3=TrimAxes[current_axis]->Run();
    //cout << "FGRotation::theta: " << fdmex->GetRotation()->Gettht()*RADTODEG << endl;
    d0=fabs(x3-x1);

    //iterations
    max_sub_iterations=TrimAxes[current_axis]->GetIterationLimit();
    i=0;
    while ((fabs(d) > eps) && (i < max_sub_iterations)) {

      d=(x3-x1)/d0;
      x2=x1-d*d0*f1/(f3-f1);
      // if(x2 < min)
      //         x2=min;
      //       else if(x2 > max)
      //         x2=max;
      //cout << "x2: " << x2 << endl;
      TrimAxes[current_axis]->SetControl(x2);
      f2=TrimAxes[current_axis]->Run();
      //cout << "FGRotation::theta: " << fdmex->GetRotation()->Gettht()*RADTODEG << endl;
      if(Debug > 1) {
        cout << "FGTrim::solve i,x1,x2,x3: " << i << ", " << x1
        << ", " << x2 << ", " << x3 << endl;
        cout << "                             " << f1 << ", " << f2 << ", " << f3 << endl;
      }
      if(f1*f2 <= 0.0) {
        x3=x2;
        f3=f2;
        f1=relax*f1;
        //cout << "Solution is between x1 and x2" << endl;
      }
      else if(f2*f3 <= 0.0) {
        x1=x2;
        f1=f2;
        f3=relax*f3;
        //cout << "Solution is between x2 and x3" << endl;

      }
      //cout << i << endl;

      i++;
    }//end while
    if(i < max_iterations) {
      success=true;

    }
    *actual_its=i;
  } else {
    *actual_its=0;
  }
  return success;
}


/******************************************************************************/

bool FGTrim::DoTrim(void) {
  int k=0;
  int its;

  trim_failed=false;


  float qdot;
  //clear the sub iterations counts & zero out the controls
  for(current_axis=0;current_axis<NumAxes;current_axis++) {
    //cout << current_axis << "  " << TrimAxes[current_axis]->GetAccelName()
    //<< "  " << TrimAxes[current_axis]->GetControlName()<< endl;
    TrimAxes[current_axis]->SetControl(0);
    TrimAxes[current_axis]->Run();
    sub_iterations[current_axis]=0;
    successful[current_axis]=0;
  }
  do {
    axis_count=0;
    for(current_axis=0;current_axis<NumAxes;current_axis++) {
      //cout << TrimAxes[current_axis]->GetControlName() << "->" << TrimAxes[current_axis]->GetAccelName() << ": ";
      if(TrimAxes[current_axis]->checkLimits() ) {
        solve(&its);
      }
      sub_iterations[current_axis]+=its;
      /* if(Debug > 0) {
        TrimAxes[current_axis]->AxisReport(); 
    } */

    }
    for(current_axis=0;current_axis<NumAxes;current_axis++) {
      //these checks need to be done after all the axes have run
      if(Debug > 0) {
        TrimAxes[current_axis]->AxisReport();
      }
      if(fabs(TrimAxes[current_axis]->GetAccel()) < 
               TrimAxes[current_axis]->GetTolerance()) {
        axis_count++;
        successful[current_axis]++;
      }  
      //else
      //  cout << TrimAxes[current_axis]->GetAccelName() << " failed" << endl;  
    }
    if((axis_count == NumAxes-1) && (NumAxes > 1)) {
      //cout << NumAxes-1 << " out of " << NumAxes << "!" << endl;
      //At this point we can check the input limits of the failed axis
      //and declare the trim failed if there is no sign change. If there
      //is, keep going until success or max iteration count

      //Oh, well: two out of three ain't bad
      for(current_axis=0;current_axis<NumAxes;current_axis++) {
        //these checks need to be done after all the axes have run
        if(fabs(TrimAxes[current_axis]->GetAccel())
            > TrimAxes[current_axis]->GetTolerance()) {

          TrimAxes[current_axis]->checkLimits();
          if(TrimAxes[current_axis]->GetSolutionDomain() == 0) {
            // special case this for now -- if other cases arise proper
            // support can be added to FGTrimAxis
            if( (gamma_fallback) &&
                (TrimAxes[current_axis]->GetAccelType() == tUdot) &&
                (TrimAxes[current_axis]->GetControlType() == tThrottle)) {
              cout << "  Can't trim udot with throttle, trying flight"
              << " path angle." << endl;
              if(TrimAxes[current_axis]->GetAccel() > 0)
                TrimAxes[current_axis]->SetControlToMin();
              else
                TrimAxes[current_axis]->SetControlToMax();
              TrimAxes[current_axis]->Run();
              delete TrimAxes[current_axis];
              TrimAxes[current_axis]=new FGTrimAxis(fdmex,fgic,tUdot,
                                                    tGamma,Tolerance);
            } else {
              cout << "  Sorry, " << TrimAxes[current_axis]->GetAccelName()
              << " doesn't appear to be trimmable" << endl;
              //total_its=k;
              trim_failed=true; //force the trim to fail
            }

          }
        }
      }
    }
    k++;
    if(k > max_iterations)
      trim_failed=true;
  } while((axis_count < NumAxes) && (k <= max_iterations) && (!trim_failed));
  if((!trim_failed) && (axis_count >= NumAxes)) {
    total_its=k;
    cout << endl << "  Trim successful" << endl;
  } else {
    total_its=k;
    cout << endl << "  Trim failed" << endl;
  }
  return !trim_failed;
}

//YOU WERE WARNED, BUT YOU DID IT ANYWAY.

