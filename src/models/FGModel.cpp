/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGModel.cpp
 Author:       Jon Berndt
 Date started: 11/11/98
 Purpose:      Base class for all models
 Called by:    FGSimExec, et. al.

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
This base class for the FGAerodynamics, FGPropagate, etc. classes defines
methods common to all models.

HISTORY
--------------------------------------------------------------------------------
11/11/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGFDMExec.h"
#include "input_output/FGModelLoader.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGModel::FGModel(FGFDMExec* fdmex)
{
  FDMExec     = fdmex;

  //in order for FGModel derived classes to self-bind (that is, call
  //their bind function in the constructor, the PropertyManager pointer
  //must be brought up now.
  PropertyManager = FDMExec->GetPropertyManager();

  exe_ctr     = 1;
  rate        = 1;

  if (debug_lvl & 2) cout << "              FGModel Base Class" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGModel::~FGModel()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGModel" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModel::InitModel(void)
{
  exe_ctr = 1;
  return FGModelFunctions::InitModel();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModel::Run(bool Holding)
{
  if (debug_lvl & 4) cout << "Entering Run() for model " << Name << endl;

  if (rate == 1) return false; // Fast exit if nothing to do

  if (exe_ctr >= rate) exe_ctr = 0;

  if (exe_ctr++ == 1) return false;
  else              return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SGPath FGModel::FindFullPathName(const SGPath& path) const
{
  return CheckPathName(FDMExec->GetFullAircraftPath(), path);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModel::Upload(Element* el, bool preLoad)
{
  FGModelLoader ModelLoader(this);
  Element* document = ModelLoader.Open(el);

  if (!document) return false;

  if (document->GetName() != el->GetName()) {
    cerr << el->ReadFrom()
         << " Read model '" << document->GetName()
         << "' while expecting model '" << el->GetName() << "'" << endl;
    return false;
  }

  bool result = true;

  if (preLoad)
    result = FGModelFunctions::Load(document, FDMExec);

  if (document != el) {
    el->MergeAttributes(document);

    if (preLoad) {
      // After reading interface properties in a file, read properties in the
      // local model element. This allows general-purpose models to be defined
      // in a file, with overrides or initial loaded constants supplied in the
      // relevant element of the aircraft configuration file.
      LocalProperties.Load(el, PropertyManager, true);
    }

    Element* element = document->FindElement();
    while (element) {
      el->AddChildElement(element);
      element->SetParent(el);
      element = document->FindNextElement();
    }
  }

  return result;
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

void FGModel::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGModel" << endl;
    if (from == 1) cout << "Destroyed:    FGModel" << endl;
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
}
