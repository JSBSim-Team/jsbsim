/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropeller.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the propeller object

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

#include <iostream>
#include <sstream>

#include "FGPropeller.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGPropeller.cpp,v 1.45 2012/06/10 13:21:19 bcoconni Exp $";
static const char *IdHdr = ID_PROPELLER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// This class currently makes certain assumptions when calculating torque and
// p-factor. That is, that the axis of rotation is the X axis of the aircraft -
// not just the X-axis of the engine/propeller. This may or may not work for a
// helicopter.

FGPropeller::FGPropeller(FGFDMExec* exec, Element* prop_element, int num)
                       : FGThruster(exec, prop_element, num)
{
  string token;
  Element *table_element, *local_element;
  string name="";
  FGPropertyManager* PropertyManager = exec->GetPropertyManager();

  MaxPitch = MinPitch = P_Factor = Pitch = Advance = MinRPM = MaxRPM = 0.0;
  Sense = 1; // default clockwise rotation
  ReversePitch = 0.0;
  Reversed = false;
  Feathered = false;
  Reverse_coef = 0.0;
  GearRatio = 1.0;
  CtFactor = CpFactor = 1.0;
  ConstantSpeed = 0;
  cThrust = cPower = CtMach = CpMach = 0;
  Vinduced = 0.0;

  if (prop_element->FindElement("ixx"))
    Ixx = prop_element->FindElementValueAsNumberConvertTo("ixx", "SLUG*FT2");
  if (prop_element->FindElement("diameter"))
    Diameter = prop_element->FindElementValueAsNumberConvertTo("diameter", "FT");
  if (prop_element->FindElement("numblades"))
    numBlades = (int)prop_element->FindElementValueAsNumber("numblades");
  if (prop_element->FindElement("gearratio"))
    GearRatio = prop_element->FindElementValueAsNumber("gearratio");
  if (prop_element->FindElement("minpitch"))
    MinPitch = prop_element->FindElementValueAsNumber("minpitch");
  if (prop_element->FindElement("maxpitch"))
    MaxPitch = prop_element->FindElementValueAsNumber("maxpitch");
  if (prop_element->FindElement("minrpm"))
    MinRPM = prop_element->FindElementValueAsNumber("minrpm");
  if (prop_element->FindElement("maxrpm")) {
    MaxRPM = prop_element->FindElementValueAsNumber("maxrpm");
    ConstantSpeed = 1;
    }
  if (prop_element->FindElement("constspeed"))
    ConstantSpeed = (int)prop_element->FindElementValueAsNumber("constspeed");
  if (prop_element->FindElement("reversepitch"))
    ReversePitch = prop_element->FindElementValueAsNumber("reversepitch");
  while((table_element = prop_element->FindNextElement("table")) != 0) {
    name = table_element->GetAttributeValue("name");
    try {
      if (name == "C_THRUST") {
        cThrust = new FGTable(PropertyManager, table_element);
      } else if (name == "C_POWER") {
        cPower = new FGTable(PropertyManager, table_element);
      } else if (name == "CT_MACH") {
        CtMach = new FGTable(PropertyManager, table_element);
      } else if (name == "CP_MACH") {
        CpMach = new FGTable(PropertyManager, table_element);
      } else {
        cerr << "Unknown table type: " << name << " in propeller definition." << endl;
      }
    } catch (std::string str) {
      throw("Error loading propeller table:" + name + ". " + str);
    }
  }
  if( (cPower == 0) || (cThrust == 0)){
      cerr << "Propeller configuration must contain C_THRUST and C_POWER tables!" << endl;
  }

  local_element = prop_element->GetParent()->FindElement("sense");
  if (local_element) {
    double Sense = local_element->GetDataAsNumber();
    SetSense(Sense >= 0.0 ? 1.0 : -1.0);
  }
  local_element = prop_element->GetParent()->FindElement("p_factor");
  if (local_element) {
    P_Factor = local_element->GetDataAsNumber();
  }
  if (P_Factor < 0) {
    cerr << "P-Factor value in propeller configuration file must be greater than zero" << endl;
  }
  if (prop_element->FindElement("ct_factor"))
    SetCtFactor( prop_element->FindElementValueAsNumber("ct_factor") );
  if (prop_element->FindElement("cp_factor"))
    SetCpFactor( prop_element->FindElementValueAsNumber("cp_factor") );

  Type = ttPropeller;
  RPM = 0;
  vTorque.InitMatrix();
  D4 = Diameter*Diameter*Diameter*Diameter;
  D5 = D4*Diameter;
  Pitch = MinPitch;

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNum);
  property_name = base_property_name + "/engine-rpm";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetEngineRPM );
  property_name = base_property_name + "/advance-ratio";
  PropertyManager->Tie( property_name.c_str(), &J );
  property_name = base_property_name + "/blade-angle";
  PropertyManager->Tie( property_name.c_str(), &Pitch );
  property_name = base_property_name + "/thrust-coefficient";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetThrustCoefficient );
  property_name = base_property_name + "/propeller-rpm";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetRPM );
  property_name = base_property_name + "/helical-tip-Mach";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetHelicalTipMach );
  property_name = base_property_name + "/constant-speed-mode";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetConstantSpeed,
                      &FGPropeller::SetConstantSpeed );
  property_name = base_property_name + "/prop-induced-velocity_fps";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetInducedVelocity,
                      &FGPropeller::SetInducedVelocity );

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropeller::~FGPropeller()
{
  delete cThrust;
  delete cPower;
  delete CtMach;
  delete CpMach;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// We must be getting the aerodynamic velocity here, NOT the inertial velocity.
// We need the velocity with respect to the wind.
//
// Remembering that Torque * omega = Power, we can derive the torque on the
// propeller and its acceleration to give a new RPM. The current RPM will be
// used to calculate thrust.
//
// Because RPM could be zero, we need to be creative about what RPM is stated as.

double FGPropeller::Calculate(double EnginePower)
{
  FGColumnVector3 localAeroVel = Transform().Transposed() * in.AeroUVW;
  double omega, PowerAvailable;

  double Vel = localAeroVel(eU);
  double rho = in.Density;
  double RPS = RPM/60.0;

  // Calculate helical tip Mach
  double Area = 0.25*Diameter*Diameter*M_PI;
  double Vtip = RPS * Diameter * M_PI;
  HelicalTipMach = sqrt(Vtip*Vtip + Vel*Vel) / in.Soundspeed;

  PowerAvailable = EnginePower - GetPowerRequired();

  if (RPS > 0.0) J = Vel / (Diameter * RPS); // Calculate J normally
  else           J = Vel / Diameter;

  if (MaxPitch == MinPitch) {    // Fixed pitch prop
    ThrustCoeff = cThrust->GetValue(J);
  } else {                       // Variable pitch prop
    ThrustCoeff = cThrust->GetValue(J, Pitch);
  }

  // Apply optional scaling factor to Ct (default value = 1)
  ThrustCoeff *= CtFactor;

  // Apply optional Mach effects from CT_MACH table
  if (CtMach) ThrustCoeff *= CtMach->GetValue(HelicalTipMach);

  Thrust = ThrustCoeff*RPS*RPS*D4*rho;

  // Induced velocity in the propeller disk area. This formula is obtained
  // from momentum theory - see B. W. McCormick, "Aerodynamics, Aeronautics,
  // and Flight Mechanics" 1st edition, eqn. 6.15 (propeller analysis chapter).
  // Since Thrust and Vel can both be negative we need to adjust this formula
  // To handle sign (direction) separately from magnitude.
  double Vel2sum = Vel*abs(Vel) + 2.0*Thrust/(rho*Area);
  
  if( Vel2sum > 0.0)
    Vinduced = 0.5 * (-Vel + sqrt(Vel2sum));
  else
    Vinduced = 0.5 * (-Vel - sqrt(-Vel2sum));

  // We need to drop the case where the downstream velocity is opposite in
  // direction to the aircraft velocity. For example, in such a case, the
  // direction of the airflow on the tail would be opposite to the airflow on
  // the wing tips. When such complicated airflows occur, the momentum theory
  // breaks down and the formulas above are no longer applicable
  // (see H. Glauert, "The Elements of Airfoil and Airscrew Theory",
  // 2nd edition, §16.3, pp. 219-221)

  if ((Vel+2.0*Vinduced)*Vel < 0.0) {
    // The momentum theory is no longer applicable so let's assume the induced
    // saturates to -0.5*Vel so that the total velocity Vel+2*Vinduced equals 0.
    Vinduced = -0.5*Vel;
  }
    
  // P-factor is simulated by a shift of the acting location of the thrust.
  // The shift is a multiple of the angle between the propeller shaft axis
  // and the relative wind that goes through the propeller disk.
  if (P_Factor > 0.0001) {
    double tangentialVel = localAeroVel.Magnitude(eV, eW);

    if (tangentialVel > 0.0001) {
      double angle = atan2(tangentialVel, localAeroVel(eU));
      double factor = Sense * P_Factor * angle / tangentialVel;
      SetActingLocationY( GetLocationY() + factor * localAeroVel(eW));
      SetActingLocationZ( GetLocationZ() + factor * localAeroVel(eV));
    }
  }

  omega = RPS*2.0*M_PI;

  vFn(eX) = Thrust;

  // The Ixx value and rotation speed given below are for rotation about the
  // natural axis of the engine. The transform takes place in the base class
  // FGForce::GetBodyForces() function.

  vH(eX) = Ixx*omega*Sense;
  vH(eY) = 0.0;
  vH(eZ) = 0.0;

  if (omega > 0.0) ExcessTorque = PowerAvailable / omega;
  else             ExcessTorque = PowerAvailable / 1.0;

  RPM = (RPS + ((ExcessTorque / Ixx) / (2.0 * M_PI)) * deltaT) * 60.0;

  if (RPM < 0.0) RPM = 0.0; // Engine won't turn backwards

  // Transform Torque and momentum first, as PQR is used in this
  // equation and cannot be transformed itself.
  vMn = in.PQR*(Transform()*vH) + Transform()*vTorque;

  return Thrust; // return thrust in pounds
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropeller::GetPowerRequired(void)
{
  double cPReq, J;
  double rho = in.Density;
  double Vel = in.AeroUVW(eU);
  double RPS = RPM / 60.0;

  if (RPS != 0.0) J = Vel / (Diameter * RPS);
  else            J = Vel / Diameter;

  if (MaxPitch == MinPitch) {   // Fixed pitch prop
    cPReq = cPower->GetValue(J);

  } else {                      // Variable pitch prop

    if (ConstantSpeed != 0) {   // Constant Speed Mode

      // do normal calculation when propeller is neither feathered nor reversed
      // Note:  This method of feathering and reversing was added to support the
      //        turboprop model.  It's left here for backward compatablity, but
      //        now feathering and reversing should be done in Manual Pitch Mode.
      if (!Feathered) {
        if (!Reversed) {

          double rpmReq = MinRPM + (MaxRPM - MinRPM) * Advance;
          double dRPM = rpmReq - RPM;
          // The pitch of a variable propeller cannot be changed when the RPMs are
          // too low - the oil pump does not work.
          if (RPM > 200) Pitch -= dRPM * deltaT;
          if (Pitch < MinPitch)       Pitch = MinPitch;
          else if (Pitch > MaxPitch)  Pitch = MaxPitch;

        } else { // Reversed propeller

          // when reversed calculate propeller pitch depending on throttle lever position
          // (beta range for taxing full reverse for braking)
          double PitchReq = MinPitch - ( MinPitch - ReversePitch ) * Reverse_coef;
          // The pitch of a variable propeller cannot be changed when the RPMs are
          // too low - the oil pump does not work.
          if (RPM > 200) Pitch += (PitchReq - Pitch) / 200;
          if (RPM > MaxRPM) {
            Pitch += (MaxRPM - RPM) / 50;
            if (Pitch < ReversePitch) Pitch = ReversePitch;
            else if (Pitch > MaxPitch)  Pitch = MaxPitch;
          }
        }

      } else { // Feathered propeller
               // ToDo: Make feathered and reverse settings done via FGKinemat
        Pitch += (MaxPitch - Pitch) / 300; // just a guess (about 5 sec to fully feathered)
      }

    } else { // Manual Pitch Mode, pitch is controlled externally

    }

    cPReq = cPower->GetValue(J, Pitch);
  }

  // Apply optional scaling factor to Cp (default value = 1)
  cPReq *= CpFactor;

  // Apply optional Mach effects from CP_MACH table
  if (CpMach) cPReq *= CpMach->GetValue(HelicalTipMach);

  if (RPS > 0.1) {
    PowerRequired = cPReq*RPS*RPS*RPS*D5*rho;
    vTorque(eX) = -Sense*PowerRequired / (RPS*2.0*M_PI);
  } else {
     // For a stationary prop we have to estimate torque first.
     double CL = (90.0 - Pitch) / 20.0;
     if (CL > 1.5) CL = 1.5;
     double BladeArea = Diameter * Diameter / 32.0 * numBlades;
     vTorque(eX) = -Sense*BladeArea*Diameter*fabs(Vel)*Vel*rho*0.19*CL;
     PowerRequired = Sense*(vTorque(eX))*0.2*M_PI;
  }

  return PowerRequired;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGPropeller::GetPFactor() const
{
  double px=0.0, py, pz;

  py = Thrust * Sense * (GetActingLocationY() - GetLocationY()) / 12.0;
  pz = Thrust * Sense * (GetActingLocationZ() - GetLocationZ()) / 12.0;

  return FGColumnVector3(px, py, pz);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropeller::GetThrusterLabels(int id, const string& delimeter)
{
  std::ostringstream buf;

  buf << Name << " Torque (engine " << id << ")" << delimeter
      << Name << " PFactor Pitch (engine " << id << ")" << delimeter
      << Name << " PFactor Yaw (engine " << id << ")" << delimeter
      << Name << " Thrust (engine " << id << " in lbs)" << delimeter;
  if (IsVPitch())
    buf << Name << " Pitch (engine " << id << ")" << delimeter;
  buf << Name << " RPM (engine " << id << ")";

  return buf.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropeller::GetThrusterValues(int id, const string& delimeter)
{
  std::ostringstream buf;

  FGColumnVector3 vPFactor = GetPFactor();
  buf << vTorque(eX) << delimeter
      << vPFactor(ePitch) << delimeter
      << vPFactor(eYaw) << delimeter
      << Thrust << delimeter;
  if (IsVPitch())
    buf << Pitch << delimeter;
  buf << RPM;

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

void FGPropeller::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "\n    Propeller Name: " << Name << endl;
      cout << "      IXX = " << Ixx << endl;
      cout << "      Diameter = " << Diameter << " ft." << endl;
      cout << "      Number of Blades  = " << numBlades << endl;
      cout << "      Gear Ratio  = " << GearRatio << endl;
      cout << "      Minimum Pitch  = " << MinPitch << endl;
      cout << "      Maximum Pitch  = " << MaxPitch << endl;
      cout << "      Minimum RPM  = " << MinRPM << endl;
      cout << "      Maximum RPM  = " << MaxRPM << endl;
// Tables are being printed elsewhere...
//      cout << "      Thrust Coefficient: " <<  endl;
//      cThrust->Print();
//      cout << "      Power Coefficient: " <<  endl;
//      cPower->Print();
//      cout << "      Mach Thrust Coefficient: " <<  endl;
//      if(CtMach)
//      {
//          CtMach->Print();
//      } else {
//          cout << "        NONE" <<  endl;
//      }
//      cout << "      Mach Power Coefficient: " <<  endl;
//      if(CpMach)
//      {
//          CpMach->Print();
//      } else {
//          cout << "        NONE" <<  endl;
//      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGPropeller" << endl;
    if (from == 1) cout << "Destroyed:    FGPropeller" << endl;
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
