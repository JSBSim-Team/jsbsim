/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGGroundReactions.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the ground reaction forces (gear and collision)

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <sstream>
#include <iomanip>

#include "FGGroundReactions.h"
#include <input_output/FGPropertyManager.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGGroundReactions.cpp,v 1.4 2005/07/02 16:58:58 jberndt Exp $";
static const char *IdHdr = ID_GROUNDREACTIONS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGGroundReactions::FGGroundReactions(FGFDMExec* fgex) : FGModel(fgex)
{
  Name = "FGGroundReactions";

  ActiveGearUnit = 0;
  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGroundReactions::~FGGroundReactions(void)
{
  unbind();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundReactions::Run(void)
{
  if (FGModel::Run()) return true;

  vForces.InitMatrix();
  vMoments.InitMatrix();

  if ( Propagate->GetDistanceAGL() < 300.0 ) { // Only execute gear code below 300 feet
    vector <FGLGear>::iterator iGear = lGear.begin();

    // Sum forces and moments for all gear, here.
    // Some optimizations may be made here - or rather in the gear code itself.
    // The gear ::Run() method is called several times - once for each gear.
    // Perhaps there is some commonality for things which only need to be
    // calculated once.

    ActiveGearUnit = 0;
    while (iGear != lGear.end()) {
      vForces  += iGear->Force();
      vMoments += iGear->Moment();
      iGear++;
      ActiveGearUnit++;
    }

  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGGroundReactions::GetSlipAngle(void) const
{
  char property_name[80];
  snprintf(property_name, 80, "gear/unit[%d]/slip-angle-deg", ActiveGearUnit);
  return PropertyManager->getDoubleValue(property_name);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundReactions::Load(Element* el)
{
  Element* contact_element = el->FindElement("contact");
  int num=0;

  Debug(2);

  while (contact_element) {
    lGear.push_back(FGLGear(contact_element, FDMExec, num++));
    FCS->AddGear(); // make the FCS aware of the landing gear
    contact_element = el->FindNextElement("contact");
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGGroundReactions::GetGroundReactionStrings(string delimeter)
{
  std::ostringstream buf;

  for (unsigned int i=0;i<lGear.size();i++) {
    string name = lGear[i].GetName();
    buf << name << "_WOW" << delimeter
        << name << "_stroke" << delimeter
        << name << "_strokeVel" << delimeter
        << name << "_CompressForce" << delimeter
        << name << "_WhlSideForce" << delimeter
        << name << "_WhlVelVecX" << delimeter
        << name << "_WhlVelVecY" << delimeter
        << name << "_WhlRollForce" << delimeter
        << name << "_BodyXForce" << delimeter
        << name << "_BodyYForce" << delimeter
        << name << "_WhlSlipDegrees" << delimeter;
  }

  buf << "TotalGearForce_X" << delimeter
      << "TotalGearForce_Y" << delimeter
      << "TotalGearForce_Z" << delimeter
      << "TotalGearMoment_L" << delimeter
      << "TotalGearMoment_M" << delimeter
      << "TotalGearMoment_N";

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGGroundReactions::GetGroundReactionValues(string delimeter)
{
  std::ostringstream buf;

  for (unsigned int i=0;i<lGear.size();i++) {
    FGLGear& gear = lGear[i];
    buf << (gear.GetWOW() ? "1, " : "0, ")
        << setprecision(5) << gear.GetCompLen() << delimeter
        << setprecision(6) << gear.GetCompVel() << delimeter
        << setprecision(10) << gear.GetCompForce() << delimeter
        << setprecision(6) << gear.GetWheelVel(eX) << delimeter
        << gear.GetWheelVel(eY) << delimeter
        << gear.GetWheelSideForce() << delimeter
        << gear.GetWheelRollForce() << delimeter
        << gear.GetBodyXForce() << delimeter
        << gear.GetBodyYForce() << delimeter
        << gear.GetWheelSlipAngle() << delimeter;
  }

  buf << vForces(eX) << delimeter
      << vForces(eY) << delimeter
      << vForces(eZ) << delimeter
      << vMoments(eX) << delimeter
      << vMoments(eY) << delimeter
      << vMoments(eZ);

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGroundReactions::bind(void)
{
  typedef double (FGGroundReactions::*PMF)(int) const;
  PropertyManager->Tie("gear/num-units", this, &FGGroundReactions::GetNumGearUnits);
  PropertyManager->Tie("gear/slip-angle-deg", this, &FGGroundReactions::GetSlipAngle);
  PropertyManager->Tie("moments/l-gear-lbsft", this, eL, (PMF)&FGGroundReactions::GetMoments);
  PropertyManager->Tie("moments/m-gear-lbsft", this, eM, (PMF)&FGGroundReactions::GetMoments);
  PropertyManager->Tie("moments/n-gear-lbsft", this, eN, (PMF)&FGGroundReactions::GetMoments);
  PropertyManager->Tie("forces/fbx-gear-lbs", this, eX, (PMF)&FGGroundReactions::GetForces);
  PropertyManager->Tie("forces/fby-gear-lbs", this, eY, (PMF)&FGGroundReactions::GetForces);
  PropertyManager->Tie("forces/fbz-gear-lbs", this, eZ, (PMF)&FGGroundReactions::GetForces);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGroundReactions::unbind(void)
{
  PropertyManager->Untie("gear/num-units");
  PropertyManager->Untie("gear/slip-angle-deg");
  PropertyManager->Untie("moments/l-gear-lbsft");
  PropertyManager->Untie("moments/m-gear-lbsft");
  PropertyManager->Untie("moments/n-gear-lbsft");
  PropertyManager->Untie("forces/fbx-gear-lbs");
  PropertyManager->Untie("forces/fby-gear-lbs");
  PropertyManager->Untie("forces/fbz-gear-lbs");
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

void FGGroundReactions::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 2) { // Loading
      cout << endl << "  Ground Reactions: " << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGGroundReactions" << endl;
    if (from == 1) cout << "Destroyed:    FGGroundReactions" << endl;
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
