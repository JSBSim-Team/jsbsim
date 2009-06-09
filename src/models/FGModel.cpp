/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGModel.cpp
 Author:       Jon Berndt
 Date started: 11/11/98
 Purpose:      Base class for all models
 Called by:    FGSimExec, et. al.

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
This base class for the FGAerodynamics, FGPropagate, etc. classes defines methods
common to all models.

HISTORY
--------------------------------------------------------------------------------
11/11/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGPropulsion.h"
#include "FGMassBalance.h"
#include "FGAerodynamics.h"
#include "FGInertial.h"
#include "FGGroundReactions.h"
#include "FGExternalReactions.h"
#include "FGAircraft.h"
#include "FGPropagate.h"
#include "FGAuxiliary.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGModel.cpp,v 1.9 2009/06/09 13:17:09 jberndt Exp $";
static const char *IdHdr = ID_MODEL;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGModel::FGModel(FGFDMExec* fdmex)
{
  FDMExec     = fdmex;
  NextModel   = 0L;

  State           = 0;
  Atmosphere      = 0;
  FCS             = 0;
  Propulsion      = 0;
  MassBalance     = 0;
  Aerodynamics    = 0;
  Inertial        = 0;
  GroundReactions = 0;
  ExternalReactions = 0;
  Aircraft        = 0;
  Propagate       = 0;
  Auxiliary       = 0;

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
  for (unsigned int i=0; i<interface_properties.size(); i++) delete interface_properties[i];
  interface_properties.clear();

  if (debug_lvl & 2) cout << "Destroyed:    FGModel" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModel::InitModel(void)
{
  State           = FDMExec->GetState();
  Atmosphere      = FDMExec->GetAtmosphere();
  FCS             = FDMExec->GetFCS();
  Propulsion      = FDMExec->GetPropulsion();
  MassBalance     = FDMExec->GetMassBalance();
  Aerodynamics    = FDMExec->GetAerodynamics();
  Inertial        = FDMExec->GetInertial();
  GroundReactions = FDMExec->GetGroundReactions();
  ExternalReactions = FDMExec->GetExternalReactions();
  BuoyantForces   = FDMExec->GetBuoyantForces();
  Aircraft        = FDMExec->GetAircraft();
  Propagate       = FDMExec->GetPropagate();
  Auxiliary       = FDMExec->GetAuxiliary();

  if (!State ||
      !Atmosphere ||
      !FCS ||
      !Propulsion ||
      !MassBalance ||
      !Aerodynamics ||
      !Inertial ||
      !GroundReactions ||
      !ExternalReactions ||
      !Aircraft ||
      !Propagate ||
      !Auxiliary) return(false);
  else return(true);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModel::Load(Element* el)
{
  // Interface properties are all stored in the interface properties array.

  string interface_property_string = "";

  Element *property_element = el->FindElement("property");
  if (property_element && debug_lvl > 0) cout << endl << "    Declared properties" << endl << endl;
  while (property_element) {
    interface_property_string = property_element->GetDataLine();
    if (PropertyManager->HasNode(interface_property_string)) {
      cerr << "      Property " << interface_property_string << " is already defined." << endl;
    } else {
      double value=0.0;
      if ( ! property_element->GetAttributeValue("value").empty())
        value = property_element->GetAttributeValueAsNumber("value");
      interface_properties.push_back(new double(value));
      PropertyManager->Tie(interface_property_string, interface_properties.back());
      if (debug_lvl > 0)
        cout << "      " << interface_property_string << " (initial value: " << value << ")" << endl;
    }
    property_element = el->FindNextElement("property");
  }
  
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGModel::Run()
{
  if (debug_lvl & 4) cout << "Entering Run() for model " << Name << endl;

  if (exe_ctr++ >= rate) exe_ctr = 1;

  if (exe_ctr == 1) return false;
  else              return true;
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
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
