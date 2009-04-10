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

#include "FGTank.h"
#include <models/FGAuxiliary.h>

using std::cerr;
using std::endl;
using std::cout;

namespace JSBSim {

static const char *IdSrc = "$Id: FGTank.cpp,v 1.18 2009/04/10 11:40:36 jberndt Exp $";
static const char *IdHdr = ID_TANK;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTank::FGTank(FGFDMExec* exec, Element* el, int tank_number)
                  : TankNumber(tank_number), Exec(exec)
{
  string token;
  Element* element;
  Element* element_Grain;
  Area = 1.0;
  Temperature = -9999.0;
  Ixx = Iyy = Izz = 0.0;
  Radius = Capacity = Contents = Standpipe = Length = InnerRadius = 0.0;
  PropertyManager = Exec->GetPropertyManager();
  vXYZ.InitMatrix();
  vXYZ_drain.InitMatrix();

  type = el->GetAttributeValue("type");
  if      (type == "FUEL")     Type = ttFUEL;
  else if (type == "OXIDIZER") Type = ttOXIDIZER;
  else                         Type = ttUNKNOWN;

  element = el->FindElement("location");
  if (element)  vXYZ = element->FindElementTripletConvertTo("IN");
  else          cerr << "No location found for this tank." << endl;

  vXYZ_drain = vXYZ; // Set initial drain location to initial tank CG

  element = el->FindElement("drain_location");
  if (element)  {
    vXYZ_drain = element->FindElementTripletConvertTo("IN");
  }

  if (el->FindElement("radius"))
    Radius = el->FindElementValueAsNumberConvertTo("radius", "IN");
  if (el->FindElement("capacity"))
    Capacity = el->FindElementValueAsNumberConvertTo("capacity", "LBS");
  if (el->FindElement("contents"))
    InitialContents = Contents = el->FindElementValueAsNumberConvertTo("contents", "LBS");
  if (el->FindElement("temperature"))
    InitialTemperature = Temperature = el->FindElementValueAsNumber("temperature");
  if (el->FindElement("standpipe"))
    InitialStandpipe = Standpipe = el->FindElementValueAsNumberConvertTo("standpipe", "LBS");

  Selected = true;

  if (Capacity != 0) {
    PctFull = 100.0*Contents/Capacity;            // percent full; 0 to 100.0
  } else {
    Contents = 0;
    PctFull  = 0;
  }

  // Check whether this is a solid propellant "tank". Initialize it if true.

  grainType = gtUNKNOWN; // This is the default
  
  element_Grain = el->FindElement("grain_config");
  if (element_Grain) {

    strGType = element_Grain->GetAttributeValue("type");
    if (strGType == "CYLINDRICAL")     grainType = gtCYLINDRICAL;
    else if (strGType == "ENDBURNING") grainType = gtENDBURNING;
    else                               cerr << "Unknown propellant grain type specified" << endl;

    if (element_Grain->FindElement("length"))
      Length = element_Grain->FindElementValueAsNumberConvertTo("length", "IN");
    if (element_Grain->FindElement("bore_diameter"))
      InnerRadius = element_Grain->FindElementValueAsNumberConvertTo("bore_diameter", "IN")/2.0;

    // Initialize solid propellant values for debug and runtime use.

    switch (grainType) {
      case gtCYLINDRICAL:
        if (Radius <= InnerRadius) {
          cerr << "The bore diameter should be smaller than the total grain diameter!" << endl;
          exit(-1);
        }
        Volume = M_PI * Length * (Radius*Radius - InnerRadius*InnerRadius); // cubic inches
        break;
      case gtENDBURNING:
        Volume = M_PI * Length * Radius * Radius; // cubic inches
        break;
      case gtUNKNOWN:
        cerr << "Unknown grain type found in this rocket engine definition." << endl;
        exit(-1);
    }
    Density = (Contents*lbtoslug)/Volume; // slugs/in^3
    CalculateInertias();
  }

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/tank", TankNumber);
  property_name = base_property_name + "/contents-lbs";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetContents,
                                       &FGTank::SetContents );

  if (Temperature != -9999.0)  InitialTemperature = Temperature = FahrenheitToCelsius(Temperature);
  Area = 40.0 * pow(Capacity/1975, 0.666666667);

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTank::~FGTank()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTank::ResetToIC(void)
{
  Temperature = InitialTemperature;
  Standpipe = InitialStandpipe;
  Contents = InitialContents;
  PctFull = 100.0*Contents/Capacity;
  Selected = true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3 FGTank::GetXYZ(void)
{
  return vXYZ_drain + (Contents/Capacity)*(vXYZ - vXYZ_drain);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const double FGTank::GetXYZ(int idx)
{
  return vXYZ_drain(idx) + (Contents/Capacity)*(vXYZ(idx)-vXYZ_drain(idx));
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

  if (grainType != gtUNKNOWN) CalculateInertias();

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
  double TAT = Exec->GetAuxiliary()->GetTAT_C();
  double Tdiff = TAT - Temperature;
  double dTemp = 0.0;                 // Temp change due to one surface
  if (fabs(Tdiff) > 0.1) {
    dTemp = (TempFlowFactor * Area * Tdiff * dt) / (Contents * HeatCapacity);
  }
  return Temperature += (dTemp + dTemp);    // For now, assume upper/lower the same
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  This function calculates the moments of inertia for a solid propellant
//  grain - either an end burning cylindrical grain or a bored cylindrical
//  grain.

void FGTank::CalculateInertias(void)
{
  double Mass = Contents*lbtoslug;
  double RadSumSqr;
  double Rad2 = Radius*Radius;

  if (Density > 0.0) {
    Volume = (Contents*lbtoslug)/Density; // in^3
  } else {
    cerr << endl << "  Solid propellant grain density is zero!" << endl << endl;
    exit(-1);
  }

  switch (grainType) {
    case gtCYLINDRICAL:
      InnerRadius = sqrt(Rad2 - Volume/(M_PI * Length));
      RadSumSqr = (Rad2 + InnerRadius*InnerRadius)/144.0;
      Ixx = 0.5*Mass*RadSumSqr;
      Iyy = Mass*(3.0*RadSumSqr + Length*Length/144.0)/12.0;
      break;
    case gtENDBURNING:
      Length = Volume/(M_PI*Rad2);
      Ixx = 0.5*Mass*Rad2/144.0;
      Iyy = Mass*(3.0*Rad2 + Length*Length)/(144.0*12.0);
      break;
  }
  Izz  = Iyy;

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
