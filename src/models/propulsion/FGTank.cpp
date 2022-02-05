/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTank.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft

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
See header file.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGTank.h"
#include "FGFDMExec.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTank::FGTank(FGFDMExec* exec, Element* el, int tank_number)
  : TankNumber(tank_number)
{
  string token, strFuelName;
  Element* element;
  Element* element_Grain;
  auto PropertyManager = exec->GetPropertyManager();
  Area = 1.0;
  Density = 6.6;
  InitialTemperature = Temperature = -9999.0;
  Ixx = Iyy = Izz = 0.0;
  InertiaFactor = 1.0;
  Radius = Contents = Standpipe = Length = InnerRadius = 0.0;
  ExternalFlow = 0.0;
  InitialStandpipe = 0.0;
  Capacity = 0.00001; UnusableVol = 0.0;
  Priority = InitialPriority = 1;
  vXYZ.InitMatrix();
  vXYZ_drain.InitMatrix();
  ixx_unit = iyy_unit = izz_unit = 1.0;
  grainType = gtUNKNOWN; // This is the default

  string type = el->GetAttributeValue("type");
  if      (type == "FUEL")     Type = ttFUEL;
  else if (type == "OXIDIZER") Type = ttOXIDIZER;
  else                         Type = ttUNKNOWN;

  Name = el->GetAttributeValue("name");

  element = el->FindElement("location");
  if (element)  vXYZ = element->FindElementTripletConvertTo("IN");
  else          cerr << el->ReadFrom() << "No location found for this tank."
                     << endl;

  vXYZ_drain = vXYZ; // Set initial drain location to initial tank CG

  element = el->FindElement("drain_location");
  if (element)  {
    vXYZ_drain = element->FindElementTripletConvertTo("IN");
  }

  if (el->FindElement("radius"))
    Radius = el->FindElementValueAsNumberConvertTo("radius", "IN");
  if (el->FindElement("inertia_factor"))
    InertiaFactor = el->FindElementValueAsNumber("inertia_factor");
  if (el->FindElement("capacity"))
    Capacity = el->FindElementValueAsNumberConvertTo("capacity", "LBS");
  if (el->FindElement("contents"))
    InitialContents = Contents = el->FindElementValueAsNumberConvertTo("contents", "LBS");
  if (el->FindElement("unusable-volume"))
    UnusableVol = el->FindElementValueAsNumberConvertTo("unusable-volume", "GAL");
  if (el->FindElement("temperature"))
    InitialTemperature = Temperature = el->FindElementValueAsNumber("temperature");
  if (el->FindElement("standpipe"))
    InitialStandpipe = Standpipe = el->FindElementValueAsNumberConvertTo("standpipe", "LBS");
  if (el->FindElement("priority"))
    InitialPriority = Priority = (int)el->FindElementValueAsNumber("priority");
  if (el->FindElement("density"))
    Density = el->FindElementValueAsNumberConvertTo("density", "LBS/GAL");
  if (el->FindElement("type"))
    strFuelName = el->FindElementValue("type");


  SetPriority( InitialPriority );     // this will also set the Selected flag

  if (Capacity == 0) {
    cerr << el->ReadFrom()
         << "Tank capacity must not be zero. Reset to 0.00001 lbs!" << endl;
    Capacity = 0.00001;
    Contents = 0.0;
  }
  if (Capacity <= GetUnusable()) {
    cerr << el->ReadFrom() << "Tank capacity (" << Capacity
         << " lbs) is lower than the amount of unusable fuel (" << GetUnusable()
         << " lbs) for tank " << tank_number
         << "! Did you accidentally swap unusable and capacity?" << endl;
    throw("tank definition error");
  }
  if (Contents > Capacity) {
    cerr << el->ReadFrom() << "Tank content (" << Contents
         << " lbs) is greater than tank capacity (" << Capacity
         << " lbs) for tank " << tank_number
         << "! Did you accidentally swap contents and capacity?" << endl;
    throw("tank definition error");
  }
  if (Contents < GetUnusable()) {
    cerr << el->ReadFrom() << "Tank content (" << Contents
         << " lbs) is lower than the amount of unusable fuel (" << GetUnusable()
         << " lbs) for tank " << tank_number << endl;
  }

  PctFull = 100.0*Contents/Capacity;            // percent full; 0 to 100.0

  // Check whether this is a solid propellant "tank". Initialize it if true.

  element_Grain = el->FindElement("grain_config");
  if (element_Grain) {

    string strGType = element_Grain->GetAttributeValue("type");
    if (strGType == "CYLINDRICAL")     grainType = gtCYLINDRICAL;
    else if (strGType == "ENDBURNING") grainType = gtENDBURNING;
    else if (strGType == "FUNCTION")   {
      grainType = gtFUNCTION;
      if (element_Grain->FindElement("ixx") != 0) {
        Element* element_ixx = element_Grain->FindElement("ixx");
        if (element_ixx->GetAttributeValue("unit") == "KG*M2") ixx_unit = 1.0/1.35594;
        if (element_ixx->FindElement("function") != 0) {
          function_ixx = new FGFunction(exec, element_ixx->FindElement("function"));
        }
      } else {
        throw("For tank "+to_string(TankNumber)+" and when grain_config is specified an ixx must be specified when the FUNCTION grain type is specified.");
      }

      if (element_Grain->FindElement("iyy")) {
        Element* element_iyy = element_Grain->FindElement("iyy");
        if (element_iyy->GetAttributeValue("unit") == "KG*M2") iyy_unit = 1.0/1.35594;
        if (element_iyy->FindElement("function") != 0) {
          function_iyy = new FGFunction(exec, element_iyy->FindElement("function"));
        }
      } else {
        throw("For tank "+to_string(TankNumber)+" and when grain_config is specified an iyy must be specified when the FUNCTION grain type is specified.");
      }

      if (element_Grain->FindElement("izz")) {
        Element* element_izz = element_Grain->FindElement("izz");
        if (element_izz->GetAttributeValue("unit") == "KG*M2") izz_unit = 1.0/1.35594;
        if (element_izz->FindElement("function") != 0) {
          function_izz = new FGFunction(exec, element_izz->FindElement("function"));
        }
      } else {
        throw("For tank "+to_string(TankNumber)+" and when grain_config is specified an izz must be specified when the FUNCTION grain type is specified.");
      }
    }
    else
      cerr << el->ReadFrom() << "Unknown propellant grain type specified"
           << endl;

    if (element_Grain->FindElement("length"))
      Length = element_Grain->FindElementValueAsNumberConvertTo("length", "IN");
    if (element_Grain->FindElement("bore_diameter"))
      InnerRadius = element_Grain->FindElementValueAsNumberConvertTo("bore_diameter", "IN")/2.0;

    // Initialize solid propellant values for debug and runtime use.

    switch (grainType) {
      case gtCYLINDRICAL:
        if (Radius <= InnerRadius) {
          const string s("The bore diameter should be smaller than the total grain diameter!");
          cerr << element_Grain->ReadFrom() << endl << s << endl;
          throw BaseException(s);
        }
        Volume = M_PI * Length * (Radius*Radius - InnerRadius*InnerRadius); // cubic inches
        break;
      case gtENDBURNING:
        Volume = M_PI * Length * Radius * Radius; // cubic inches
        break;
      case gtFUNCTION:
        Volume = 1;  // Volume is irrelevant for the FUNCTION type, but it can't be zero!
        break;
      case gtUNKNOWN:
        {
          const string s("Unknown grain type found in this rocket engine definition.");
          cerr << el->ReadFrom() << endl << s << endl;
          throw BaseException(s);
        }
    }
    Density = (Capacity*lbtoslug)/Volume; // slugs/in^3
  }

  CalculateInertias();

  if (Temperature != -9999.0)  InitialTemperature = Temperature = FahrenheitToCelsius(Temperature);
  Area = 40.0 * pow(Capacity/1975, 0.666666667);

  // A named fuel type will override a previous density value
  if (!strFuelName.empty()) Density = ProcessFuelName(strFuelName);

  bind(PropertyManager.get());

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
  SetTemperature( InitialTemperature );
  SetStandpipe ( InitialStandpipe );
  SetContents ( InitialContents );
  PctFull = 100.0*Contents/Capacity;
  SetPriority( InitialPriority );
  CalculateInertias();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGTank::GetXYZ(void) const
{
  return vXYZ_drain + (Contents/Capacity)*(vXYZ - vXYZ_drain);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::GetXYZ(int idx) const
{
  return vXYZ_drain(idx) + (Contents/Capacity)*(vXYZ(idx)-vXYZ_drain(idx));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::Drain(double used)
{
  double remaining = Contents - used;

  if (remaining >= GetUnusable()) { // Reduce contents by amount used.
    Contents -= used;
  } else { // This tank must be empty.
    if (Contents > GetUnusable())
      Contents = GetUnusable();

    remaining = Contents;
  }

  PctFull = 100.0*Contents/Capacity;
  CalculateInertias();

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

  CalculateInertias();

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

  CalculateInertias();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTank::SetContentsGallons(double gallons)
{
  SetContents(gallons * Density);
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::Calculate(double dt, double TAT_C)
{
  if(ExternalFlow < 0.) Drain( -ExternalFlow *dt);
  else Fill(ExternalFlow * dt);

  if (Temperature == -9999.0) return 0.0;
  double HeatCapacity = 900.0;        // Joules/lbm/C
  double TempFlowFactor = 1.115;      // Watts/sqft/C
  double Tdiff = TAT_C - Temperature;
  double dTemp = 0.0;                 // Temp change due to one surface
  if (fabs(Tdiff) > 0.1 && Contents > 0.01) {
    dTemp = (TempFlowFactor * Area * Tdiff * dt) / (Contents * HeatCapacity);
  }

  return Temperature += (dTemp + dTemp);    // For now, assume upper/lower the same
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  This function calculates the moments of inertia for a solid propellant
//  grain - either an end burning cylindrical grain or a bored cylindrical
//  grain, as well as liquid propellants IF a tank radius and inertia factor
//  are given.
//
//  From NASA CR-383, the MoI of a tank with liquid propellant is specified
//  for baffled and non-baffled tanks as a ratio compared to that in which the
//  propellant is solid. The more baffles, the more "rigid" the propellant and
//  the higher the ratio (up to 1.0). For a cube tank with five baffles, the
//  ratio ranges from 0.5 to 0.7. For a cube tank with no baffles, the ratio is
//  roughly 0.18. One might estimate that for a spherical tank with no baffles
//  the ratio might be somewhere around 0.10 to 0.15. Cylindrical tanks with or
//  without baffles might have biased moment of inertia effects based on the
//  baffle layout and tank geometry. A vector inertia_factor may be supported
//  at some point.

void FGTank::CalculateInertias(void)
{
  double Mass = Contents*lbtoslug;
  double RadSumSqr;
  double Rad2 = Radius*Radius;

  if (grainType != gtUNKNOWN) { // assume solid propellant

    if (Density > 0.0) {
      Volume = (Contents*lbtoslug)/Density; // in^3
    } else if (Contents <= 0.0) {
      Volume = 0;
    } else {
      const string s("  Solid propellant grain density is zero!");
      cerr << endl << s << endl;
      throw BaseException(s);
    }

    switch (grainType) {
    case gtCYLINDRICAL:
      InnerRadius = sqrt(Rad2 - Volume/(M_PI * Length));
      RadSumSqr = (Rad2 + InnerRadius*InnerRadius)/144.0;
      Ixx = 0.5*Mass*RadSumSqr;
      Iyy = Mass*(3.0*RadSumSqr + Length*Length/144.0)/12.0;
      Izz  = Iyy;
      break;
    case gtENDBURNING:
      Length = Volume/(M_PI*Rad2);
      Ixx = 0.5*Mass*Rad2/144.0;
      Iyy = Mass*(3.0*Rad2 + Length*Length)/(144.0*12.0);
      Izz  = Iyy;
      break;
    case gtFUNCTION:
      Ixx = function_ixx->GetValue()*ixx_unit;
      Iyy = function_iyy->GetValue()*iyy_unit;
      Izz = function_izz->GetValue()*izz_unit;
      break;
    default:
      {
        const string s("Unknown grain type found.");
        cerr << s << endl;
        throw BaseException(s);
      }
    }

  } else { // assume liquid propellant: shrinking snowball

    if (Radius > 0.0) Ixx = Iyy = Izz = Mass * InertiaFactor * 0.4 * Radius * Radius / 144.0;

  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::ProcessFuelName(const std::string& name)
{
   if      (name == "AVGAS")    return 6.02;
   else if (name == "JET-A")    return 6.74;
   else if (name == "JET-A1")   return 6.74;
   else if (name == "JET-B")    return 6.48;
   else if (name == "JP-1")     return 6.76;
   else if (name == "JP-2")     return 6.38;
   else if (name == "JP-3")     return 6.34;
   else if (name == "JP-4")     return 6.48;
   else if (name == "JP-5")     return 6.81;
   else if (name == "JP-6")     return 6.55;
   else if (name == "JP-7")     return 6.61;
   else if (name == "JP-8")     return 6.66;
   else if (name == "JP-8+100") return 6.66;
 //else if (name == "JP-9")     return 6.74;
 //else if (name == "JPTS")     return 6.74;
   else if (name == "RP-1")     return 6.73;
   else if (name == "T-1")      return 6.88;
   else if (name == "ETHANOL")  return 6.58;
   else if (name == "HYDRAZINE")return 8.61;
   else if (name == "F-34")     return 6.66;
   else if (name == "F-35")     return 6.74;
   else if (name == "F-40")     return 6.48;
   else if (name == "F-44")     return 6.81;
   else if (name == "AVTAG")    return 6.48;
   else if (name == "AVCAT")    return 6.81;
   else {
     cerr << "Unknown fuel type specified: "<< name << endl;
   }

   return 6.6;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTank::bind(FGPropertyManager* PropertyManager)
{
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/tank", TankNumber);
  property_name = base_property_name + "/contents-lbs";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetContents,
                                       &FGTank::SetContents );
  property_name = base_property_name + "/unusable-volume-gal";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetUnusableVolume,
                        &FGTank::SetUnusableVolume );
  property_name = base_property_name + "/pct-full";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetPctFull);
  property_name = base_property_name + "/density-lbs_per_gal";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetDensity);

  property_name = base_property_name + "/priority";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetPriority,
                                       &FGTank::SetPriority );
  property_name = base_property_name + "/external-flow-rate-pps";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetExternalFlow,
                                       &FGTank::SetExternalFlow );
  property_name = base_property_name + "/local-ixx-slug_ft2";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetIxx);
  property_name = base_property_name + "/local-iyy-slug_ft2";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetIyy);
  property_name = base_property_name + "/local-izz-slug_ft2";
  PropertyManager->Tie( property_name.c_str(), (FGTank*)this, &FGTank::GetIzz);

  property_name = base_property_name + "/x-position";
  PropertyManager->Tie(property_name.c_str(), (FGTank*)this, &FGTank::GetLocationX, &FGTank::SetLocationX);
  property_name = base_property_name + "/y-position";
  PropertyManager->Tie(property_name.c_str(), (FGTank*)this, &FGTank::GetLocationY, &FGTank::SetLocationY);
  property_name = base_property_name + "/z-position";
  PropertyManager->Tie(property_name.c_str(), (FGTank*)this, &FGTank::GetLocationZ, &FGTank::SetLocationZ);

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
      string type;
      switch (Type) {
      case ttFUEL:
        type = "FUEL";
        break;
      case ttOXIDIZER:
        type = "OXIDIZER";
        break;
      default:
        type = "UNKNOWN";
        break;
      }

      cout << "      " << Name << " (" << type << ") tank holds " << Capacity << " lbs. " << type << endl;
      cout << "      currently at " << PctFull << "% of maximum capacity" << endl;
      cout << "      Tank location (X, Y, Z): " << vXYZ(eX) << ", " << vXYZ(eY) << ", " << vXYZ(eZ) << endl;
      cout << "      Effective radius: " << Radius << " inches" << endl;
      cout << "      Initial temperature: " << Temperature << " Fahrenheit" << endl;
      cout << "      Priority: " << Priority << endl;
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
    }
  }
}
}
