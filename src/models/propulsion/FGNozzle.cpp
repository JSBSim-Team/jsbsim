/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGNozzle.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the nozzle object

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
08/24/00  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGNozzle.h"
#include "FGFDMExec.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGNozzle::FGNozzle(FGFDMExec* FDMExec, Element* nozzle_element, int num)
  : FGThruster(FDMExec, nozzle_element, num)
{
  if (nozzle_element->FindElement("area"))
    Area = nozzle_element->FindElementValueAsNumberConvertTo("area", "FT2");
  else {
    XMLLogException err(fdmex->GetLogger(), nozzle_element);
    err << "Fatal Error: Nozzle exit area must be given in nozzle config file.\n";
    throw err;
  }

  Thrust = 0;
  Type = ttNozzle;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGNozzle::~FGNozzle()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGNozzle::Calculate(double vacThrust)
{
  Thrust = max((double)0.0, vacThrust - in.Pressure*Area);

  vFn(1) = Thrust * cos(ReverserAngle);

  return Thrust;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGNozzle::GetThrusterLabels(int id, const string& delimeter)
{
  std::ostringstream buf;

  buf << Name << " Thrust (engine " << id << " in lbs)";

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGNozzle::GetThrusterValues(int id, const string& delimeter)
{
  std::ostringstream buf;

  buf << Thrust;

  return buf.str();
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

void FGNozzle::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
      log << "      Nozzle Name: " << Name << "\n";
      log << "      Nozzle Exit Area = " << Area << "\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGNozzle\n";
    if (from == 1) log << "Destroyed:    FGNozzle\n";
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
