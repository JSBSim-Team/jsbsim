/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTank.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft

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
See header file.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <FGFDMExec.h>
#include <models/FGAuxiliary.h>
#include "FGTank.h"

#if !defined ( sgi ) || defined( __GNUC__ ) && (_COMPILER_VERSION < 740)
using std::cerr;
using std::endl;
using std::cout;
#endif

namespace JSBSim {

static const char *IdSrc = "$Id: FGTank.cpp,v 1.8 2008/04/29 12:09:41 jberndt Exp $";
static const char *IdHdr = ID_TANK;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTank::FGTank(FGFDMExec* exec, Element* el, int tank_number)
                  : TankNumber(tank_number)
{
  string token;
  Element* element;
  Area = 1.0;
  Temperature = -9999.0;
  Auxiliary = exec->GetAuxiliary();
  Radius = Capacity = Contents = Standpipe = 0.0;
  PropertyManager = exec->GetPropertyManager();

  type = el->GetAttributeValue("type");
  if      (type == "FUEL")     Type = ttFUEL;
  else if (type == "OXIDIZER") Type = ttOXIDIZER;
  else                         Type = ttUNKNOWN;

  element = el->FindElement("location");
  if (element)  vXYZ = element->FindElementTripletConvertTo("IN");
  else          cerr << "No location found for this tank." << endl;

  if (el->FindElement("radius"))
    Radius = el->FindElementValueAsNumberConvertTo("radius", "IN");
  if (el->FindElement("capacity"))
    Capacity = el->FindElementValueAsNumberConvertTo("capacity", "LBS");
  if (el->FindElement("contents"))
    Contents = el->FindElementValueAsNumberConvertTo("contents", "LBS");
  if (el->FindElement("temperature"))
    Temperature = el->FindElementValueAsNumber("temperature");
  if (el->FindElement("standpipe"))
    Standpipe = el->FindElementValueAsNumberConvertTo("standpipe", "LBS");

  Selected = true;

  if (Capacity != 0) {
    PctFull = 100.0*Contents/Capacity;            // percent full; 0 to 100.0
  } else {
    Contents = 0;
    PctFull  = 0;
  }

  char property_name[80];
  snprintf(property_name, 80, "propulsion/tank[%d]/contents-lbs", TankNumber);
  PropertyManager->Tie( property_name, (FGTank*)this, &FGTank::GetContents,
                                       &FGTank::SetContents );

  if (Temperature != -9999.0)  Temperature = FahrenheitToCelsius(Temperature);
  Area = 40.0 * pow(Capacity/1975, 0.666666667);

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTank::~FGTank()
{
  char property_name[80];
  snprintf(property_name, 80, "propulsion/tank[%d]/contents-lbs", TankNumber);
  PropertyManager->Untie( property_name );
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::Drain(double used)
{
  double remaining = Contents - used;

  if (remaining >= 0) { // Reduce contents by amount used.

    Contents -= used;
    PctFull = 100.0*Contents/Capacity;

  } else { // This tank must be empty.

    Contents = 0.0;
    PctFull = 0.0;
    Selected = false;
  }
  return remaining;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::Fill(double amount)
{
  double overage = 0.0;

  Contents += amount;

  if (Contents > Capacity) {
    overage = Contents - Capacity;
    Contents = Capacity;
    PctFull = 100.0;
  } else {
    PctFull = Contents/Capacity*100.0;
  }
  return overage;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTank::SetContents(double amount)
{
  Contents = amount;
  if (Contents > Capacity) {
    Contents = Capacity;
    PctFull = 100.0;
  } else {
    PctFull = Contents/Capacity*100.0;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::Calculate(double dt)
{
  if (Temperature == -9999.0) return 0.0;
  double HeatCapacity = 900.0;        // Joules/lbm/C
  double TempFlowFactor = 1.115;      // Watts/sqft/C
  double TAT = Auxiliary->GetTAT_C();
  double Tdiff = TAT - Temperature;
  double dTemp = 0.0;                 // Temp change due to one surface
  if (fabs(Tdiff) > 0.1) {
    dTemp = (TempFlowFactor * Area * Tdiff * dt) / (Contents * HeatCapacity);
  }
  return Temperature += (dTemp + dTemp);    // For now, assume upper/lower the same
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

void FGTank::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      " << type << " tank holds " << Capacity << " lbs. " << type << endl;
      cout << "      currently at " << PctFull << "% of maximum capacity" << endl;
      cout << "      Tank location (X, Y, Z): " << vXYZ(eX) << ", " << vXYZ(eY) << ", " << vXYZ(eZ) << endl;
      cout << "      Effective radius: " << Radius << " inches" << endl;
      cout << "      Initial temperature: " << Temperature << " Fahrenheit" << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTank" << endl;
    if (from == 1) cout << "Destroyed:    FGTank" << endl;
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
