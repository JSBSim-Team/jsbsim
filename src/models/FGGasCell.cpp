/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGasCell.h
 Author:       Anders Gidenstam
 Date started: 01/21/2006

 ----- Copyright (C) 2006 - 2011  Anders Gidenstam (anders(at)gidenstam.org) --

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
01/21/2006  AG   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "models/FGMassBalance.h"
#include "FGGasCell.h"
#include "input_output/FGXMLElement.h"
#include <iostream>
#include <cstdlib>

using std::cerr;
using std::endl;
using std::cout;
using std::string;
using std::max;

namespace JSBSim {

static const char *IdSrc = "$Id: FGGasCell.cpp,v 1.16 2011/11/22 20:21:49 andgi Exp $";
static const char *IdHdr = ID_GASCELL;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/* Constants. */
const double FGGasCell::R = 3.4071;              // [lbs ft/(mol Rankine)]
const double FGGasCell::M_air = 0.0019186;       // [slug/mol]
const double FGGasCell::M_hydrogen = 0.00013841; // [slug/mol]
const double FGGasCell::M_helium = 0.00027409;   // [slug/mol]

FGGasCell::FGGasCell(FGFDMExec* exec, Element* el, int num, const struct Inputs& input)
  : FGForce(exec), in(input)
{
  string token;
  Element* element;

  PropertyManager = exec->GetPropertyManager();
  MassBalance = exec->GetMassBalance();

  gasCellJ = FGMatrix33();
  gasCellM = FGColumnVector3();

  Buoyancy = MaxVolume = MaxOverpressure = Temperature = Pressure =
    Contents = Volume = dVolumeIdeal = 0.0;
  Xradius = Yradius = Zradius = Xwidth = Ywidth = Zwidth = 0.0;
  ValveCoefficient = ValveOpen = 0.0;
  CellNum = num;

  // NOTE: In the local system X points north, Y points east and Z points down.
  SetTransformType(FGForce::tLocalBody);

  type = el->GetAttributeValue("type");
  if      (type == "HYDROGEN") Type = ttHYDROGEN;
  else if (type == "HELIUM")   Type = ttHELIUM;
  else if (type == "AIR")      Type = ttAIR;
  else                         Type = ttUNKNOWN;

  element = el->FindElement("location");
  if (element) {
    vXYZ = element->FindElementTripletConvertTo("IN");
  } else {
    cerr << "Fatal Error: No location found for this gas cell." << endl;
    exit(-1);
  }
  if ((el->FindElement("x_radius") || el->FindElement("x_width")) &&
      (el->FindElement("y_radius") || el->FindElement("y_width")) &&
      (el->FindElement("z_radius") || el->FindElement("z_width"))) {

    if (el->FindElement("x_radius")) {
      Xradius = el->FindElementValueAsNumberConvertTo("x_radius", "FT");
    }
    if (el->FindElement("y_radius")) {
      Yradius = el->FindElementValueAsNumberConvertTo("y_radius", "FT");
    }
    if (el->FindElement("z_radius")) {
      Zradius = el->FindElementValueAsNumberConvertTo("z_radius", "FT");
    }

    if (el->FindElement("x_width")) {
      Xwidth = el->FindElementValueAsNumberConvertTo("x_width", "FT");
    }
    if (el->FindElement("y_width")) {
      Ywidth = el->FindElementValueAsNumberConvertTo("y_width", "FT");
    }
    if (el->FindElement("z_width")) {
      Zwidth = el->FindElementValueAsNumberConvertTo("z_width", "FT");
    }

    // The volume is a (potentially) extruded ellipsoid.
    // However, currently only a few combinations of radius and width are
    // fully supported.
    if ((Xradius != 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
        (Xwidth  == 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
      // Ellipsoid volume.
      MaxVolume = 4.0  * M_PI * Xradius * Yradius * Zradius / 3.0;
    } else if  ((Xradius == 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
                (Xwidth  != 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
      // Cylindrical volume.
      MaxVolume = M_PI * Yradius * Zradius * Xwidth;
    } else {
      cerr << "Warning: Unsupported gas cell shape." << endl;
      MaxVolume = 
        (4.0  * M_PI * Xradius * Yradius * Zradius / 3.0 +
         M_PI * Yradius * Zradius * Xwidth +
         M_PI * Xradius * Zradius * Ywidth +
         M_PI * Xradius * Yradius * Zwidth +
         2.0  * Xradius * Ywidth * Zwidth +
         2.0  * Yradius * Xwidth * Zwidth +
         2.0  * Zradius * Xwidth * Ywidth +
         Xwidth * Ywidth * Zwidth);
    }
  } else {
    cerr << "Fatal Error: Gas cell shape must be given." << endl;
    exit(-1);
  }
  if (el->FindElement("max_overpressure")) {
    MaxOverpressure = el->FindElementValueAsNumberConvertTo("max_overpressure",
                                                            "LBS/FT2");
  }
  if (el->FindElement("fullness")) {
    const double Fullness = el->FindElementValueAsNumber("fullness");
    if (0 <= Fullness) { 
      Volume = Fullness * MaxVolume; 
    } else {
      cerr << "Warning: Invalid initial gas cell fullness value." << endl;
    }
  }  
  if (el->FindElement("valve_coefficient")) {
    ValveCoefficient =
      el->FindElementValueAsNumberConvertTo("valve_coefficient",
                                            "FT4*SEC/SLUG");
    ValveCoefficient = max(ValveCoefficient, 0.0);
  }

  // Initialize state
  SetLocation(vXYZ);

  if (Temperature == 0.0) {
    Temperature = in.Temperature;
  }
  if (Pressure == 0.0) {
    Pressure = in.Pressure;
  }
  if (Volume != 0.0) {
    // Calculate initial gas content.
    Contents = Pressure * Volume / (R * Temperature);
    
    // Clip to max allowed value.
    const double IdealPressure = Contents * R * Temperature / MaxVolume;
    if (IdealPressure > Pressure + MaxOverpressure) {
      Contents = (Pressure + MaxOverpressure) * MaxVolume / (R * Temperature);
      Pressure = Pressure + MaxOverpressure;
    } else {
      Pressure = max(IdealPressure, Pressure);
    }
  } else {
    // Calculate initial gas content.
    Contents = Pressure * MaxVolume / (R * Temperature);
  }

  Volume = Contents * R * Temperature / Pressure;
  Mass = Contents * M_gas();

  // Bind relevant properties
  string property_name, base_property_name;

  base_property_name = CreateIndexedPropertyName("buoyant_forces/gas-cell", CellNum);

  property_name = base_property_name + "/max_volume-ft3";
  PropertyManager->Tie( property_name.c_str(), &MaxVolume, false );
  PropertyManager->SetWritable( property_name, false );
  property_name = base_property_name + "/temp-R";
  PropertyManager->Tie( property_name.c_str(), &Temperature, false );
  property_name = base_property_name + "/pressure-psf";
  PropertyManager->Tie( property_name.c_str(), &Pressure, false );
  property_name = base_property_name + "/volume-ft3";
  PropertyManager->Tie( property_name.c_str(), &Volume, false );
  property_name = base_property_name + "/buoyancy-lbs";
  PropertyManager->Tie( property_name.c_str(), &Buoyancy, false );
  property_name = base_property_name + "/contents-mol";
  PropertyManager->Tie( property_name.c_str(), &Contents, false );
  property_name = base_property_name + "/valve_open";
  PropertyManager->Tie( property_name.c_str(), &ValveOpen, false );

  Debug(0);

  // Read heat transfer coefficients
  if (Element* heat = el->FindElement("heat")) {
    Element* function_element = heat->FindElement("function");
    while (function_element) {
      HeatTransferCoeff.push_back(new FGFunction(PropertyManager,
                                                 function_element));
      function_element = heat->FindNextElement("function");
    }
  }

  // Load ballonets if there are any
  if (Element* ballonet_element = el->FindElement("ballonet")) {
    while (ballonet_element) {
      Ballonet.push_back(new FGBallonet(exec,
                                        ballonet_element,
                                        Ballonet.size(),
                                        this, in));
      ballonet_element = el->FindNextElement("ballonet");
    }
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGasCell::~FGGasCell()
{
  unsigned int i;

  for (i = 0; i < HeatTransferCoeff.size(); i++) delete HeatTransferCoeff[i];
  HeatTransferCoeff.clear();

  for (i = 0; i < Ballonet.size(); i++) delete Ballonet[i];
  Ballonet.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGasCell::Calculate(double dt)
{
  const double AirTemperature = in.Temperature;  // [Rankine]
  const double AirPressure    = in.Pressure;     // [lbs/ft^2]
  const double AirDensity     = in.Density;      // [slug/ft^3]
  const double g = in.gravity;                   // [lbs/slug]

  const double OldTemperature = Temperature;
  const double OldPressure    = Pressure;
  unsigned int i;
  const unsigned int no_ballonets = Ballonet.size();

  //-- Read ballonet state --
  // NOTE: This model might need a more proper integration technique. 
  double BallonetsVolume = 0.0;
  double BallonetsHeatFlow = 0.0;
  for (i = 0; i < no_ballonets; i++) {
    BallonetsVolume   += Ballonet[i]->GetVolume();
    BallonetsHeatFlow += Ballonet[i]->GetHeatFlow();
  }

  //-- Gas temperature --

  if (HeatTransferCoeff.size() > 0) {
    // The model is based on the ideal gas law.
    // However, it does look a bit fishy. Please verify.
    //   dT/dt = dU / (Cv n R)
    double dU = 0.0;
    for (i = 0; i < HeatTransferCoeff.size(); i++) {
      dU += HeatTransferCoeff[i]->GetValue();
    }
    // Don't include dt when accounting for adiabatic expansion/contraction.
    // The rate of adiabatic cooling looks about right: ~5.4 Rankine/1000ft. 
    if (Contents > 0) {
      Temperature +=
        (dU * dt - Pressure * dVolumeIdeal - BallonetsHeatFlow) /
        (Cv_gas() * Contents * R);
    } else {
      Temperature = AirTemperature;
    }
  } else {
    // No simulation of complex temperature changes.
    // Note: Making the gas cell behave adiabatically might be a better
    // option.
    Temperature = AirTemperature;
  }

  //-- Pressure --
  const double IdealPressure =
    Contents * R * Temperature / (MaxVolume - BallonetsVolume);
  if (IdealPressure > AirPressure + MaxOverpressure) {
    Pressure = AirPressure + MaxOverpressure;
  } else {
    Pressure = max(IdealPressure, AirPressure);
  }

  //-- Manual valving --

  // FIXME: Presently the effect of manual valving is computed using
  //        an ad hoc formula which might not be a good representation
  //        of reality.
  if ((ValveCoefficient > 0.0) && (ValveOpen > 0.0)) {
    // First compute the difference in pressure between the gas in the
    // cell and the air above it.
    // FixMe: CellHeight should depend on current volume.
    const double CellHeight = 2 * Zradius + Zwidth;                   // [ft]
    const double GasMass    = Contents * M_gas();                     // [slug]
    const double GasVolume  = Contents * R * Temperature / Pressure;  // [ft³]
    const double GasDensity = GasMass / GasVolume;
    const double DeltaPressure =
      Pressure + CellHeight * g * (AirDensity - GasDensity) - AirPressure;
    const double VolumeValved =
      ValveOpen * ValveCoefficient * DeltaPressure * dt;
    Contents =
      max(0.0, Contents - Pressure * VolumeValved / (R * Temperature));
  }

  //-- Update ballonets. --
  // Doing that here should give them the opportunity to react to the
  // new pressure.
  BallonetsVolume = 0.0;
  for (i = 0; i < no_ballonets; i++) {
    Ballonet[i]->Calculate(dt);
    BallonetsVolume += Ballonet[i]->GetVolume();
  }

  //-- Automatic safety valving. --
  if (Contents * R * Temperature / (MaxVolume - BallonetsVolume) >
      AirPressure + MaxOverpressure) {
    // Gas is automatically valved. Valving capacity is assumed to be infinite.
    // FIXME: This could/should be replaced by damage to the gas cell envelope.
    Contents =
      (AirPressure + MaxOverpressure) *
      (MaxVolume - BallonetsVolume) / (R * Temperature);
  }

  //-- Volume --
  Volume = Contents * R * Temperature / Pressure + BallonetsVolume;
  dVolumeIdeal =
    Contents * R * (Temperature / Pressure - OldTemperature / OldPressure);

  //-- Current buoyancy --
  // The buoyancy is computed using the atmospheres local density.
  Buoyancy = Volume * AirDensity * g;
  
  // Note: This is gross buoyancy. The weight of the gas itself and
  // any ballonets is not deducted here as the effects of the gas mass
  // is handled by FGMassBalance.
  vFn.InitMatrix(0.0, 0.0, - Buoyancy);

  // Compute the inertia of the gas cell.
  // Consider the gas cell as a shape of uniform density.
  // FIXME: If the cell isn't ellipsoid or cylindrical the inertia will
  //        be wrong.
  gasCellJ = FGMatrix33();
  const double mass = Contents * M_gas();
  double Ixx, Iyy, Izz;
  if ((Xradius != 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
      (Xwidth  == 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
    // Ellipsoid volume.
    Ixx = (1.0 / 5.0) * mass * (Yradius*Yradius + Zradius*Zradius);
    Iyy = (1.0 / 5.0) * mass * (Xradius*Xradius + Zradius*Zradius);
    Izz = (1.0 / 5.0) * mass * (Xradius*Xradius + Yradius*Yradius);     
  } else if  ((Xradius == 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
              (Xwidth  != 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
    // Cylindrical volume (might not be valid with an elliptical cross-section).
    Ixx = (1.0 / 2.0) * mass * Yradius * Zradius;
    Iyy =
      (1.0 / 4.0) * mass * Yradius * Zradius +
      (1.0 / 12.0) * mass * Xwidth * Xwidth;
    Izz =
      (1.0 / 4.0) * mass * Yradius * Zradius +
      (1.0 / 12.0) * mass * Xwidth * Xwidth;
  } else {
    // Not supported. Revert to pointmass model.
    Ixx = Iyy = Izz = 0.0;
  }
  // The volume is symmetric, so Ixy = Ixz = Iyz = 0.
  gasCellJ(1,1) = Ixx;
  gasCellJ(2,2) = Iyy;
  gasCellJ(3,3) = Izz;
  Mass = mass;
  // Transform the moments of inertia to the body frame.
  gasCellJ += MassBalance->GetPointmassInertia(Mass, GetXYZ());

  gasCellM.InitMatrix();
  gasCellM(eX) +=
    GetXYZ(eX) * Mass*slugtolb;
  gasCellM(eY) +=
    GetXYZ(eY) * Mass*slugtolb;
  gasCellM(eZ) +=
    GetXYZ(eZ) * Mass*slugtolb;

  if (no_ballonets > 0) {
    // Add the mass, moment and inertia of any ballonets.
    for (i = 0; i < no_ballonets; i++) {
      Mass += Ballonet[i]->GetMass();
       
      // Add ballonet moments due to mass (in the structural frame).
      gasCellM(eX) +=
        Ballonet[i]->GetXYZ(eX) * Ballonet[i]->GetMass()*slugtolb;
      gasCellM(eY) +=
        Ballonet[i]->GetXYZ(eY) * Ballonet[i]->GetMass()*slugtolb;
      gasCellM(eZ) +=
        Ballonet[i]->GetXYZ(eZ) * Ballonet[i]->GetMass()*slugtolb;
      
      gasCellJ += Ballonet[i]->GetInertia();
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

void FGGasCell::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "    Gas cell holds " << Contents << " mol " <<
        type << endl;
      cout << "      Cell location (X, Y, Z) (in.): " << vXYZ(eX) << ", " <<
        vXYZ(eY) << ", " << vXYZ(eZ) << endl;
      cout << "      Maximum volume: " << MaxVolume << " ft3" << endl;
      cout << "      Relief valve release pressure: " << MaxOverpressure << 
        " lbs/ft2" << endl;
      cout << "      Manual valve coefficient: " << ValveCoefficient << 
        " ft4*sec/slug" << endl;
      cout << "      Initial temperature: " << Temperature << " Rankine" <<
        endl;
      cout << "      Initial pressure: " << Pressure << " lbs/ft2" << endl;
      cout << "      Initial volume: " << Volume << " ft3" << endl;
      cout << "      Initial mass: " << GetMass() << " slug mass" << endl;
      cout << "      Initial weight: " << GetMass()*lbtoslug << " lbs force" <<
        endl;
      cout << "      Heat transfer: " << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGGasCell" << endl;
    if (from == 1) cout << "Destroyed:    FGGasCell" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables    
      cout << "      " << type << " cell holds " << Contents << " mol " << endl;
      cout << "      Temperature: " << Temperature << " Rankine" << endl;
      cout << "      Pressure: " << Pressure << " lbs/ft2" << endl;
      cout << "      Volume: " << Volume << " ft3" << endl;
      cout << "      Mass: " << GetMass() << " slug mass" << endl;
      cout << "      Weight: " << GetMass()*lbtoslug << " lbs force" << endl;
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

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
const double FGBallonet::R = 3.4071;              // [lbs ft/(mol Rankine)]
const double FGBallonet::M_air = 0.0019186;       // [slug/mol]
const double FGBallonet::Cv_air = 5.0/2.0;        // [??]

FGBallonet::FGBallonet(FGFDMExec* exec, Element* el, int num, FGGasCell* parent, const struct FGGasCell::Inputs& input)
  : in(input)
{
  string token;
  Element* element;

  PropertyManager = exec->GetPropertyManager();
  MassBalance = exec->GetMassBalance();

  ballonetJ = FGMatrix33();

  MaxVolume = MaxOverpressure = Temperature = Pressure =
    Contents = Volume = dVolumeIdeal = dU = 0.0;
  Xradius = Yradius = Zradius = Xwidth = Ywidth = Zwidth = 0.0;
  ValveCoefficient = ValveOpen = 0.0;
  BlowerInput = NULL;
  CellNum = num;
  Parent = parent;

  // NOTE: In the local system X points north, Y points east and Z points down.
  element = el->FindElement("location");
  if (element) {
    vXYZ = element->FindElementTripletConvertTo("IN");
  } else {
    cerr << "Fatal Error: No location found for this ballonet." << endl;
    exit(-1);
  }
  if ((el->FindElement("x_radius") || el->FindElement("x_width")) &&
      (el->FindElement("y_radius") || el->FindElement("y_width")) &&
      (el->FindElement("z_radius") || el->FindElement("z_width"))) {

    if (el->FindElement("x_radius")) {
      Xradius = el->FindElementValueAsNumberConvertTo("x_radius", "FT");
    }
    if (el->FindElement("y_radius")) {
      Yradius = el->FindElementValueAsNumberConvertTo("y_radius", "FT");
    }
    if (el->FindElement("z_radius")) {
      Zradius = el->FindElementValueAsNumberConvertTo("z_radius", "FT");
    }

    if (el->FindElement("x_width")) {
      Xwidth = el->FindElementValueAsNumberConvertTo("x_width", "FT");
    }
    if (el->FindElement("y_width")) {
      Ywidth = el->FindElementValueAsNumberConvertTo("y_width", "FT");
    }
    if (el->FindElement("z_width")) {
      Zwidth = el->FindElementValueAsNumberConvertTo("z_width", "FT");
    }

    // The volume is a (potentially) extruded ellipsoid.
    // FIXME: However, currently only a few combinations of radius and
    //        width are fully supported.
    if ((Xradius != 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
        (Xwidth  == 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
      // Ellipsoid volume.
      MaxVolume = 4.0  * M_PI * Xradius * Yradius * Zradius / 3.0;
    } else if  ((Xradius == 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
                (Xwidth  != 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
      // Cylindrical volume.
      MaxVolume = M_PI * Yradius * Zradius * Xwidth;
    } else {
      cerr << "Warning: Unsupported ballonet shape." << endl;
      MaxVolume = 
        (4.0  * M_PI * Xradius * Yradius * Zradius / 3.0 +
         M_PI * Yradius * Zradius * Xwidth +
         M_PI * Xradius * Zradius * Ywidth +
         M_PI * Xradius * Yradius * Zwidth +
         2.0  * Xradius * Ywidth * Zwidth +
         2.0  * Yradius * Xwidth * Zwidth +
         2.0  * Zradius * Xwidth * Ywidth +
         Xwidth * Ywidth * Zwidth);
    }
  } else {
    cerr << "Fatal Error: Ballonet shape must be given." << endl;
    exit(-1);
  }
  if (el->FindElement("max_overpressure")) {
    MaxOverpressure = el->FindElementValueAsNumberConvertTo("max_overpressure",
                                                            "LBS/FT2");
  }
  if (el->FindElement("fullness")) {
    const double Fullness = el->FindElementValueAsNumber("fullness");
    if (0 <= Fullness) { 
      Volume = Fullness * MaxVolume; 
    } else {
      cerr << "Warning: Invalid initial ballonet fullness value." << endl;
    }
  }  
  if (el->FindElement("valve_coefficient")) {
    ValveCoefficient =
      el->FindElementValueAsNumberConvertTo("valve_coefficient",
                                            "FT4*SEC/SLUG");
    ValveCoefficient = max(ValveCoefficient, 0.0);
  }

  // Initialize state
  if (Temperature == 0.0) {
    Temperature = Parent->GetTemperature();
  }
  if (Pressure == 0.0) {
    Pressure = Parent->GetPressure();
  }
  if (Volume != 0.0) {
    // Calculate initial air content.
    Contents = Pressure * Volume / (R * Temperature);
    
    // Clip to max allowed value.
    const double IdealPressure = Contents * R * Temperature / MaxVolume;
    if (IdealPressure > Pressure + MaxOverpressure) {
      Contents = (Pressure + MaxOverpressure) * MaxVolume / (R * Temperature);
      Pressure = Pressure + MaxOverpressure;
    } else {
      Pressure = max(IdealPressure, Pressure);
    }
  } else {
    // Calculate initial air content.
    Contents = Pressure * MaxVolume / (R * Temperature);
  }

  Volume = Contents * R * Temperature / Pressure;

  // Bind relevant properties
  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("buoyant_forces/gas-cell", Parent->GetIndex());
  base_property_name = CreateIndexedPropertyName(base_property_name + "/ballonet", CellNum);

  property_name = base_property_name + "/max_volume-ft3";
  PropertyManager->Tie( property_name, &MaxVolume, false );
  PropertyManager->SetWritable( property_name, false );

  property_name = base_property_name + "/temp-R";
  PropertyManager->Tie( property_name, &Temperature, false );

  property_name = base_property_name + "/pressure-psf";
  PropertyManager->Tie( property_name, &Pressure, false );

  property_name = base_property_name + "/volume-ft3";
  PropertyManager->Tie( property_name, &Volume, false );

  property_name = base_property_name + "/contents-mol";
  PropertyManager->Tie( property_name, &Contents, false );

  property_name = base_property_name + "/valve_open";
  PropertyManager->Tie( property_name, &ValveOpen, false );

  Debug(0);

  // Read heat transfer coefficients
  if (Element* heat = el->FindElement("heat")) {
    Element* function_element = heat->FindElement("function");
    while (function_element) {
      HeatTransferCoeff.push_back(new FGFunction(PropertyManager,
                                                 function_element));
      function_element = heat->FindNextElement("function");
    }
  }
  // Read blower input function
  if (Element* blower = el->FindElement("blower_input")) {
    Element* function_element = blower->FindElement("function");
    BlowerInput = new FGFunction(PropertyManager,
                                 function_element);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGBallonet::~FGBallonet()
{
  unsigned int i;

  for (i = 0; i < HeatTransferCoeff.size(); i++) delete HeatTransferCoeff[i];
  HeatTransferCoeff.clear();

  delete BlowerInput;
  BlowerInput = NULL;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGBallonet::Calculate(double dt)
{
  const double ParentPressure = Parent->GetPressure(); // [lbs/ft^2]
  const double AirPressure    = in.Pressure;           // [lbs/ft^2]

  const double OldTemperature = Temperature;
  const double OldPressure    = Pressure;
  unsigned int i;

  //-- Gas temperature --

  // The model is based on the ideal gas law.
  // However, it does look a bit fishy. Please verify.
  //   dT/dt = dU / (Cv n R)
  dU = 0.0;
  for (i = 0; i < HeatTransferCoeff.size(); i++) {
    dU += HeatTransferCoeff[i]->GetValue();
  }
  // dt is already accounted for in dVolumeIdeal.
  if (Contents > 0) {
    Temperature +=
      (dU * dt - Pressure * dVolumeIdeal) / (Cv_air * Contents * R);
  } else {
    Temperature = Parent->GetTemperature();
  }

  //-- Pressure --
  const double IdealPressure = Contents * R * Temperature / MaxVolume;
  // The pressure is at least that of the parent gas cell.
  Pressure = max(IdealPressure, ParentPressure);

  //-- Blower input --
  if (BlowerInput) {
    const double AddedVolume = BlowerInput->GetValue() * dt;
    if (AddedVolume > 0.0) {
      Contents += Pressure * AddedVolume / (R * Temperature);
    }
  }

  //-- Pressure relief and manual valving --
  // FIXME: Presently the effect of valving is computed using
  //        an ad hoc formula which might not be a good representation
  //        of reality.
  if ((ValveCoefficient > 0.0) &&
      ((ValveOpen > 0.0) || (Pressure > AirPressure + MaxOverpressure))) {
    const double DeltaPressure = Pressure - AirPressure;
    const double VolumeValved =
      ((Pressure > AirPressure + MaxOverpressure) ? 1.0 : ValveOpen) *
      ValveCoefficient * DeltaPressure * dt;
    // FIXME: Too small values of Contents sometimes leads to NaN.
    //        Currently the minimum is restricted to a safe value.
    Contents =
      max(1.0, Contents - Pressure * VolumeValved / (R * Temperature));
  }

  //-- Volume --
  Volume = Contents * R * Temperature / Pressure;
  dVolumeIdeal =
    Contents * R * (Temperature / Pressure - OldTemperature / OldPressure);

  // Compute the inertia of the ballonet.
  // Consider the ballonet as a shape of uniform density.
  // FIXME: If the ballonet isn't ellipsoid or cylindrical the inertia will
  //        be wrong.
  ballonetJ = FGMatrix33();
  const double mass = Contents * M_air;
  double Ixx, Iyy, Izz;
  if ((Xradius != 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
      (Xwidth  == 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
    // Ellipsoid volume.
    Ixx = (1.0 / 5.0) * mass * (Yradius*Yradius + Zradius*Zradius);
    Iyy = (1.0 / 5.0) * mass * (Xradius*Xradius + Zradius*Zradius);
    Izz = (1.0 / 5.0) * mass * (Xradius*Xradius + Yradius*Yradius);     
  } else if  ((Xradius == 0.0) && (Yradius != 0.0) && (Zradius != 0.0) &&
              (Xwidth  != 0.0) && (Ywidth  == 0.0) && (Zwidth  == 0.0)) {
    // Cylindrical volume (might not be valid with an elliptical cross-section).
    Ixx = (1.0 / 2.0) * mass * Yradius * Zradius;
    Iyy =
      (1.0 / 4.0) * mass * Yradius * Zradius +
      (1.0 / 12.0) * mass * Xwidth * Xwidth;
    Izz =
      (1.0 / 4.0) * mass * Yradius * Zradius +
      (1.0 / 12.0) * mass * Xwidth * Xwidth;
  } else {
    // Not supported. Revert to pointmass model.
    Ixx = Iyy = Izz = 0.0;
  }
  // The volume is symmetric, so Ixy = Ixz = Iyz = 0.
  ballonetJ(1,1) = Ixx;
  ballonetJ(2,2) = Iyy;
  ballonetJ(3,3) = Izz;
  // Transform the moments of inertia to the body frame.
  ballonetJ += MassBalance->GetPointmassInertia(GetMass(), GetXYZ());
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

void FGBallonet::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      Ballonet holds " << Contents << " mol air" << endl;
      cout << "        Location (X, Y, Z) (in.): " << vXYZ(eX) << ", " <<
        vXYZ(eY) << ", " << vXYZ(eZ) << endl;
      cout << "        Maximum volume: " << MaxVolume << " ft3" << endl;
      cout << "        Relief valve release pressure: " << MaxOverpressure << 
        " lbs/ft2" << endl;
      cout << "        Relief valve coefficient: " << ValveCoefficient << 
        " ft4*sec/slug" << endl;
      cout << "        Initial temperature: " << Temperature << " Rankine" <<
        endl;
      cout << "        Initial pressure: " << Pressure << " lbs/ft2" << endl;
      cout << "        Initial volume: " << Volume << " ft3" << endl;
      cout << "        Initial mass: " << GetMass() << " slug mass" << endl;
      cout << "        Initial weight: " << GetMass()*lbtoslug <<
        " lbs force" << endl;
      cout << "        Heat transfer: " << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGBallonet" << endl;
    if (from == 1) cout << "Destroyed:    FGBallonet" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables    
      cout << "        Ballonet holds " << Contents <<
        " mol air" << endl;
      cout << "        Temperature: " << Temperature << " Rankine" << endl;
      cout << "        Pressure: " << Pressure << " lbs/ft2" << endl;
      cout << "        Volume: " << Volume << " ft3" << endl;
      cout << "        Mass: " << GetMass() << " slug mass" << endl;
      cout << "        Weight: " << GetMass()*lbtoslug << " lbs force" << endl;
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
