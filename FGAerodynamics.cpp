/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAerodynamics.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the aerodynamic forces (gear and collision)

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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
#include "FGPropagate.h"
#include "FGAircraft.h"
#include "FGState.h"
#include "FGMassBalance.h"
#include "FGFactorGroup.h"
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGAerodynamics.cpp,v 1.49 2004/04/17 21:16:19 jberndt Exp $";
static const char *IdHdr = ID_AERODYNAMICS;

const unsigned NAxes=6;
const char* AxisNames[] = { "drag", "side-force", "lift", "rolling-moment",
                            "pitching-moment","yawing-moment" };

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAerodynamics::FGAerodynamics(FGFDMExec* FDMExec) : FGModel(FDMExec)
{
  Name = "FGAerodynamics";

  AxisIdx["DRAG"]  = 0;
  AxisIdx["SIDE"]  = 1;
  AxisIdx["LIFT"]  = 2;
  AxisIdx["ROLL"]  = 3;
  AxisIdx["PITCH"] = 4;
  AxisIdx["YAW"]   = 5;

  Coeff = new CoeffArray[6];

  impending_stall = stall_hyst = 0.0;
  alphaclmin = alphaclmax = 0.0;
  alphahystmin = alphahystmax = 0.0;
  clsq = lod = 0.0;
  alphaw = 0.0;
  bi2vel = ci2vel = 0.0;
  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAerodynamics::~FGAerodynamics()
{
  unsigned int i,j;

  unbind();

  for (i=0; i<6; i++) {
    for (j=0; j<Coeff[i].size(); j++) {
      delete Coeff[i][j];
    }
  }
  delete[] Coeff;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::Run(void)
{
  unsigned int axis_ctr,ctr;
  double alpha, twovel;

  if (!FGModel::Run()) {

    twovel = 2*Auxiliary->GetVt();
    if (twovel != 0) {
      bi2vel = Aircraft->GetWingSpan() / twovel;
      ci2vel = Aircraft->Getcbar() / twovel;
    }

    alphaw = Auxiliary->Getalpha() + Aircraft->GetWingIncidence();

    alpha = Auxiliary->Getalpha();

    if (alphaclmax != 0) {
      if (alpha > 0.85*alphaclmax) {
        impending_stall = 10*(alpha/alphaclmax - 0.85);
      } else {
        impending_stall = 0;
      }
    }

    if (alphahystmax != 0.0 && alphahystmin != 0.0) {
      if (alpha > alphahystmax) {
         stall_hyst = 1;
      } else if (alpha < alphahystmin) {
         stall_hyst = 0;
      }
    }

    vLastFs = vFs;
    vFs.InitMatrix();

    for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
      for (ctr=0; ctr < Coeff[axis_ctr].size(); ctr++) {
        vFs(axis_ctr+1) += Coeff[axis_ctr][ctr]->TotalValue();
      }
    }

    //correct signs of drag and lift to wind axes convention
    //positive forward, right, down
    if ( Auxiliary->Getqbar() > 0) {
      clsq = vFs(eLift) / (Aircraft->GetWingArea()*Auxiliary->Getqbar());
      clsq *= clsq;
    }
    if ( vFs(eDrag)  > 0) {
      lod = vFs(eLift) / vFs(eDrag);
    }

    //correct signs of drag and lift to wind axes convention
    //positive forward, right, down
    vFs(eDrag)*=-1; vFs(eLift)*=-1;

    vForces = State->GetTs2b()*vFs;

    vDXYZcg = MassBalance->StructuralToBody(Aircraft->GetXYZrp());

    vMoments = vDXYZcg*vForces; // M = r X F

    for (axis_ctr = 0; axis_ctr < 3; axis_ctr++) {
      for (ctr = 0; ctr < Coeff[axis_ctr+3].size(); ctr++) {
        vMoments(axis_ctr+1) += Coeff[axis_ctr+3][ctr]->TotalValue();
      }
    }

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAerodynamics::Load(FGConfigFile* AC_cfg)
{
  string parameter, axis, scratch;

  AC_cfg->GetNextConfigLine();

  while ((parameter = AC_cfg->GetValue()) != string("/AERODYNAMICS")) {
    if (parameter == "AXIS") {
      CoeffArray ca;
      axis = AC_cfg->GetValue("NAME");
      AC_cfg->GetNextConfigLine();
      while ((parameter = AC_cfg->GetValue()) != string("/AXIS")) {
        if ( parameter == "COEFFICIENT" ) {
          ca.push_back( new FGCoefficient(FDMExec) );
          ca.back()->Load(AC_cfg);
        } else if ( parameter == "GROUP" ) {
          ca.push_back( new FGFactorGroup(FDMExec) );
          ca.back()->Load(AC_cfg);
        }
      }
      Coeff[AxisIdx[axis]] = ca;
      AC_cfg->GetNextConfigLine();
    } else if (parameter == "AC_ALPHALIMITS") {
      *AC_cfg >> scratch >> alphaclmin >> alphaclmax;
      if (debug_lvl > 0) cout << "    Maximum Alpha: " << alphaclmax
                              << "    Minimum Alpha: " << alphaclmin
                              << endl;
    } else if (parameter == "AC_HYSTLIMITS") {
      *AC_cfg >> scratch >> alphahystmin >> alphahystmax;
      if (debug_lvl > 0) cout << "    Hysteresis Start: " << alphahystmax
                              << "    Hysteresis End: " << alphahystmin
                              << endl;
    }
  }

  bindModel();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAerodynamics::GetCoefficientStrings(void)
{
  string CoeffStrings = "";
  bool firstime = true;
  unsigned int axis, sd;

  for (axis = 0; axis < 6; axis++) {
    for (sd = 0; sd < Coeff[axis].size(); sd++) {
      if (firstime) {
        firstime = false;
      } else {
        CoeffStrings += ", ";
      }
      CoeffStrings += Coeff[axis][sd]->GetCoefficientName();
    }
  }
  return CoeffStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGAerodynamics::GetCoefficientValues(void)
{
  string SDValues = "";
  bool firstime = true;

  for (unsigned int axis = 0; axis < 6; axis++) {
    for (unsigned int sd = 0; sd < Coeff[axis].size(); sd++) {
      if (firstime) {
        firstime = false;
      } else {
        SDValues += ", ";
      }
      SDValues += Coeff[axis][sd]->GetSDstring();
    }
  }

  return SDValues;
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
                       (PMF)&FGAerodynamics::GetvFs);
  PropertyManager->Tie("forces/fwy-aero-lbs", this,2,
                       (PMF)&FGAerodynamics::GetvFs);
  PropertyManager->Tie("forces/fwz-aero-lbs", this,3,
                       (PMF)&FGAerodynamics::GetvFs);
  PropertyManager->Tie("forces/lod-norm", this,
                       &FGAerodynamics::GetLoD);
  PropertyManager->Tie("aero/cl-squared-norm", this,
                       &FGAerodynamics::GetClSquared);
  PropertyManager->Tie("aero/alpha-max-deg", this,
                       &FGAerodynamics::GetAlphaCLMax,
                       &FGAerodynamics::SetAlphaCLMax,
                       true);
  PropertyManager->Tie("aero/alpha-min-deg", this,
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

void FGAerodynamics::bindModel(void)
{
  unsigned i,j;
  FGPropertyManager* node;
  string axis_node_name;
  node = PropertyManager->GetNode("aero/buildup",true);
  for (i=0;i<NAxes;i++) {
     node = node->GetNode( string(AxisNames[i]),true );
     for (j=0; j < Coeff[i].size(); j++) {
       Coeff[i][j]->bind(node);
     }
     node = (FGPropertyManager*)node->getParent();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAerodynamics::unbind(void)
{
  unsigned i,j;

  PropertyManager->Untie("forces/fbx-aero-lbs");
  PropertyManager->Untie("forces/fby-aero-lbs");
  PropertyManager->Untie("forces/fbz-aero-lbs");
  PropertyManager->Untie("moments/l-aero-lbsft");
  PropertyManager->Untie("moments/m-aero-lbsft");
  PropertyManager->Untie("moments/n-aero-lbsft");
  PropertyManager->Untie("forces/fwx-aero-lbs");
  PropertyManager->Untie("forces/fwy-aero-lbs");
  PropertyManager->Untie("forces/fwz-aero-lbs");
  PropertyManager->Untie("forces/lod-norm");
  PropertyManager->Untie("aero/cl-squared-norm");
  PropertyManager->Untie("aero/alpha-max-deg");
  PropertyManager->Untie("aero/alpha-min-deg");
  PropertyManager->Untie("aero/bi2vel");
  PropertyManager->Untie("aero/ci2vel");
  PropertyManager->Untie("aero/alpha-wing-rad");
  PropertyManager->Untie("aero/stall-hyst-norm");
  PropertyManager->Untie("systems/stall-warn-norm");

  for ( i=0; i<NAxes; i++ ) {
     for ( j=0; j < Coeff[i].size(); j++ ) {
       Coeff[i][j]->unbind();

     }
  }
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
    if (from == 0) { // Constructor

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
