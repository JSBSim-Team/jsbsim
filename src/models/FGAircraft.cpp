/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAircraft.cpp
 Author:       Jon S. Berndt
 Date started: 12/12/98
 Purpose:      Encapsulates an aircraft
 Called by:    FGFDMExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
Models the aircraft reactions and forces. This class is instantiated by the
FGFDMExec class and scheduled as an FDM entry.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <sys/stat.h>
#include <sys/types.h>

#include <cmath>

#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGInertial.h"
#include "FGGroundReactions.h"
#include "FGExternalReactions.h"
#include "FGBuoyantForces.h"
#include "FGAerodynamics.h"
#include <FGFDMExec.h>
#include "FGPropagate.h"
#include <input_output/FGPropertyManager.h>

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: FGAircraft.cpp,v 1.20 2009/06/09 03:23:54 jberndt Exp $";
static const char *IdHdr = ID_AIRCRAFT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGAircraft::FGAircraft(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGAircraft";
  WingSpan = 0.0;
  HTailArea = VTailArea = 0.0;
  HTailArm  = VTailArm  = 0.0;
  lbarh = lbarv = 0.0;
  vbarh = vbarv = 0.0;
  WingIncidence = 0.0;
  HoldDown = 0;

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAircraft::~FGAircraft()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::Run(void)
{
  if (FGModel::Run()) return true;
  if (FDMExec->Holding()) return false;

  vForces.InitMatrix();
  if (!HoldDown) {
    vForces += Aerodynamics->GetForces();
    vForces += Propulsion->GetForces();
    vForces += GroundReactions->GetForces();
    vForces += ExternalReactions->GetForces();
    vForces += BuoyantForces->GetForces();
  }

  vMoments.InitMatrix();
  if (!HoldDown) {
    vMoments += Aerodynamics->GetMoments();
    vMoments += Propulsion->GetMoments();
    vMoments += GroundReactions->GetMoments();
    vMoments += ExternalReactions->GetMoments();
    vMoments += BuoyantForces->GetMoments();
  }

  vBodyAccel = vForces/MassBalance->GetMass();

  vNcg = vBodyAccel/Inertial->gravity();

  vNwcg = Aerodynamics->GetTb2w() * vNcg;
  vNwcg(3) = -1*vNwcg(3) + 1;

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGAircraft::GetNlf(void) const
{
  return -1*Aerodynamics->GetvFw(3)/MassBalance->GetWeight();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAircraft::Load(Element* el)
{
  string element_name;
  Element* element;

  FGModel::Load(el);

  if (el->FindElement("wingarea"))
    WingArea = el->FindElementValueAsNumberConvertTo("wingarea", "FT2");
  if (el->FindElement("wingspan"))
    WingSpan = el->FindElementValueAsNumberConvertTo("wingspan", "FT");
  if (el->FindElement("chord"))
    cbar = el->FindElementValueAsNumberConvertTo("chord", "FT");
  if (el->FindElement("wing_incidence"))
    WingIncidence = el->FindElementValueAsNumberConvertTo("wing_incidence", "RAD");
  if (el->FindElement("htailarea"))
    HTailArea = el->FindElementValueAsNumberConvertTo("htailarea", "FT2");
  if (el->FindElement("htailarm"))
    HTailArm = el->FindElementValueAsNumberConvertTo("htailarm", "FT");
  if (el->FindElement("vtailarea"))
    VTailArea = el->FindElementValueAsNumberConvertTo("vtailarea", "FT2");
  if (el->FindElement("vtailarm"))
    VTailArm = el->FindElementValueAsNumberConvertTo("vtailarm", "FT");

  // Find all LOCATION elements that descend from this METRICS branch of the
  // config file. This would be CG location, eyepoint, etc.

  element = el->FindElement("location");
  while (element) {
    element_name = element->GetAttributeValue("name");

    if (element_name == "AERORP") vXYZrp = element->FindElementTripletConvertTo("IN");
    else if (element_name == "EYEPOINT") vXYZep = element->FindElementTripletConvertTo("IN");
    else if (element_name == "VRP") vXYZvrp = element->FindElementTripletConvertTo("IN");

    element = el->FindNextElement("location");
  }

  // calculate some derived parameters
  if (cbar != 0.0) {
    lbarh = HTailArm/cbar;
    lbarv = VTailArm/cbar;
    if (WingArea != 0.0) {
      vbarh = HTailArm*HTailArea / (cbar*WingArea);
      vbarv = VTailArm*VTailArea / (WingSpan*WingArea);
    }
  }

  Debug(2);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAircraft::bind(void)
{
  typedef double (FGAircraft::*PMF)(int) const;
  PropertyManager->Tie("metrics/Sw-sqft", this, &FGAircraft::GetWingArea, &FGAircraft::SetWingArea);
  PropertyManager->Tie("metrics/bw-ft", this, &FGAircraft::GetWingSpan);
  PropertyManager->Tie("metrics/cbarw-ft", this, &FGAircraft::Getcbar);
  PropertyManager->Tie("metrics/iw-rad", this, &FGAircraft::GetWingIncidence);
  PropertyManager->Tie("metrics/iw-deg", this, &FGAircraft::GetWingIncidenceDeg);
  PropertyManager->Tie("metrics/Sh-sqft", this, &FGAircraft::GetHTailArea);
  PropertyManager->Tie("metrics/lh-ft", this, &FGAircraft::GetHTailArm);
  PropertyManager->Tie("metrics/Sv-sqft", this, &FGAircraft::GetVTailArea);
  PropertyManager->Tie("metrics/lv-ft", this, &FGAircraft::GetVTailArm);
  PropertyManager->Tie("metrics/lh-norm", this, &FGAircraft::Getlbarh);
  PropertyManager->Tie("metrics/lv-norm", this, &FGAircraft::Getlbarv);
  PropertyManager->Tie("metrics/vbarh-norm", this, &FGAircraft::Getvbarh);
  PropertyManager->Tie("metrics/vbarv-norm", this, &FGAircraft::Getvbarv);
  PropertyManager->Tie("metrics/aero-rp-x-in", this, eX, (PMF)&FGAircraft::GetXYZrp);
  PropertyManager->Tie("metrics/aero-rp-y-in", this, eY, (PMF)&FGAircraft::GetXYZrp);
  PropertyManager->Tie("metrics/aero-rp-z-in", this, eZ, (PMF)&FGAircraft::GetXYZrp);
  PropertyManager->Tie("metrics/eyepoint-x-in", this, eX, (PMF)&FGAircraft::GetXYZep);
  PropertyManager->Tie("metrics/eyepoint-y-in", this, eY,(PMF)&FGAircraft::GetXYZep);
  PropertyManager->Tie("metrics/eyepoint-z-in", this, eZ, (PMF)&FGAircraft::GetXYZep);
  PropertyManager->Tie("metrics/visualrefpoint-x-in", this, eX, (PMF)&FGAircraft::GetXYZvrp);
  PropertyManager->Tie("metrics/visualrefpoint-y-in", this, eY, (PMF)&FGAircraft::GetXYZvrp);
  PropertyManager->Tie("metrics/visualrefpoint-z-in", this, eZ, (PMF)&FGAircraft::GetXYZvrp);
  PropertyManager->Tie("forces/fbx-total-lbs", this, eX, (PMF)&FGAircraft::GetForces);
  PropertyManager->Tie("forces/fby-total-lbs", this, eY, (PMF)&FGAircraft::GetForces);
  PropertyManager->Tie("forces/fbz-total-lbs", this, eZ, (PMF)&FGAircraft::GetForces);
  PropertyManager->Tie("forces/load-factor", this, &FGAircraft::GetNlf);
  PropertyManager->Tie("forces/hold-down", this, &FGAircraft::GetHoldDown, &FGAircraft::SetHoldDown);
  PropertyManager->Tie("moments/l-total-lbsft", this, eL, (PMF)&FGAircraft::GetMoments);
  PropertyManager->Tie("moments/m-total-lbsft", this, eM, (PMF)&FGAircraft::GetMoments);
  PropertyManager->Tie("moments/n-total-lbsft", this, eN, (PMF)&FGAircraft::GetMoments);
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

void FGAircraft::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 2) { // Loading
      cout << endl << "  Aircraft Metrics:" << endl;
      cout << "    WingArea: " << WingArea  << endl;
      cout << "    WingSpan: " << WingSpan  << endl;
      cout << "    Incidence: " << WingIncidence << endl;
      cout << "    Chord: " << cbar << endl;
      cout << "    H. Tail Area: " << HTailArea << endl;
      cout << "    H. Tail Arm: " << HTailArm << endl;
      cout << "    V. Tail Area: " << VTailArea << endl;
      cout << "    V. Tail Arm: " << VTailArm << endl;
      cout << "    Eyepoint (x, y, z): " << vXYZep << endl;
      cout << "    Ref Pt (x, y, z): " << vXYZrp << endl;
      cout << "    Visual Ref Pt (x, y, z): " << vXYZvrp << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGAircraft" << endl;
    if (from == 1) cout << "Destroyed:    FGAircraft" << endl;
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
