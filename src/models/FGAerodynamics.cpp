/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAerodynamics.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the aerodynamic forces

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
09/13/00   JSB   Created
04/22/01   JSB   Moved code into here from FGAircraft

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGAerodynamics.h"
#include "FGFDMExec.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAerodynamics::FGAerodynamics(FGFDMExec* FDMExec) : FGModel(FDMExec)
{
  Name = "FGAerodynamics";

  AxisIdx["DRAG"]   = 0;
  AxisIdx["SIDE"]   = 1;
  AxisIdx["LIFT"]   = 2;
  AxisIdx["ROLL"]   = 3;
  AxisIdx["PITCH"]  = 4;
  AxisIdx["YAW"]    = 5;

  AxisIdx["AXIAL"]  = 0;
  AxisIdx["NORMAL"] = 2;

  AxisIdx["X"] = 0;
  AxisIdx["Y"] = 1;
  AxisIdx["Z"] = 2;

  forceAxisType = atNone;
  momentAxisType = atNone;

  AeroFunctions = new AeroFunctionArray[6];
  AeroFunctionsAtCG = new AeroFunctionArray[6];

  impending_stall = stall_hyst = 0.0;
  alphaclmin = alphaclmax = 0.0;
  alphaclmin0 = alphaclmax0 = 0.0;
  alphahystmin = alphahystmax = 0.0;
  clsq = lod = 0.0;
  alphaw = 0.0;
  bi2vel = ci2vel = 0.0;
  AeroRPShift = 0;
  vDeltaRP.InitMatrix();

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAerodynamics::~FGAerodynamics()
{
  unsigned int i,j;

  for (i=0; i<6; i++)
    for (j=0; j<AeroFunctions[i].size(); j++)
      delete AeroFunctions[i][j];
  for (i=0; i<6; i++)
    for (j=0; j<AeroFunctionsAtCG[i].size(); j++)
      delete AeroFunctionsAtCG[i][j];

  delete[] AeroFunctions;
  delete[] AeroFunctionsAtCG;

  delete AeroRPShift;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  impending_stall = stall_hyst = 0.0;
  alphaclmin = alphaclmin0;
  alphaclmax = alphaclmax0;
  alphahystmin = alphahystmax = 0.0;
  clsq = lod = 0.0;
  alphaw = 0.0;
  bi2vel = ci2vel = 0.0;
  AeroRPShift = 0;
  vDeltaRP.InitMatrix();
  vForces.InitMatrix();
  vMoments.InitMatrix();
  return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::Run(bool Holding)
{

  if (FGModel::Run(Holding)) return true;
  if (Holding) return false; // if paused don't execute

  unsigned int axis_ctr;
  const double twovel=2*in.Vt;

  // The lift coefficient squared (property aero/cl-squared) is computed before
  // the aero functions are called to make sure that they use the same value for
  // qbar.
  if ( in.Qbar > 1.0) {
    // Skip the computation if qbar is close to zero to avoid huge values for
    // aero/cl-squared when a non-null lift coincides with a very small aero
    // velocity (i.e. when qbar is close to zero).
    clsq = vFw(eLift) / (in.Wingarea*in.Qbar);
    clsq *= clsq;
  }

  RunPreFunctions();

  // calculate some oft-used quantities for speed

  if (twovel != 0) {
    bi2vel = in.Wingspan / twovel;
    ci2vel = in.Wingchord / twovel;
  }
  alphaw = in.Alpha + in.Wingincidence;
  qbar_area = in.Wingarea * in.Qbar;

  if (alphaclmax != 0) {
    if (in.Alpha > 0.85*alphaclmax) {
      impending_stall = 10*(in.Alpha/alphaclmax - 0.85);
    } else {
      impending_stall = 0;
    }
  }

  if (alphahystmax != 0.0 && alphahystmin != 0.0) {
    if (in.Alpha > alphahystmax) {
      stall_hyst = 1;
    } else if (in.Alpha < alphahystmin) {
      stall_hyst = 0;
    }
  }

  vFw.InitMatrix();
  vFnative.InitMatrix();
  vFnativeAtCG.InitMatrix();

  BuildStabilityTransformMatrices();

  for (axis_ctr = 0; axis_ctr < 3; ++axis_ctr) {
    AeroFunctionArray::iterator f;

    AeroFunctionArray* array = &AeroFunctions[axis_ctr];
    for (f=array->begin(); f != array->end(); ++f) {
      // Tell the Functions to cache values, so when the function values are
      // being requested for output, the functions do not get calculated again
      // in a context that might have changed, but instead use the values that
      // have already been calculated for this frame.
      (*f)->cacheValue(true);
      vFnative(axis_ctr+1) += (*f)->GetValue();
    }

    array = &AeroFunctionsAtCG[axis_ctr];
    for (f=array->begin(); f != array->end(); ++f) {
      (*f)->cacheValue(true); // Same as above
      vFnativeAtCG(axis_ctr+1) += (*f)->GetValue();
    }
  }

  switch (forceAxisType) {
  case atBodyXYZ:       // Forces already in body axes; no manipulation needed
    vForces = vFnative;
    vForcesAtCG = vFnativeAtCG;
    break;
  case atWind:      // Copy forces into wind axes
    vFnative(eDrag)*=-1; vFnative(eLift)*=-1;
    vForces = in.Tw2b*vFnative;

    vFnativeAtCG(eDrag)*=-1; vFnativeAtCG(eLift)*=-1;
    vForcesAtCG = in.Tw2b*vFnativeAtCG;
    break;
  case atBodyAxialNormal:   // Convert native forces into Axial|Normal|Side system
    vFnative(eX)*=-1; vFnative(eZ)*=-1;
    vForces = vFnative;

    vFnativeAtCG(eX)*=-1; vFnativeAtCG(eZ)*=-1;
    vForcesAtCG = vFnativeAtCG;
    break;
  case atStability:   // Convert from stability axes to both body and wind axes
    vFnative(eDrag) *= -1; vFnative(eLift) *= -1;
    vForces = Ts2b*vFnative;

    vFnativeAtCG(eDrag) *= -1; vFnativeAtCG(eLift) *= -1;
    vForcesAtCG = Ts2b*vFnativeAtCG;
    break;
  default:
    {
      LogException err(FDMExec->GetLogger());
      err << "\n  A proper axis type has NOT been selected. Check "
          << "your aerodynamics definition.\n";
      throw err;
    }
  }
  // Calculate aerodynamic reference point shift, if any. The shift takes place
  // in the structual axis. That is, if the shift is positive, it is towards the
  // back (tail) of the vehicle. The AeroRPShift function should be
  // non-dimensionalized by the wing chord. The calculated vDeltaRP will be in
  // feet.
  if (AeroRPShift) vDeltaRP(eX) = AeroRPShift->GetValue()*in.Wingchord;

  vDXYZcg(eX) = in.RPBody(eX) - vDeltaRP(eX); // vDeltaRP is given in the
  vDXYZcg(eY) = in.RPBody(eY) + vDeltaRP(eY); // structural frame.
  vDXYZcg(eZ) = in.RPBody(eZ) - vDeltaRP(eZ);

  vMomentsMRC.InitMatrix();

  for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
    AeroFunctionArray* array = &AeroFunctions[axis_ctr+3];
    for (AeroFunctionArray::iterator f=array->begin(); f != array->end(); ++f) {
      // Tell the Functions to cache values, so when the function values are
      // being requested for output, the functions do not get calculated again
      // in a context that might have changed, but instead use the values that
      // have already been calculated for this frame.
      (*f)->cacheValue(true);
      vMomentsMRC(axis_ctr+1) += (*f)->GetValue();
    }
  }

  // Transform moments to bodyXYZ if the moments are specified in stability or
  // wind axes
  vMomentsMRCBodyXYZ.InitMatrix();
  switch (momentAxisType) {
  case atBodyXYZ:
    vMomentsMRCBodyXYZ = vMomentsMRC;
    break;
  case atStability:
    vMomentsMRCBodyXYZ = Ts2b*vMomentsMRC;
    break;
  case atWind:
    vMomentsMRCBodyXYZ = in.Tw2b*vMomentsMRC;
    break;
  default:
    {
      LogException err(FDMExec->GetLogger());
      err << "\n  A proper axis type has NOT been selected. Check "
          << "your aerodynamics definition.\n";
      throw err;
    }
  }

  vMoments = vMomentsMRCBodyXYZ + vDXYZcg*vForces; // M = r X F

  // Now add the "at CG" values to base forces - after the moments have been
  // transferred.
  vForces += vForcesAtCG;

  // Note that we still need to convert to wind axes here, because it is used in
  // the L/D calculation, and we still may want to look at Lift and Drag.
  //
  // JSB 4/27/12 - After use, convert wind axes to produce normal lift and drag
  // values - not negative ones!
  //
  // As a clarification, JSBSim assumes that drag and lift values are defined in
  // wind axes - BUT with a 180 rotation about the Y axis. That is, lift and
  // drag will be positive up and aft, respectively, so that they are reported
  // as positive numbers. However, the wind axes themselves assume that the X
  // and Z forces are positive forward and down. Same applies to the stability
  // axes.
  vFw = in.Tb2w * vForces;
  vFw(eDrag) *= -1; vFw(eLift) *= -1;

  // Calculate Lift over Drag
  if ( fabs(vFw(eDrag)) > 0.0)
    lod = fabs( vFw(eLift) / vFw(eDrag));

  RunPostFunctions();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGAerodynamics::GetForcesInStabilityAxes(void) const
{
  FGColumnVector3 vFs = Tb2s*vForces;
  // Need sign flips since drag is positive and lift is positive in stability axes
  vFs(eDrag) *= -1; vFs(eLift) *= -1;

  return vFs;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::Load(Element *document)
{
  string axis;
  string scratch_unit="";
  Element *temp_element, *axis_element, *function_element;

  Name = "Aerodynamics Model: " + document->GetAttributeValue("name");

  // Perform base class Pre-Load
  if (!FGModel::Upload(document, true))
    return false;

  DetermineAxisSystem(document); // Determine if Lift/Side/Drag, etc. is used.

  Debug(2);

  if ((temp_element = document->FindElement("alphalimits"))) {
    scratch_unit = temp_element->GetAttributeValue("unit");
    if (scratch_unit.empty()) scratch_unit = "RAD";
    alphaclmin0 = temp_element->FindElementValueAsNumberConvertFromTo("min", scratch_unit, "RAD");
    alphaclmax0 = temp_element->FindElementValueAsNumberConvertFromTo("max", scratch_unit, "RAD");
    alphaclmin = alphaclmin0;
    alphaclmax = alphaclmax0;
  }

  if ((temp_element = document->FindElement("hysteresis_limits"))) {
    scratch_unit = temp_element->GetAttributeValue("unit");
    if (scratch_unit.empty()) scratch_unit = "RAD";
    alphahystmin = temp_element->FindElementValueAsNumberConvertFromTo("min", scratch_unit, "RAD");
    alphahystmax = temp_element->FindElementValueAsNumberConvertFromTo("max", scratch_unit, "RAD");
  }

  if ((temp_element = document->FindElement("aero_ref_pt_shift_x"))) {
    function_element = temp_element->FindElement("function");
    AeroRPShift = new FGFunction(FDMExec, function_element);
  }

  axis_element = document->FindElement("axis");
  while (axis_element) {
    AeroFunctionArray ca;
    AeroFunctionArray ca_atCG;
    axis = axis_element->GetAttributeValue("name");
    function_element = axis_element->FindElement("function");
    while (function_element) {
      try {
        if (function_element->HasAttribute("apply_at_cg") &&
            function_element->GetAttributeValue("apply_at_cg") == "true")
          ca_atCG.push_back(new FGFunction(FDMExec, function_element));
        else
          ca.push_back(new FGFunction(FDMExec, function_element));
      } catch (BaseException& e) {
        string current_func_name = function_element->GetAttributeValue("name");
        FGXMLLogging log(FDMExec->GetLogger(), axis_element, LogLevel::ERROR);
        log << LogFormat::RED << "\nError loading aerodynamic function in "
            << current_func_name << ":" << e.what() << " Aborting.\n" << LogFormat::RESET;
        return false;
      }
      function_element = axis_element->FindNextElement("function");
    }
    AeroFunctions[AxisIdx[axis]] = ca;
    AeroFunctionsAtCG[AxisIdx[axis]] = ca_atCG;
    axis_element = document->FindNextElement("axis");
  }

  PostLoad(document, FDMExec); // Perform base class Post-Load

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// This private class function checks to verify consistency in the choice of
// aerodynamic axes used in the config file. One set of LIFT|DRAG|SIDE, or
// X|Y|Z, or AXIAL|NORMAL|SIDE must be chosen; mixed system axes are not allowed.
// Note that if the "SIDE" axis specifier is entered first in a config file,
// a warning message will be given IF the AXIAL|NORMAL specifiers are also given.
// This is OK, and the warning is due to the SIDE specifier used for both
// the Lift/Drag and Axial/Normal axis systems.
// Alternatively the axis name 'X|Y|Z or ROLL|PITCH|YAW' can be specified in
// conjunction with a frame 'BODY|STABILITY|WIND', for example:
// <axis name="X" frame="STABILITY"/>
//
// In summary, possible combinations:
//
// FORCES
//    Body
//       <axis name="AXIAL|SIDE|NORMAL" />
//       <axis name="X|Y|Z" />
//       <axis name="X|Y|Z" frame="BODY" />
//
//    Wind
//       <axis name="DRAG|SIDE|LIFT" / >
//       <axis name="X|Y|Z" frame="WIND" / >
//
//    Stability
//       <axis name="X|Y|Z" frame="STABILITY" />
//
// MOMENTS
//    Body
//       <axis name="ROLL|PITCH|YAW" />
//       <axis name="ROLL|PITCH|YAW" frame="BODY" / >
//
//    Wind
//       <axis name="ROLL|PITCH|YAW" frame="WIND" />
//
//    Stability
//       <axis name="ROLL|PITCH|YAW" frame="STABILITY" />
//

void FGAerodynamics::DetermineAxisSystem(Element* document)
{
  Element* axis_element = document->FindElement("axis");
  string axis;
  while (axis_element) {
    axis = axis_element->GetAttributeValue("name");
    string frame = axis_element->GetAttributeValue("frame");
    if (axis == "X" || axis == "Y" || axis == "Z") {
      ProcessAxesNameAndFrame(forceAxisType, axis, frame, axis_element,
                              "(X Y Z)");
    } else if (axis == "ROLL" || axis == "PITCH" || axis == "YAW") {
      ProcessAxesNameAndFrame(momentAxisType, axis, frame, axis_element,
                              "(ROLL PITCH YAW)");
    } else if (axis == "LIFT" || axis == "DRAG") {
      if (forceAxisType == atNone) forceAxisType = atWind;
      else if (forceAxisType != atWind) {
        FGXMLLogging log(FDMExec->GetLogger(), axis_element, LogLevel::WARN);
        log << "\n  Mixed aerodynamic axis systems have been used in the"
            << " aircraft config file. (LIFT DRAG)\n";
      }
    } else if (axis == "SIDE") {
      if (forceAxisType != atNone && forceAxisType != atWind && forceAxisType != atBodyAxialNormal) {
        FGXMLLogging log(FDMExec->GetLogger(), axis_element, LogLevel::WARN);
        log << "\n  Mixed aerodynamic axis systems have been used in the"
            << " aircraft config file. (SIDE)\n";
      }
    } else if (axis == "AXIAL" || axis == "NORMAL") {
      if (forceAxisType == atNone) forceAxisType = atBodyAxialNormal;
      else if (forceAxisType != atBodyAxialNormal) {
        FGXMLLogging log(FDMExec->GetLogger(), axis_element, LogLevel::WARN);
        log << "\n  Mixed aerodynamic axis systems have been used in the"
            << " aircraft config file. (NORMAL AXIAL)\n";
      }
    } else { // error
      XMLLogException err(FDMExec->GetLogger(), axis_element);
      err << "\n  An unknown axis type, " << axis << " has been specified"
          << " in the aircraft configuration file.\n";
      throw err;
    }
    axis_element = document->FindNextElement("axis");
  }

  if (forceAxisType == atNone) {
    forceAxisType = atWind;
    FGLogging log(FDMExec->GetLogger(), LogLevel::INFO);
    log << "\n  The aerodynamic axis system has been set by default"
        << " to the Lift/Side/Drag system.\n";
  }
  if (momentAxisType == atNone) {
    momentAxisType = atBodyXYZ;
    FGLogging log(FDMExec->GetLogger(), LogLevel::INFO);
    log << "\n  The aerodynamic moment axis system has been set by default"
        << " to the bodyXYZ system.\n";
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAerodynamics::ProcessAxesNameAndFrame(eAxisType& axisType, const string& name,
                                             const string& frame, Element* el,
                                             const string& validNames)
{
  if (frame == "BODY" || frame.empty()) {
    if (axisType == atNone) axisType = atBodyXYZ;
    else if (axisType != atBodyXYZ) {
      FGXMLLogging log(FDMExec->GetLogger(), el, LogLevel::WARN);
      log << "\n Mixed aerodynamic axis systems have been used in the "
          << " aircraft config file." << validNames << " - BODY\n";
    }
  }
  else if (frame == "STABILITY") {
    if (axisType == atNone) axisType = atStability;
    else if (axisType != atStability) {
      FGXMLLogging log(FDMExec->GetLogger(), el, LogLevel::WARN);
      log << "\n Mixed aerodynamic axis systems have been used in the "
          << " aircraft config file." << validNames << " - STABILITY\n";
    }
  }
  else if (frame == "WIND") {
    if (axisType == atNone) axisType = atWind;
    else if (axisType != atWind){
      FGXMLLogging log(FDMExec->GetLogger(), el, LogLevel::WARN);
      log << "\n Mixed aerodynamic axis systems have been used in the "
          << " aircraft config file." << validNames << " - WIND\n";
    }
  }
  else {
    XMLLogException err(FDMExec->GetLogger(), el);
    err << "\n Unknown axis frame type of - " << frame << "\n";
    throw err;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAerodynamics::GetAeroFunctionStrings(const string& delimeter) const
{
  string AeroFunctionStrings = "";
  bool firstime = true;
  unsigned int axis, sd;

  for (axis = 0; axis < 6; axis++) {
    for (sd = 0; sd < AeroFunctions[axis].size(); sd++) {
      if (firstime) {
        firstime = false;
      } else {
        AeroFunctionStrings += delimeter;
      }
      AeroFunctionStrings += AeroFunctions[axis][sd]->GetName();
    }
  }

  string FunctionStrings = FGModelFunctions::GetFunctionStrings(delimeter);

  if (!FunctionStrings.empty()) {
    if (!AeroFunctionStrings.empty()) {
      AeroFunctionStrings += delimeter + FunctionStrings;
    } else {
      AeroFunctionStrings = FunctionStrings;
    }
  }

  return AeroFunctionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAerodynamics::GetAeroFunctionValues(const string& delimeter) const
{
  ostringstream buf;

  for (unsigned int axis = 0; axis < 6; axis++) {
    for (unsigned int sd = 0; sd < AeroFunctions[axis].size(); sd++) {
      if (buf.tellp() > 0) buf << delimeter;
      buf << AeroFunctions[axis][sd]->GetValue();
    }
  }

  string FunctionValues = FGModelFunctions::GetFunctionValues(delimeter);

  if (!FunctionValues.empty()) {
    if (!buf.str().empty()) {
      buf << delimeter << FunctionValues;
    } else {
      buf << FunctionValues;
    }
  }

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAerodynamics::bind(void)
{
  PropertyManager->Tie("forces/fbx-aero-lbs",  this, eX, &FGAerodynamics::GetForces);
  PropertyManager->Tie("forces/fby-aero-lbs",  this, eY, &FGAerodynamics::GetForces);
  PropertyManager->Tie("forces/fbz-aero-lbs",  this, eZ, &FGAerodynamics::GetForces);
  PropertyManager->Tie("moments/l-aero-lbsft", this, eL, &FGAerodynamics::GetMoments);
  PropertyManager->Tie("moments/m-aero-lbsft", this, eM, &FGAerodynamics::GetMoments);
  PropertyManager->Tie("moments/n-aero-lbsft", this, eN, &FGAerodynamics::GetMoments);
  PropertyManager->Tie("forces/fwx-aero-lbs",  this, eDrag, &FGAerodynamics::GetvFw);
  PropertyManager->Tie("forces/fwy-aero-lbs",  this, eSide, &FGAerodynamics::GetvFw);
  PropertyManager->Tie("forces/fwz-aero-lbs",  this, eLift, &FGAerodynamics::GetvFw);
  PropertyManager->Tie("forces/fsx-aero-lbs",  this, eX, &FGAerodynamics::GetForcesInStabilityAxes);
  PropertyManager->Tie("forces/fsy-aero-lbs",  this, eY, &FGAerodynamics::GetForcesInStabilityAxes);
  PropertyManager->Tie("forces/fsz-aero-lbs",  this, eZ, &FGAerodynamics::GetForcesInStabilityAxes);
  PropertyManager->Tie("moments/roll-stab-aero-lbsft", this, eRoll, &FGAerodynamics::GetMomentsInStabilityAxes);
  PropertyManager->Tie("moments/pitch-stab-aero-lbsft", this, ePitch, &FGAerodynamics::GetMomentsInStabilityAxes);
  PropertyManager->Tie("moments/yaw-stab-aero-lbsft", this, eYaw, &FGAerodynamics::GetMomentsInStabilityAxes);
  PropertyManager->Tie("moments/roll-wind-aero-lbsft", this, eRoll, &FGAerodynamics::GetMomentsInWindAxes);
  PropertyManager->Tie("moments/pitch-wind-aero-lbsft", this, ePitch, &FGAerodynamics::GetMomentsInWindAxes);
  PropertyManager->Tie("moments/yaw-wind-aero-lbsft", this, eYaw, &FGAerodynamics::GetMomentsInWindAxes);
  PropertyManager->Tie("forces/lod-norm",      this, &FGAerodynamics::GetLoD);
  PropertyManager->Tie("aero/cl-squared",      this, &FGAerodynamics::GetClSquared);
  PropertyManager->Tie("aero/qbar-area", &qbar_area);
  PropertyManager->Tie("aero/alpha-max-rad",   this, &FGAerodynamics::GetAlphaCLMax, &FGAerodynamics::SetAlphaCLMax);
  PropertyManager->Tie("aero/alpha-min-rad",   this, &FGAerodynamics::GetAlphaCLMin, &FGAerodynamics::SetAlphaCLMin);
  PropertyManager->Tie("aero/bi2vel",          this, &FGAerodynamics::GetBI2Vel);
  PropertyManager->Tie("aero/ci2vel",          this, &FGAerodynamics::GetCI2Vel);
  PropertyManager->Tie("aero/alpha-wing-rad",  this, &FGAerodynamics::GetAlphaW);
  PropertyManager->Tie("systems/stall-warn-norm", this, &FGAerodynamics::GetStallWarn);
  PropertyManager->Tie("aero/stall-hyst-norm", this, &FGAerodynamics::GetHysteresisParm);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// Build transformation matrices for transforming from stability axes to
// body axes and to wind axes. Where "a" is alpha and "B" is beta:
//
// The transform from body to stability axes is:
//
//   cos(a)     0     sin(a)
//   0          1     0
//   -sin(a)    0     cos(a)
//
// The transform from stability to body axes is:
//
//   cos(a)     0     -sin(a)
//   0          1     0
//   sin(a)     0     cos(a)
//
//

void FGAerodynamics::BuildStabilityTransformMatrices(void)
{
  double ca = cos(in.Alpha);
  double sa = sin(in.Alpha);

  // Stability-to-body
  Ts2b(1, 1) = ca;
  Ts2b(1, 2) = 0.0;
  Ts2b(1, 3) = -sa;
  Ts2b(2, 1) = 0.0;
  Ts2b(2, 2) = 1.0;
  Ts2b(2, 3) = 0.0;
  Ts2b(3, 1) = sa;
  Ts2b(3, 2) = 0.0;
  Ts2b(3, 3) = ca;

  Tb2s = Ts2b.Transposed();
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

void FGAerodynamics::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 2) { // Loader
      FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
      switch (forceAxisType) {
      case (atWind):
        log << "\n  Aerodynamics (Lift|Side|Drag axes):\n\n";
        break;
      case (atBodyAxialNormal):
        log << "\n  Aerodynamics (Axial|Side|Normal axes):\n\n";
        break;
      case (atBodyXYZ):
        log << "\n  Aerodynamics (Body X|Y|Z axes):\n\n";
        break;
      case (atStability):
        log << "\n  Aerodynamics (Stability X|Y|Z axes):\n\n";
        break;
      case (atNone):
        log << "\n  Aerodynamics (undefined axes):\n\n";
        break;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGAerodynamics\n";
    if (from == 1) log << "Destroyed:    FGAerodynamics\n";
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

} // namespace JSBSim
