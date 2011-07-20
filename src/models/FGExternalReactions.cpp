/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGExternalReactions.cpp
 Author:       David P. Culp
 Date started: 17/11/06
 Purpose:      Manages the External Forces
 Called by:    FGAircraft

 ------------- Copyright (C) 2006  David P. Culp (davidculp2@comcast.net) -------------

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
17/11/06   DC   Created

/%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGExternalReactions.h"
#include "input_output/FGXMLElement.h"
#include <iostream>
#include <string>

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DATA
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

static const char *IdSrc = "$Id: FGExternalReactions.cpp,v 1.12 2011/07/20 12:16:34 jberndt Exp $";
static const char *IdHdr = ID_EXTERNALREACTIONS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGExternalReactions::FGExternalReactions(FGFDMExec* fdmex) : FGModel(fdmex)
{
  NoneDefined = true;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGExternalReactions::Load(Element* el)
{
  // check if a file attribute was specified
  string fname = el->GetAttributeValue("file");
  if (!fname.empty()) {
    string file = FDMExec->GetFullAircraftPath() + "/" + fname;
    el = LoadXMLDocument(file);
    if (el == 0L) return false;
  }

  FGModel::Load(el); // Call the base class Load() function to load interface properties.

  Debug(2);

  // Parse force elements

  int index=0;
  Element* force_element = el->FindElement("force");
  while (force_element) {
    Forces.push_back( new FGExternalForce(FDMExec, force_element, index) );
    NoneDefined = false;
    index++; 
    force_element = el->FindNextElement("force");
  }

  PostLoad(el, PropertyManager);

  if (!NoneDefined) bind();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGExternalReactions::~FGExternalReactions()
{
  for (unsigned int i=0; i<Forces.size(); i++) delete Forces[i];
  Forces.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGExternalReactions::InitModel(void)
{
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGExternalReactions::Run(bool Holding)
{
  if (FGModel::Run(Holding)) return true;
  if (Holding) return false; // if paused don't execute
  if (NoneDefined) return true;

  RunPreFunctions();

  vTotalForces.InitMatrix();
  vTotalMoments.InitMatrix();

  for (unsigned int i=0; i<Forces.size(); i++) {
    vTotalForces  += Forces[i]->GetBodyForces();
    vTotalMoments += Forces[i]->GetMoments();
  }

  RunPostFunctions();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGExternalReactions::bind(void)
{
  typedef double (FGExternalReactions::*PMF)(int) const;
  PropertyManager->Tie("moments/l-external-lbsft", this, eL, (PMF)&FGExternalReactions::GetMoments);
  PropertyManager->Tie("moments/m-external-lbsft", this, eM, (PMF)&FGExternalReactions::GetMoments);
  PropertyManager->Tie("moments/n-external-lbsft", this, eN, (PMF)&FGExternalReactions::GetMoments);
  PropertyManager->Tie("forces/fbx-external-lbs", this, eX, (PMF)&FGExternalReactions::GetForces);
  PropertyManager->Tie("forces/fby-external-lbs", this, eY, (PMF)&FGExternalReactions::GetForces);
  PropertyManager->Tie("forces/fbz-external-lbs", this, eZ, (PMF)&FGExternalReactions::GetForces);
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

void FGExternalReactions::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor - loading and initialization
    }
    if (from == 2) { // Loading
      cout << endl << "  External Reactions: " << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGExternalReactions" << endl;
    if (from == 1) cout << "Destroyed:    FGExternalReactions" << endl;
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

