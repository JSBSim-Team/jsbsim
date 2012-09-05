/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrim.cpp
 Author:       Tony Peden
 Date started: 9/8/99

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
9/8/99   TP   Created

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

This class takes the given set of IC's and finds the angle of attack, elevator,
and throttle setting required to fly steady level. This is currently for in-air
conditions only.  It is implemented using an iterative, one-axis-at-a-time
scheme. */

//  !!!!!!! BEWARE ALL YE WHO ENTER HERE !!!!!!!

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iomanip>
#include "FGTrim.h"
#include "models/FGGroundReactions.h"
#include "models/FGInertial.h"

#if _MSC_VER
#pragma warning (disable : 4786 4788)
#endif

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGTrim.cpp,v 1.17 2012/09/05 21:49:19 bcoconni Exp $";
static const char *IdHdr = ID_TRIM;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTrim::FGTrim(FGFDMExec *FDMExec,TrimMode tt) {

  N=Nsub=0;
  max_iterations=60;
  max_sub_iterations=100;
  Tolerance=1E-3;
  A_Tolerance = Tolerance / 10;

  Debug=0;DebugLevel=0;
  fdmex=FDMExec;
  fgic=fdmex->GetIC();
  total_its=0;
  trimudot=true;
  gamma_fallback=false;
  axis_count=0;
  mode=tt;
  xlo=xhi=alo=ahi=0.0;
  targetNlf=1.0;
  debug_axis=tAll;
  SetMode(tt);
  if (debug_lvl & 2) cout << "Instantiated: FGTrim" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTrim::~FGTrim(void) {
  for(current_axis=0; current_axis<TrimAxes.size(); current_axis++) {
    delete TrimAxes[current_axis];
  }
  delete[] sub_iterations;
  delete[] successful;
  delete[] solution;
  if (debug_lvl & 2) cout << "Destroyed:    FGTrim" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::TrimStats() {
  int run_sum=0;
  cout << endl << "  Trim Statistics: " << endl;
  cout << "    Total Iterations: " << total_its << endl;
  if( total_its > 0) {
    cout << "    Sub-iterations:" << endl;
    for (current_axis=0; current_axis<TrimAxes.size(); current_axis++) {
      run_sum += TrimAxes[current_axis]->GetRunCount();
      cout << "   " << setw(5) << TrimAxes[current_axis]->GetStateName().c_str()
           << ": " << setprecision(3) << sub_iterations[current_axis]
           << " average: " << setprecision(5) << sub_iterations[current_axis]/double(total_its)
           << "  successful:  " << setprecision(3) << successful[current_axis]
           << "  stability: " << setprecision(5) << TrimAxes[current_axis]->GetAvgStability()
           << endl;
    }
    cout << "    Run Count: " << run_sum << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::Report(void) {
  cout << "  Trim Results: " << endl;
  for(current_axis=0; current_axis<TrimAxes.size(); current_axis++)
    TrimAxes[current_axis]->AxisReport();

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::ClearStates(void) {
    FGTrimAxis* ta;

    mode=tCustom;
    vector<FGTrimAxis*>::iterator iAxes;
    iAxes = TrimAxes.begin();
    while (iAxes != TrimAxes.end()) {
      ta=*iAxes;
      delete ta;
      iAxes++;
    }
    TrimAxes.clear();
    //cout << "TrimAxes.size(): " << TrimAxes.size() << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::AddState( State state, Control control ) {
  FGTrimAxis* ta;
  bool result=true;

  mode = tCustom;
  vector <FGTrimAxis*>::iterator iAxes = TrimAxes.begin();
  while (iAxes != TrimAxes.end()) {
      ta=*iAxes;
      if( ta->GetStateType() == state )
        result=false;
      iAxes++;
  }
  if(result) {
    TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,state,control));
    delete[] sub_iterations;
    delete[] successful;
    delete[] solution;
    sub_iterations=new double[TrimAxes.size()];
    successful=new double[TrimAxes.size()];
    solution=new bool[TrimAxes.size()];
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::RemoveState( State state ) {
  FGTrimAxis* ta;
  bool result=false;

  mode = tCustom;
  vector <FGTrimAxis*>::iterator iAxes = TrimAxes.begin();
  while (iAxes != TrimAxes.end()) {
      ta=*iAxes;
      if( ta->GetStateType() == state ) {
        delete ta;
        iAxes = TrimAxes.erase(iAxes);
        result=true;
        continue;
      }
      iAxes++;
  }
  if(result) {
    delete[] sub_iterations;
    delete[] successful;
    delete[] solution;
    sub_iterations=new double[TrimAxes.size()];
    successful=new double[TrimAxes.size()];
    solution=new bool[TrimAxes.size()];
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::EditState( State state, Control new_control ){
  FGTrimAxis* ta;
  bool result=false;

  mode = tCustom;
  vector <FGTrimAxis*>::iterator iAxes = TrimAxes.begin();
  while (iAxes != TrimAxes.end()) {
      ta=*iAxes;
      if( ta->GetStateType() == state ) {
        TrimAxes.insert(iAxes,1,new FGTrimAxis(fdmex,fgic,state,new_control));
        delete ta;
        TrimAxes.erase(iAxes+1);
        result=true;
        break;
      }
      iAxes++;
  }
  return result;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::DoTrim(void) {

  trim_failed=false;
  int i;

  for(i=0;i < fdmex->GetGroundReactions()->GetNumGearUnits();i++){
    fdmex->GetGroundReactions()->GetGearUnit(i)->SetReport(false);
  }

  fdmex->DisableOutput();

  fdmex->SetTrimStatus(true);
  fdmex->SuspendIntegration();

  fgic->SetPRadpsIC(0.0);
  fgic->SetQRadpsIC(0.0);
  fgic->SetRRadpsIC(0.0);

  //clear the sub iterations counts & zero out the controls
  for(current_axis=0;current_axis<TrimAxes.size();current_axis++) {
    //cout << current_axis << "  " << TrimAxes[current_axis]->GetStateName()
    //<< "  " << TrimAxes[current_axis]->GetControlName()<< endl;
    if(TrimAxes[current_axis]->GetStateType() == tQdot) {
      if(mode == tGround) {
        TrimAxes[current_axis]->initTheta();
      }
    }
    xlo=TrimAxes[current_axis]->GetControlMin();
    xhi=TrimAxes[current_axis]->GetControlMax();
    TrimAxes[current_axis]->SetControl((xlo+xhi)/2);
    TrimAxes[current_axis]->Run();
    //TrimAxes[current_axis]->AxisReport();
    sub_iterations[current_axis]=0;
    successful[current_axis]=0;
    solution[current_axis]=false;
  }


  if(mode == tPullup ) {
    cout << "Setting pitch rate and nlf... " << endl;
    setupPullup();
    cout << "pitch rate done ... " << endl;
    TrimAxes[0]->SetStateTarget(targetNlf);
    cout << "nlf done" << endl;
  } else if (mode == tTurn) {
    setupTurn();
    //TrimAxes[0]->SetStateTarget(targetNlf);
  }

  do {
    axis_count=0;
    for(current_axis=0;current_axis<TrimAxes.size();current_axis++) {
      setDebug();
      updateRates();
      Nsub=0;
      if(!solution[current_axis]) {
        if(checkLimits()) {
          solution[current_axis]=true;
          solve();
        }
      } else if(findInterval()) {
        solve();
      } else {
        solution[current_axis]=false;
      }
      sub_iterations[current_axis]+=Nsub;
    }
    for(current_axis=0;current_axis<TrimAxes.size();current_axis++) {
      //these checks need to be done after all the axes have run
      if(Debug > 0) TrimAxes[current_axis]->AxisReport();
      if(TrimAxes[current_axis]->InTolerance()) {
        axis_count++;
        successful[current_axis]++;
      }
    }


    if((axis_count == TrimAxes.size()-1) && (TrimAxes.size() > 1)) {
      //cout << TrimAxes.size()-1 << " out of " << TrimAxes.size() << "!" << endl;
      //At this point we can check the input limits of the failed axis
      //and declare the trim failed if there is no sign change. If there
      //is, keep going until success or max iteration count

      //Oh, well: two out of three ain't bad
      for(current_axis=0;current_axis<TrimAxes.size();current_axis++) {
        //these checks need to be done after all the axes have run
        if(!TrimAxes[current_axis]->InTolerance()) {
          if(!checkLimits()) {
            // special case this for now -- if other cases arise proper
            // support can be added to FGTrimAxis
            if( (gamma_fallback) &&
                (TrimAxes[current_axis]->GetStateType() == tUdot) &&
                (TrimAxes[current_axis]->GetControlType() == tThrottle)) {
              cout << "  Can't trim udot with throttle, trying flight"
              << " path angle. (" << N << ")" << endl;
              if(TrimAxes[current_axis]->GetState() > 0)
                TrimAxes[current_axis]->SetControlToMin();
              else
                TrimAxes[current_axis]->SetControlToMax();
              TrimAxes[current_axis]->Run();
              delete TrimAxes[current_axis];
              TrimAxes[current_axis]=new FGTrimAxis(fdmex,fgic,tUdot,
                                                    tGamma );
            } else {
              cout << "  Sorry, " << TrimAxes[current_axis]->GetStateName()
              << " doesn't appear to be trimmable" << endl;
              //total_its=k;
              trim_failed=true; //force the trim to fail
            } //gamma_fallback
          }
        } //solution check
      } //for loop
    } //all-but-one check
    N++;
    if(N > max_iterations)
      trim_failed=true;
  } while((axis_count < TrimAxes.size()) && (!trim_failed));
  if((!trim_failed) && (axis_count >= TrimAxes.size())) {
    total_its=N;
    if (debug_lvl > 0)
        cout << endl << "  Trim successful" << endl;
  } else {
    total_its=N;
    if (debug_lvl > 0)
        cout << endl << "  Trim failed" << endl;
  }
  for(i=0;i < fdmex->GetGroundReactions()->GetNumGearUnits();i++){
    fdmex->GetGroundReactions()->GetGearUnit(i)->SetReport(true);
  }
  fdmex->SetTrimStatus(false);
  fdmex->ResumeIntegration();
  fdmex->EnableOutput();
  return !trim_failed;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::solve(void) {

  double x1,x2,x3,f1,f2,f3,d,d0;
  const double relax =0.9;
  double eps=TrimAxes[current_axis]->GetSolverEps();

  x1=x2=x3=0;
  d=1;
  bool success=false;
  //initializations
  if( solutionDomain != 0) {
   /* if(ahi > alo) { */
      x1=xlo;f1=alo;
      x3=xhi;f3=ahi;
   /* } else {
      x1=xhi;f1=ahi;
      x3=xlo;f3=alo;
    }   */
    d0=fabs(x3-x1);
    //iterations
    //max_sub_iterations=TrimAxes[current_axis]->GetIterationLimit();
    while ( (TrimAxes[current_axis]->InTolerance() == false )
             && (fabs(d) > eps) && (Nsub < max_sub_iterations)) {
      Nsub++;
      d=(x3-x1)/d0;
      x2=x1-d*d0*f1/(f3-f1);
      TrimAxes[current_axis]->SetControl(x2);
      TrimAxes[current_axis]->Run();
      f2=TrimAxes[current_axis]->GetState();
      if(Debug > 1) {
        cout << "FGTrim::solve Nsub,x1,x2,x3: " << Nsub << ", " << x1
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


    }//end while
    if(Nsub < max_sub_iterations) success=true;
  }
  return success;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
 produces an interval (xlo..xhi) on one side or the other of the current
 control value in which a solution exists.  This domain is, hopefully,
 smaller than xmin..0 or 0..xmax and the solver will require fewer iterations
 to find the solution. This is, hopefully, more efficient than having the
 solver start from scratch every time. Maybe it isn't though...
 This tries to take advantage of the idea that the changes from iteration to
 iteration will be small after the first one or two top-level iterations.

 assumes that changing the control will a produce significant change in the
 accel i.e. checkLimits() has already been called.

 if a solution is found above the current control, the function returns true
 and xlo is set to the current control, xhi to the interval max it found, and
 solutionDomain is set to 1.
 if the solution lies below the current control, then the function returns
 true and xlo is set to the interval min it found and xmax to the current
 control. if no solution is found, then the function returns false.


 in all cases, alo=accel(xlo) and ahi=accel(xhi) after the function exits.
 no assumptions about the state of the sim after this function has run
 can be made.
*/
bool FGTrim::findInterval(void) {
  bool found=false;
  double step;
  double current_control=TrimAxes[current_axis]->GetControl();
  double current_accel=TrimAxes[current_axis]->GetState();;
  double xmin=TrimAxes[current_axis]->GetControlMin();
  double xmax=TrimAxes[current_axis]->GetControlMax();
  double lastxlo,lastxhi,lastalo,lastahi;

  step=0.025*fabs(xmax);
  xlo=xhi=current_control;
  alo=ahi=current_accel;
  lastxlo=xlo;lastxhi=xhi;
  lastalo=alo;lastahi=ahi;
  do {

    Nsub++;
    step*=2;
    xlo-=step;
    if(xlo < xmin) xlo=xmin;
    xhi+=step;
    if(xhi > xmax) xhi=xmax;
    TrimAxes[current_axis]->SetControl(xlo);
    TrimAxes[current_axis]->Run();
    alo=TrimAxes[current_axis]->GetState();
    TrimAxes[current_axis]->SetControl(xhi);
    TrimAxes[current_axis]->Run();
    ahi=TrimAxes[current_axis]->GetState();
    if(fabs(ahi-alo) <= TrimAxes[current_axis]->GetTolerance()) continue;
    if(alo*ahi <=0) {  //found interval with root
      found=true;
      if(alo*current_accel <= 0) { //narrow interval down a bit
        solutionDomain=-1;
        xhi=lastxlo;
        ahi=lastalo;
        //xhi=current_control;
        //ahi=current_accel;
      } else {
        solutionDomain=1;
        xlo=lastxhi;
        alo=lastahi;
        //xlo=current_control;
        //alo=current_accel;
      }
    }
    lastxlo=xlo;lastxhi=xhi;
    lastalo=alo;lastahi=ahi;
    if( !found && xlo==xmin && xhi==xmax ) continue;
    if(Debug > 1)
      cout << "FGTrim::findInterval: Nsub=" << Nsub << " Lo= " << xlo
                           << " Hi= " << xhi << " alo*ahi: " << alo*ahi << endl;
  } while(!found && (Nsub <= max_sub_iterations) );
  return found;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//checks to see which side of the current control value the solution is on
//and sets solutionDomain accordingly:
//  1 if solution is between the current and max
// -1 if solution is between the min and current
//  0 if there is no solution
//
//if changing the control produces no significant change in the accel then
//solutionDomain is set to zero and the function returns false
//if a solution is found, then xlo and xhi are set so that they bracket
//the solution, alo is set to accel(xlo), and ahi is set to accel(xhi)
//if there is no change or no solution then xlo=xmin, alo=accel(xmin) and
//xhi=xmax and ahi=accel(xmax)
//in all cases the sim is left such that the control=xmax and accel=ahi

bool FGTrim::checkLimits(void) {
  bool solutionExists;
  double current_control=TrimAxes[current_axis]->GetControl();
  double current_accel=TrimAxes[current_axis]->GetState();
  xlo=TrimAxes[current_axis]->GetControlMin();
  xhi=TrimAxes[current_axis]->GetControlMax();

  TrimAxes[current_axis]->SetControl(xlo);
  TrimAxes[current_axis]->Run();
  alo=TrimAxes[current_axis]->GetState();
  TrimAxes[current_axis]->SetControl(xhi);
  TrimAxes[current_axis]->Run();
  ahi=TrimAxes[current_axis]->GetState();
  if(Debug > 1)
    cout << "checkLimits() xlo,xhi,alo,ahi: " << xlo << ", " << xhi << ", "
                                              << alo << ", " << ahi << endl;
  solutionDomain=0;
  solutionExists=false;
  if(fabs(ahi-alo) > TrimAxes[current_axis]->GetTolerance()) {
    if(alo*current_accel <= 0) {
      solutionExists=true;
      solutionDomain=-1;
      xhi=current_control;
      ahi=current_accel;
    } else if(current_accel*ahi < 0){
      solutionExists=true;
      solutionDomain=1;
      xlo=current_control;
      alo=current_accel;
    }
  }
  TrimAxes[current_axis]->SetControl(current_control);
  TrimAxes[current_axis]->Run();
  return solutionExists;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::setupPullup() {
  double g,q,cgamma;
  g=fdmex->GetInertial()->gravity();
  cgamma=cos(fgic->GetFlightPathAngleRadIC());
  cout << "setPitchRateInPullup():  " << g << ", " << cgamma << ", "
       << fgic->GetVtrueFpsIC() << endl;
  q=g*(targetNlf-cgamma)/fgic->GetVtrueFpsIC();
  cout << targetNlf << ", " << q << endl;
  fgic->SetQRadpsIC(q);
  cout << "setPitchRateInPullup() complete" << endl;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::setupTurn(void){
  double g,phi;
  phi = fgic->GetPhiRadIC();
  if( fabs(phi) > 0.001 && fabs(phi) < 1.56 ) {
    targetNlf = 1 / cos(phi);
    g = fdmex->GetInertial()->gravity();
    psidot = g*tan(phi) / fgic->GetUBodyFpsIC();
    cout << targetNlf << ", " << psidot << endl;
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::updateRates(void){
  if( mode == tTurn ) {
    double phi = fgic->GetPhiRadIC();
    double g = fdmex->GetInertial()->gravity();
    double p,q,r,theta;
    if(fabs(phi) > 0.001 && fabs(phi) < 1.56 ) {
      theta=fgic->GetThetaRadIC();
      phi=fgic->GetPhiRadIC();
      psidot = g*tan(phi) / fgic->GetUBodyFpsIC();
      p=-psidot*sin(theta);
      q=psidot*cos(theta)*sin(phi);
      r=psidot*cos(theta)*cos(phi);
    } else {
      p=q=r=0;
    }
    fgic->SetPRadpsIC(p);
    fgic->SetQRadpsIC(q);
    fgic->SetRRadpsIC(r);
  } else if( mode == tPullup && fabs(targetNlf-1) > 0.01) {
      double g,q,cgamma;
      g=fdmex->GetInertial()->gravity();
      cgamma=cos(fgic->GetFlightPathAngleRadIC());
      q=g*(targetNlf-cgamma)/fgic->GetVtrueFpsIC();
      fgic->SetQRadpsIC(q);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::setDebug(void) {
  if(debug_axis == tAll ||
      TrimAxes[current_axis]->GetStateType() == debug_axis ) {
    Debug=DebugLevel;
    return;
  } else {
    Debug=0;
    return;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::SetMode(TrimMode tt) {
    ClearStates();
    mode=tt;
    switch(tt) {
      case tFull:
        if (debug_lvl > 0)
          cout << "  Full Trim" << endl;
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tWdot,tAlpha ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tUdot,tThrottle ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tPitchTrim ));
        //TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tHmgt,tBeta ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tVdot,tPhi ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tPdot,tAileron ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tRdot,tRudder ));
        break;
      case tLongitudinal:
        if (debug_lvl > 0)
          cout << "  Longitudinal Trim" << endl;
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tWdot,tAlpha ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tUdot,tThrottle ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tPitchTrim ));
        break;
      case tGround:
        if (debug_lvl > 0)
          cout << "  Ground Trim" << endl;
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tWdot,tAltAGL ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tTheta ));
        //TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tPdot,tPhi ));
        break;
      case tPullup:
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tNlf,tAlpha ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tUdot,tThrottle ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tPitchTrim ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tHmgt,tBeta ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tVdot,tPhi ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tPdot,tAileron ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tRdot,tRudder ));
        break;
      case tTurn:
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tWdot,tAlpha ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tUdot,tThrottle ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tQdot,tPitchTrim ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tVdot,tBeta ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tPdot,tAileron ));
        TrimAxes.push_back(new FGTrimAxis(fdmex,fgic,tRdot,tRudder ));
        break;
      case tCustom:
      case tNone:
        break;
    }
    //cout << "TrimAxes.size(): " << TrimAxes.size() << endl;
    sub_iterations=new double[TrimAxes.size()];
    successful=new double[TrimAxes.size()];
    solution=new bool[TrimAxes.size()];
    current_axis=0;
}
//YOU WERE WARNED, BUT YOU DID IT ANYWAY.
}
