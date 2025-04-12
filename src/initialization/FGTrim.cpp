/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTrim.cpp
 Author:       Tony Peden
 Date started: 9/8/99

 --------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) ---------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

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
#include "models/FGInertial.h"
#include "models/FGAccelerations.h"
#include "models/FGMassBalance.h"
#include "models/FGFCS.h"
#include "input_output/FGLog.h"

#if _MSC_VER
#pragma warning (disable : 4786 4788)
#endif

using namespace std;

namespace JSBSim {

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTrim::FGTrim(FGFDMExec *FDMExec,TrimMode tt)
  : fgic(FDMExec)
{

  Nsub=0;
  max_iterations=60;
  max_sub_iterations=100;
  Tolerance=1E-3;
  A_Tolerance = Tolerance / 10;

  Debug=0;DebugLevel=0;
  fdmex=FDMExec;
  fgic = *fdmex->GetIC();
  total_its=0;
  gamma_fallback=false;
  mode=tt;
  xlo=xhi=alo=ahi=0.0;
  targetNlf=fgic.GetTargetNlfIC();
  debug_axis=tAll;
  SetMode(tt);
  if (debug_lvl & 2) {
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    log << "Instantiated: FGTrim\n";
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTrim::~FGTrim(void) {
  if (debug_lvl & 2) {
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    log << "Destroyed:    FGTrim\n";
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::TrimStats() {
  int run_sum=0;
  FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
  log << "\n  Trim Statistics:\n";
  log << "    Total Iterations: " << total_its << "\n";
  if( total_its > 0) {
    log << "    Sub-iterations:\n";
    for (unsigned int current_axis=0; current_axis<TrimAxes.size(); current_axis++) {
      run_sum += TrimAxes[current_axis].GetRunCount();
      log << "   " << setw(5) << TrimAxes[current_axis].GetStateName().c_str()
           << ": " << setprecision(3) << sub_iterations[current_axis]
           << " average: " << setprecision(5) << sub_iterations[current_axis]/double(total_its)
           << "  successful:  " << setprecision(3) << successful[current_axis]
           << "  stability: " << setprecision(5) << TrimAxes[current_axis].GetAvgStability()
           << "\n";
    }
    log << "    Run Count: " << run_sum << "\n";
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::Report(void) {
  FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
  log << "  Trim Results:\n";
  for(unsigned int current_axis=0; current_axis<TrimAxes.size(); current_axis++)
    TrimAxes[current_axis].AxisReport();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::ClearStates(void) {
    mode=tCustom;
    TrimAxes.clear();
    //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
    //log << "TrimAxes.size(): " << TrimAxes.size() << "\n";
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::AddState( State state, Control control ) {
  mode = tCustom;
  vector <FGTrimAxis>::iterator iAxes = TrimAxes.begin();
  for (; iAxes != TrimAxes.end(); ++iAxes) {
    if (iAxes->GetStateType() == state)
      return false;
  }

  TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,state,control));
  sub_iterations.resize(TrimAxes.size());
  successful.resize(TrimAxes.size());
  solution.resize(TrimAxes.size());

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::RemoveState( State state ) {
  bool result=false;

  mode = tCustom;
  vector <FGTrimAxis>::iterator iAxes = TrimAxes.begin();
  while (iAxes != TrimAxes.end()) {
      if( iAxes->GetStateType() == state ) {
        iAxes = TrimAxes.erase(iAxes);
        result=true;
        continue;
      }
      ++iAxes;
  }
  if(result) {
    sub_iterations.resize(TrimAxes.size());
    successful.resize(TrimAxes.size());
    solution.resize(TrimAxes.size());
  }
  return result;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::EditState( State state, Control new_control ){
  mode = tCustom;
  vector <FGTrimAxis>::iterator iAxes = TrimAxes.begin();
  while (iAxes != TrimAxes.end()) {
      if( iAxes->GetStateType() == state ) {
        *iAxes = FGTrimAxis(fdmex,&fgic,state,new_control);
        return true;
      }
      ++iAxes;
  }
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::DoTrim(void) {
  bool trim_failed=false;
  unsigned int N = 0;
  unsigned int axis_count = 0;
  auto FCS = fdmex->GetFCS();
  auto GroundReactions = fdmex->GetGroundReactions();
  vector<double> throttle0 = FCS->GetThrottleCmd();
  double elevator0 = FCS->GetDeCmd();
  double aileron0 = FCS->GetDaCmd();
  double rudder0 = FCS->GetDrCmd();
  double PitchTrim0 = FCS->GetPitchTrimCmd();

  for(int i=0;i < GroundReactions->GetNumGearUnits();i++)
    GroundReactions->GetGearUnit(i)->SetReport(false);

  fdmex->SetTrimStatus(true);
  fdmex->SuspendIntegration();

  fgic.SetPRadpsIC(0.0);
  fgic.SetQRadpsIC(0.0);
  fgic.SetRRadpsIC(0.0);

  if (mode == tGround) {
    fdmex->Initialize(&fgic);
    fdmex->Run();
    trimOnGround();
    double theta = fgic.GetThetaRadIC();
    double phi = fgic.GetPhiRadIC();
    // Take opportunity of the first approx. found by trimOnGround() to
    // refine the control limits.
    TrimAxes[0].SetControlLimits(0., fgic.GetAltitudeAGLFtIC());
    TrimAxes[1].SetControlLimits(theta - 5.0 * degtorad, theta + 5.0 * degtorad);
    TrimAxes[2].SetControlLimits(phi - 30.0 * degtorad, phi + 30.0 * degtorad);
  }

  //clear the sub iterations counts & zero out the controls
  for(unsigned int current_axis=0;current_axis<TrimAxes.size();current_axis++) {
    //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
    //log << current_axis << "  " << TrimAxes[current_axis]->GetStateName()
    //<< "  " << TrimAxes[current_axis]->GetControlName()<< "\n";
    xlo=TrimAxes[current_axis].GetControlMin();
    xhi=TrimAxes[current_axis].GetControlMax();
    TrimAxes[current_axis].SetControl((xlo+xhi)/2);
    TrimAxes[current_axis].Run();
    //TrimAxes[current_axis].AxisReport();
    sub_iterations[current_axis]=0;
    successful[current_axis]=0;
    solution[current_axis]=false;
  }

  if(mode == tPullup ) {
    FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
    log << "Setting pitch rate and nlf...\n";
    setupPullup();
    log << "pitch rate done ...\n";
    TrimAxes[0].SetStateTarget(targetNlf);
    log << "nlf done\n";
  } else if (mode == tTurn) {
    setupTurn();
    //TrimAxes[0].SetStateTarget(targetNlf);
  }

  do {
    axis_count=0;
    for(unsigned int current_axis=0;current_axis<TrimAxes.size();current_axis++) {
      setDebug(TrimAxes[current_axis]);
      updateRates();
      Nsub=0;
      if(!solution[current_axis]) {
        if(checkLimits(TrimAxes[current_axis])) {
          solution[current_axis]=true;
          solve(TrimAxes[current_axis]);
        }
      } else if(findInterval(TrimAxes[current_axis])) {
        solve(TrimAxes[current_axis]);
      } else {
        solution[current_axis]=false;
      }
      sub_iterations[current_axis]+=Nsub;
    }
    for(unsigned int current_axis=0;current_axis<TrimAxes.size();current_axis++) {
      //these checks need to be done after all the axes have run
      if(Debug > 0) TrimAxes[current_axis].AxisReport();
      if(TrimAxes[current_axis].InTolerance()) {
        axis_count++;
        successful[current_axis]++;
      }
    }

    if((axis_count == TrimAxes.size()-1) && (TrimAxes.size() > 1)) {
      //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
      //log << TrimAxes.size()-1 << " out of " << TrimAxes.size() << "!" << "\n";
      //At this point we can check the input limits of the failed axis
      //and declare the trim failed if there is no sign change. If there
      //is, keep going until success or max iteration count

      //Oh, well: two out of three ain't bad
      for(unsigned int current_axis=0;current_axis<TrimAxes.size();current_axis++) {
        //these checks need to be done after all the axes have run
        if(!TrimAxes[current_axis].InTolerance()) {
          if(!checkLimits(TrimAxes[current_axis])) {
            // special case this for now -- if other cases arise proper
            // support can be added to FGTrimAxis
            if( (gamma_fallback) &&
                (TrimAxes[current_axis].GetStateType() == tUdot) &&
                (TrimAxes[current_axis].GetControlType() == tThrottle)) {
              FGLogging log(fdmex->GetLogger(), LogLevel::WARN);
              log << "  Can't trim udot with throttle, trying flight"
              << " path angle. (" << N << ")\n";
              if(TrimAxes[current_axis].GetState() > 0)
                TrimAxes[current_axis].SetControlToMin();
              else
                TrimAxes[current_axis].SetControlToMax();
              TrimAxes[current_axis].Run();
              TrimAxes[current_axis]=FGTrimAxis(fdmex,&fgic,tUdot,tGamma);
            } else {
              FGLogging log(fdmex->GetLogger(), LogLevel::ERROR);
              log << "  Sorry, " << TrimAxes[current_axis].GetStateName()
              << " doesn't appear to be trimmable\n";
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
    if (debug_lvl > 0) {
      FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
      log << "\n  Trim successful\n";
    }
  } else { // The trim has failed
    total_its=N;

    // Restore the aircraft parameters to their initial values
    fgic = *fdmex->GetIC();
    FCS->SetDeCmd(elevator0);
    FCS->SetDaCmd(aileron0);
    FCS->SetDrCmd(rudder0);
    FCS->SetPitchTrimCmd(PitchTrim0);
    for (unsigned int i=0; i < throttle0.size(); i++)
      FCS->SetThrottleCmd(i, throttle0[i]);

    fdmex->Initialize(&fgic);
    fdmex->Run();

    // If WOW is true we must make sure there are no gears into the ground.
    if (GroundReactions->GetWOW())
      trimOnGround();

    if (debug_lvl > 0) {
      FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
      log << "\n  Trim failed\n";
    }
  }

  fdmex->GetPropagate()->InitializeDerivatives();
  fdmex->ResumeIntegration();
  fdmex->SetTrimStatus(false);

  for(int i=0;i < GroundReactions->GetNumGearUnits();i++)
    GroundReactions->GetGearUnit(i)->SetReport(true);

  return !trim_failed;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Trim the aircraft on the ground. The algorithm is looking for a stable
// position of the aicraft. Assuming the aircaft is a rigid body and the ground
// a plane: we need to find the translations and rotations of the aircraft that
// will move 3 non-colinear points in contact with the ground.
// The algorithm proceeds in three stages (one for each point):
// 1. Look for the contact point closer to or deeper into the ground. Move the
//    aircraft along the vertical direction so that only this contact point
//    remains in contact with the ground.
// 2. The forces applied on the aircraft (most likely the gravity) will generate
//    a moment on the aircraft around the point in contact. The rotation axis is
//    therefore the moment axis. The 2nd stage thus consists in determining the
//    minimum rotation angle around the first point in contact that will place a
//    second contact point on the ground.
// 3. At this stage, 2 points are in contact with the ground: the rotation axis
//    is therefore the vector generated by the 2 points. Like stage #2, the
//    rotation direction will be driven by the moment around the axis formed by
//    the 2 points in contact. The rotation angle is obtained similarly to stage
//    #2: it is the minimum angle that will place a third contact point on the
//    ground.
// The calculations below do not account for the compression of the landing
// gears meaning that the position found is close to the real position but not
// strictly equal to it.

void FGTrim::trimOnGround(void)
{
  auto GroundReactions = fdmex->GetGroundReactions();
  auto Propagate = fdmex->GetPropagate();
  auto MassBalance = fdmex->GetMassBalance();
  auto Accelerations = fdmex->GetAccelerations();
  vector<ContactPoints> contacts;
  FGLocation CGLocation = Propagate->GetLocation();
  FGMatrix33 Tec2b = Propagate->GetTec2b();
  FGMatrix33 Tb2l = Propagate->GetTb2l();
  double hmin = 1E+10;
  int contactRef = -1;

  // Build the list of the aircraft contact points and take opportunity of the
  // loop to find which one is closer to (or deeper into) the ground.
  for (int i = 0; i < GroundReactions->GetNumGearUnits(); ++i) {
    ContactPoints c;
    auto gear = GroundReactions->GetGearUnit(i);

    // Skip the retracted landing gears
    if (!gear->GetGearUnitDown())
      continue;

    c.location = gear->GetBodyLocation();
    FGLocation gearLoc = CGLocation.LocalToLocation(Tb2l * c.location);

    FGColumnVector3 normal, vDummy;
    FGLocation lDummy;
    double height = fdmex->GetInertial()->GetContactPoint(gearLoc, lDummy,
                                                          normal, vDummy,
                                                          vDummy);

    if (gear->IsBogey() && !GroundReactions->GetSolid())
      continue;

    c.normal = Tec2b * normal;
    contacts.push_back(c);

    if (height < hmin) {
      hmin = height;
      contactRef = contacts.size() - 1;
    }
  }

  if (contacts.size() < 3)
    return;

  // Remove the contact point that is closest to the ground from the list:
  // the rotation axis will be going thru this point so we need to remove it
  // to avoid divisions by zero that could result from the computation of
  // the rotations.
  FGColumnVector3 contact0 = contacts[contactRef].location;
  contacts.erase(contacts.begin() + contactRef);

  // Update the initial conditions: this should remove the forces generated
  // by overcompressed landing gears
  fgic.SetAltitudeASLFtIC(fgic.GetAltitudeASLFtIC() - hmin);
  fdmex->Initialize(&fgic);
  fdmex->Run();

  // Compute the rotation axis: it is obtained from the direction of the
  // moment measured at the contact point 'contact0'
  FGColumnVector3 force = MassBalance->GetMass() * Accelerations->GetUVWdot();
  FGColumnVector3 moment = MassBalance->GetJ() * Accelerations->GetPQRdot()
    + force * contact0;
  FGColumnVector3 rotationAxis = moment.Normalize();

  // Compute the rotation parameters: angle and the first point to come into
  // contact with the ground when the rotation is applied.
  RotationParameters rParam = calcRotation(contacts, rotationAxis, contact0);
  FGQuaternion q0(rParam.angleMin, rotationAxis);

  // Apply the computed rotation to all the contact points
  FGMatrix33 rot = q0.GetTInv();
  vector<ContactPoints>::iterator iter;
  for (iter = contacts.begin(); iter != contacts.end(); ++iter)
    iter->location = contact0 + rot * (iter->location - contact0);

  // Remove the second point to come in contact with the ground from the list.
  // The reason is the same than above: avoid divisions by zero when the next
  // rotation will be computed.
  FGColumnVector3 contact1 = rParam.contactRef->location;
  contacts.erase(rParam.contactRef);

  // Compute the rotation axis: now there are 2 points in contact with the
  // ground so the only option for the aircraft is to rotate around the axis
  // generated by these 2 points.
  rotationAxis = contact1 - contact0;
  // Make sure that the rotation orientation is consistent with the moment.
  if (DotProduct(rotationAxis, moment) < 0.0)
    rotationAxis = contact0 - contact1;

  rotationAxis.Normalize();

  // Compute the rotation parameters
  rParam = calcRotation(contacts, rotationAxis, contact0);
  FGQuaternion q1(rParam.angleMin, rotationAxis);

  // Update the aircraft orientation
  FGColumnVector3 euler = (fgic.GetOrientation() * q0 * q1).GetEuler();

  fgic.SetPhiRadIC(euler(1));
  fgic.SetThetaRadIC(euler(2));
  fgic.SetPsiRadIC(euler(3));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Given a set of points and a rotation axis, this routine computes for each
// point the rotation angle that would drive the point in contact with the
// plane. It returns the minimum angle as well as the point with which this
// angle has been obtained.
// The rotation axis is defined by a vector 'u' and a point 'M0' on the axis.
// Since we are in the body frame, the position if 'M0' is measured from the CG
// hence the name 'GM0'.

FGTrim::RotationParameters FGTrim::calcRotation(vector<ContactPoints>& contacts,
                                                const FGColumnVector3& u,
                                                const FGColumnVector3& GM0)
{
  RotationParameters rParam;
  vector<ContactPoints>::iterator iter;

  rParam.angleMin = 3.0 * M_PI;

  for (iter = contacts.begin(); iter != contacts.end(); ++iter) {
    // Below the processed contact point is named 'M'
    // Construct an orthonormal basis (u, v, t). The ground normal is obtained
    // from iter->normal.
    FGColumnVector3 t = u * iter->normal;
    double length = t.Magnitude();
    t /= length; // Normalize the tangent
    FGColumnVector3 v = t * u;
    FGColumnVector3 MM0 = GM0 - iter->location;
    // d0 is the distance from the circle center 'C' to the reference point 'M0'
    double d0 = DotProduct(MM0, u);
    // Compute the square of the circle radius i.e. the square of the distance
    // between 'C' and 'M'.
    double sqrRadius = DotProduct(MM0, MM0) - d0 * d0;
    // Compute the distance from the circle center 'C' to the line made by the
    // intersection between the ground and the plane that contains the circle.
    double DistPlane = d0 * DotProduct(u, iter->normal) / length;
    // The coordinate of the point of intersection 'P' between the circle and
    // the ground is (0, DistPlane, alpha) in the basis (u, v, t)
    double mag = sqrRadius - DistPlane * DistPlane;
    if (mag < 0) {
      FGLogging log(fdmex->GetLogger(), LogLevel::WARN);
      log << "FGTrim::calcRotation DistPlane^2 larger than sqrRadius\n";
    }
    double alpha = sqrt(max(mag, 0.0));
    FGColumnVector3 CP = alpha * t + DistPlane * v;
    // The transformation is now constructed: we can extract the angle using the
    // classical formulas (cosine is obtained from the dot product and sine from
    // the cross product).
    double cosine = -DotProduct(MM0, CP) / sqrRadius;
    double sine = DotProduct(MM0 * u, CP) / sqrRadius;
    double angle = atan2(sine, cosine);
    if (angle < 0.0) angle += 2.0 * M_PI;
    if (angle < rParam.angleMin) {
      rParam.angleMin = angle;
      rParam.contactRef = iter;
    }
  }

  return rParam;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGTrim::solve(FGTrimAxis& axis) {

  double x1,x2,x3,f1,f2,f3,d,d0;
  const double relax =0.9;
  double eps=axis.GetSolverEps();

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
    //max_sub_iterations=axis.GetIterationLimit();
    while ( (axis.InTolerance() == false )
             && (fabs(d) > eps) && (Nsub < max_sub_iterations)) {
      Nsub++;
      d=(x3-x1)/d0;
      x2=x1-d*d0*f1/(f3-f1);
      axis.SetControl(x2);
      axis.Run();
      f2=axis.GetState();
      if(Debug > 1) {
        FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
        log << "FGTrim::solve Nsub,x1,x2,x3: " << Nsub << ", " << x1
        << ", " << x2 << ", " << x3 << "\n";
        log << "                             " << f1 << ", " << f2 << ", " << f3 << "\n";
      }
      if(f1*f2 <= 0.0) {
        x3=x2;
        f3=f2;
        f1=relax*f1;
        //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
        //log << "Solution is between x1 and x2" << "\n";
      }
      else if(f2*f3 <= 0.0) {
        x1=x2;
        f1=f2;
        f3=relax*f3;
        //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
        //log << "Solution is between x2 and x3" << "\n";

      }
      //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
      //log << i << "\n";


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
bool FGTrim::findInterval(FGTrimAxis& axis) {
  bool found=false;
  double step;
  double current_control=axis.GetControl();
  double current_accel=axis.GetState();;
  double xmin=axis.GetControlMin();
  double xmax=axis.GetControlMax();
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
    axis.SetControl(xlo);
    axis.Run();
    alo=axis.GetState();
    axis.SetControl(xhi);
    axis.Run();
    ahi=axis.GetState();
    if(fabs(ahi-alo) <= axis.GetTolerance()) continue;
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
    if(Debug > 1) {
      FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
      log << "FGTrim::findInterval: Nsub=" << Nsub << " Lo= " << xlo
          << " Hi= " << xhi << " alo*ahi: " << alo*ahi << "\n";
    }
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

bool FGTrim::checkLimits(FGTrimAxis& axis)
{
  bool solutionExists;
  double current_control=axis.GetControl();
  double current_accel=axis.GetState();
  xlo=axis.GetControlMin();
  xhi=axis.GetControlMax();

  axis.SetControl(xlo);
  axis.Run();
  alo=axis.GetState();
  axis.SetControl(xhi);
  axis.Run();
  ahi=axis.GetState();
  if(Debug > 1) {
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    log << "checkLimits() xlo,xhi,alo,ahi: " << xlo << ", " << xhi << ", "
        << alo << ", " << ahi << "\n";
  }
  solutionDomain=0;
  solutionExists=false;
  if(fabs(ahi-alo) > axis.GetTolerance()) {
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
  axis.SetControl(current_control);
  axis.Run();
  return solutionExists;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::setupPullup() {
  double g,q,cgamma;
  g=fdmex->GetInertial()->GetGravity().Magnitude();
  cgamma=cos(fgic.GetFlightPathAngleRadIC());
  FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
  log << "setPitchRateInPullup():  " << g << ", " << cgamma << ", "
       << fgic.GetVtrueFpsIC() << "\n";
  q=g*(targetNlf-cgamma)/fgic.GetVtrueFpsIC();
  log << targetNlf << ", " << q << "\n";
  fgic.SetQRadpsIC(q);
  log << "setPitchRateInPullup() complete\n";

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::setupTurn(void){
  double g,phi;
  phi = fgic.GetPhiRadIC();
  if( fabs(phi) > 0.001 && fabs(phi) < 1.56 ) {
    targetNlf = 1 / cos(phi);
    g = fdmex->GetInertial()->GetGravity().Magnitude();
    psidot = g*tan(phi) / fgic.GetUBodyFpsIC();
    FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
    log << targetNlf << ", " << psidot << "\n";
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::updateRates(void){
  if( mode == tTurn ) {
    double phi = fgic.GetPhiRadIC();
    double g = fdmex->GetInertial()->GetGravity().Magnitude();
    double p,q,r,theta;
    if(fabs(phi) > 0.001 && fabs(phi) < 1.56 ) {
      theta=fgic.GetThetaRadIC();
      phi=fgic.GetPhiRadIC();
      psidot = g*tan(phi) / fgic.GetUBodyFpsIC();
      p=-psidot*sin(theta);
      q=psidot*cos(theta)*sin(phi);
      r=psidot*cos(theta)*cos(phi);
    } else {
      p=q=r=0;
    }
    fgic.SetPRadpsIC(p);
    fgic.SetQRadpsIC(q);
    fgic.SetRRadpsIC(r);
  } else if( mode == tPullup && fabs(targetNlf-1) > 0.01) {
      double g,q,cgamma;
      g=fdmex->GetInertial()->GetGravity().Magnitude();
      cgamma=cos(fgic.GetFlightPathAngleRadIC());
      q=g*(targetNlf-cgamma)/fgic.GetVtrueFpsIC();
      fgic.SetQRadpsIC(q);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTrim::setDebug(FGTrimAxis& axis) {
  if(debug_axis == tAll ||
      axis.GetStateType() == debug_axis ) {
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
        if (debug_lvl > 0) {
          FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
          log << "  Full Trim\n";
        }
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tWdot,tAlpha));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tUdot,tThrottle ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tQdot,tPitchTrim ));
        //TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tHmgt,tBeta ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tVdot,tPhi ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tPdot,tAileron ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tRdot,tRudder ));
        break;
      case tLongitudinal:
        if (debug_lvl > 0) {
          FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
          log << "  Longitudinal Trim\n";
        }
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tWdot,tAlpha ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tUdot,tThrottle ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tQdot,tPitchTrim ));
        break;
      case tGround:
        if (debug_lvl > 0) {
          FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
          log << "  Ground Trim\n";
        }
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tWdot,tAltAGL ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tQdot,tTheta ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tPdot,tPhi ));
        break;
      case tPullup:
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tNlf,tAlpha ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tUdot,tThrottle ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tQdot,tPitchTrim ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tHmgt,tBeta ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tVdot,tPhi ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tPdot,tAileron ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tRdot,tRudder ));
        break;
      case tTurn:
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tWdot,tAlpha ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tUdot,tThrottle ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tQdot,tPitchTrim ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tVdot,tBeta ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tPdot,tAileron ));
        TrimAxes.push_back(FGTrimAxis(fdmex,&fgic,tRdot,tRudder ));
        break;
      case tCustom:
      case tNone:
        break;
    }
    //FGLogging log(fdmex->GetLogger(), LogLevel::INFO);
    //log << "TrimAxes.size(): " << TrimAxes.size() << "\n";
    sub_iterations.resize(TrimAxes.size());
    successful.resize(TrimAxes.size());
    solution.resize(TrimAxes.size());
}
//YOU WERE WARNED, BUT YOU DID IT ANYWAY.
}
