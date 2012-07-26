/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAerodynamics.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the aerodynamic forces

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
09/13/00   JSB   Created
04/22/01   JSB   Moved code into here from FGAircraft

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include "FGFDMExec.h"
#include "FGAerodynamics.h"
#include "input_output/FGPropertyManager.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGAerodynamics.cpp,v 1.46 2012/07/26 04:33:46 jberndt Exp $";
static const char *IdHdr = ID_AERODYNAMICS;

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

  axisType = atNone;

  AeroFunctions = new AeroFunctionArray[6];

  impending_stall = stall_hyst = 0.0;
  alphaclmin = alphaclmax = 0.0;
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

  delete[] AeroFunctions;

  delete AeroRPShift;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  impending_stall = stall_hyst = 0.0;
  alphaclmin = alphaclmax = 0.0;
  alphahystmin = alphahystmax = 0.0;
  clsq = lod = 0.0;
  alphaw = 0.0;
  bi2vel = ci2vel = 0.0;
  AeroRPShift = 0;
  vDeltaRP.InitMatrix();

  return true;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::Run(bool Holding)
{

  if (FGModel::Run(Holding)) return true;
  if (Holding) return false; // if paused don't execute

  unsigned int axis_ctr, ctr;
  const double twovel=2*in.Vt;

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

  for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
    for (ctr=0; ctr < AeroFunctions[axis_ctr].size(); ctr++) {
      vFnative(axis_ctr+1) += AeroFunctions[axis_ctr][ctr]->GetValue();
    }
  }

  // Note that we still need to convert to wind axes here, because it is
  // used in the L/D calculation, and we still may want to look at Lift
  // and Drag.

  // JSB 4/27/12 - After use, convert wind axes to produce normal lift
  // and drag values - not negative ones!

  switch (axisType) {
    case atBodyXYZ:       // Forces already in body axes; no manipulation needed
      vFw = in.Tb2w*vFnative;
      vFw(eDrag)*=-1; vFw(eLift)*=-1;
      vForces = vFnative;
      break;
    case atLiftDrag:      // Copy forces into wind axes
      vFw = vFnative;
      vFw(eDrag)*=-1; vFw(eLift)*=-1;
      vForces = in.Tw2b*vFw;
      vFw(eDrag)*=-1; vFw(eLift)*=-1;
      break;
    case atAxialNormal:   // Convert native forces into Axial|Normal|Side system
      vFw = in.Tb2w*vFnative;
      vFnative(eX)*=-1; vFnative(eZ)*=-1;
      vForces = vFnative;
      break;
    default:
      cerr << endl << "  A proper axis type has NOT been selected. Check "
                   << "your aerodynamics definition." << endl;
      exit(-1);
  }

  // Calculate lift coefficient squared
  if ( in.Qbar > 0) {
    clsq = vFw(eLift) / (in.Wingarea*in.Qbar);
    clsq *= clsq;
  }

  // Calculate lift Lift over Drag
  if ( fabs(vFw(eDrag)) > 0.0) lod = fabs( vFw(eLift) / vFw(eDrag) );

  // Calculate aerodynamic reference point shift, if any. The shift
  // takes place in the structual axis. That is, if the shift is positive,
  // it is towards the back (tail) of the vehicle. The AeroRPShift
  // function should be non-dimensionalized by the wing chord. The
  // calculated vDeltaRP will be in feet.
  if (AeroRPShift) vDeltaRP(eX) = AeroRPShift->GetValue()*in.Wingchord;

  vDXYZcg(eX) = in.RPBody(eX) - vDeltaRP(eX); // vDeltaRP is given in the structural frame
  vDXYZcg(eY) = in.RPBody(eY) + vDeltaRP(eY);
  vDXYZcg(eZ) = in.RPBody(eZ) - vDeltaRP(eZ);

  vMomentsMRC.InitMatrix();

  for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
    for (ctr = 0; ctr < AeroFunctions[axis_ctr+3].size(); ctr++) {
      vMomentsMRC(axis_ctr+1) += AeroFunctions[axis_ctr+3][ctr]->GetValue();
    }
  }
  vMoments = vMomentsMRC + vDXYZcg*vForces; // M = r X F

  RunPostFunctions();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::Load(Element *element)
{
  string parameter, axis, scratch;
  string scratch_unit="";
  string fname="", file="";
  Element *temp_element, *axis_element, *function_element;

  string separator = "/";

  fname = element->GetAttributeValue("file");
  if (!fname.empty()) {
    file = FDMExec->GetFullAircraftPath() + separator + fname;
    document = LoadXMLDocument(file);
    if (document == 0L) return false;
  } else {
    document = element;
  }

  FGModel::Load(document); // Perform base class Pre-Load

  DetermineAxisSystem(); // Detemine if Lift/Side/Drag, etc. is used.

  Debug(2);

  if ((temp_element = document->FindElement("alphalimits"))) {
    scratch_unit = temp_element->GetAttributeValue("unit");
    if (scratch_unit.empty()) scratch_unit = "RAD";
    alphaclmin = temp_element->FindElementValueAsNumberConvertFromTo("min", scratch_unit, "RAD");
    alphaclmax = temp_element->FindElementValueAsNumberConvertFromTo("max", scratch_unit, "RAD");
  }

  if ((temp_element = document->FindElement("hysteresis_limits"))) {
    scratch_unit = temp_element->GetAttributeValue("unit");
    if (scratch_unit.empty()) scratch_unit = "RAD";
    alphahystmin = temp_element->FindElementValueAsNumberConvertFromTo("min", scratch_unit, "RAD");
    alphahystmax = temp_element->FindElementValueAsNumberConvertFromTo("max", scratch_unit, "RAD");
  }

  if ((temp_element = document->FindElement("aero_ref_pt_shift_x"))) {
    function_element = temp_element->FindElement("function");
    AeroRPShift = new FGFunction(PropertyManager, function_element);
  }

  axis_element = document->FindElement("axis");
  while (axis_element) {
    AeroFunctionArray ca;
    axis = axis_element->GetAttributeValue("name");
    function_element = axis_element->FindElement("function");
    while (function_element) {
      string current_func_name = function_element->GetAttributeValue("name");
      try {
        ca.push_back( new FGFunction(PropertyManager, function_element) );
      } catch (string const str) {
        cerr << endl << fgred << "Error loading aerodynamic function in " 
             << current_func_name << ":" << str << " Aborting." << reset << endl;
        return false;
      }
      function_element = axis_element->FindNextElement("function");
    }
    AeroFunctions[AxisIdx[axis]] = ca;
    axis_element = document->FindNextElement("axis");
  }

  PostLoad(document, PropertyManager); // Perform base class Post-Load

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

void FGAerodynamics::DetermineAxisSystem()
{
  Element* axis_element = document->FindElement("axis");
  string axis;
  while (axis_element) {
    axis = axis_element->GetAttributeValue("name");
    if (axis == "LIFT" || axis == "DRAG") {
      if (axisType == atNone) axisType = atLiftDrag;
      else if (axisType != atLiftDrag) {
        cerr << endl << "  Mixed aerodynamic axis systems have been used in the"
                     << " aircraft config file. (LIFT DRAG)" << endl;
      }
    } else if (axis == "SIDE") {
      if (axisType != atNone && axisType != atLiftDrag && axisType != atAxialNormal) {
        cerr << endl << "  Mixed aerodynamic axis systems have been used in the"
                     << " aircraft config file. (SIDE)" << endl;
      }
    } else if (axis == "AXIAL" || axis == "NORMAL") {
      if (axisType == atNone) axisType = atAxialNormal;
      else if (axisType != atAxialNormal) {
        cerr << endl << "  Mixed aerodynamic axis systems have been used in the"
                     << " aircraft config file. (NORMAL AXIAL)" << endl;
      }
    } else if (axis == "X" || axis == "Y" || axis == "Z") {
      if (axisType == atNone) axisType = atBodyXYZ;
      else if (axisType != atBodyXYZ) {
        cerr << endl << "  Mixed aerodynamic axis systems have been used in the"
                     << " aircraft config file. (XYZ)" << endl;
      }
    } else if (axis != "ROLL" && axis != "PITCH" && axis != "YAW") { // error
      cerr << endl << "  An unknown axis type, " << axis << " has been specified"
                   << " in the aircraft configuration file." << endl;
      exit(-1);
    }
    axis_element = document->FindNextElement("axis");
  }
  if (axisType == atNone) {
    axisType = atLiftDrag;
    cerr << endl << "  The aerodynamic axis system has been set by default"
                 << " to the Lift/Side/Drag system." << endl;
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

  if (FunctionStrings.size() > 0) {
    if (AeroFunctionStrings.size() > 0) {
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

  if (FunctionValues.size() > 0) {
    if (buf.str().size() > 0) {
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
  typedef double (FGAerodynamics::*PMF)(int) const;

  PropertyManager->Tie("forces/fbx-aero-lbs", this,1,
                       (PMF)&FGAerodynamics::GetForces);
  PropertyManager->Tie("forces/fby-aero-lbs", this,2,
                       (PMF)&FGAerodynamics::GetForces);
  PropertyManager->Tie("forces/fbz-aero-lbs", this,3,
                       (PMF)&FGAerodynamics::GetForces);
  PropertyManager->Tie("moments/l-aero-lbsft", this,1,
                       (PMF)&FGAerodynamics::GetMoments);
  PropertyManager->Tie("moments/m-aero-lbsft", this,2,
                       (PMF)&FGAerodynamics::GetMoments);
  PropertyManager->Tie("moments/n-aero-lbsft", this,3,
                       (PMF)&FGAerodynamics::GetMoments);
  PropertyManager->Tie("forces/fwx-aero-lbs", this,1,
                       (PMF)&FGAerodynamics::GetvFw);
  PropertyManager->Tie("forces/fwy-aero-lbs", this,2,
                       (PMF)&FGAerodynamics::GetvFw);
  PropertyManager->Tie("forces/fwz-aero-lbs", this,3,
                       (PMF)&FGAerodynamics::GetvFw);
  PropertyManager->Tie("forces/lod-norm", this,
                       &FGAerodynamics::GetLoD);
  PropertyManager->Tie("aero/cl-squared", this,
                       &FGAerodynamics::GetClSquared);
  PropertyManager->Tie("aero/qbar-area", &qbar_area);
  PropertyManager->Tie("aero/alpha-max-rad", this,
                       &FGAerodynamics::GetAlphaCLMax,
                       &FGAerodynamics::SetAlphaCLMax,
                       true);
  PropertyManager->Tie("aero/alpha-min-rad", this,
                       &FGAerodynamics::GetAlphaCLMin,
                       &FGAerodynamics::SetAlphaCLMin,
                       true);
  PropertyManager->Tie("aero/bi2vel", this,
                       &FGAerodynamics::GetBI2Vel);
  PropertyManager->Tie("aero/ci2vel", this,
                       &FGAerodynamics::GetCI2Vel);
  PropertyManager->Tie("aero/alpha-wing-rad", this,
                       &FGAerodynamics::GetAlphaW);
  PropertyManager->Tie("systems/stall-warn-norm", this,
                        &FGAerodynamics::GetStallWarn);
  PropertyManager->Tie("aero/stall-hyst-norm", this,
                        &FGAerodynamics::GetHysteresisParm);
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
      switch (axisType) {
        case (atLiftDrag):
          cout << endl << "  Aerodynamics (Lift|Side|Drag axes):" << endl << endl;
          break;
        case (atAxialNormal):
          cout << endl << "  Aerodynamics (Axial|Side|Normal axes):" << endl << endl;
          break;
        case (atBodyXYZ):
          cout << endl << "  Aerodynamics (X|Y|Z axes):" << endl << endl;
          break;
      case (atNone):
          cout << endl << "  Aerodynamics (undefined axes):" << endl << endl;
          break;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGAerodynamics" << endl;
    if (from == 1) cout << "Destroyed:    FGAerodynamics" << endl;
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

} // namespace JSBSim
