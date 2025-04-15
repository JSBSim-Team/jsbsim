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

#include "FGFDMExec.h"
#include "FGPropeller.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGPropeller::FGPropeller(FGFDMExec* exec, Element* prop_element, int num)
                       : FGThruster(exec, prop_element, num)
{
  Element *table_element, *local_element;
  string name="";
  auto PropertyManager = exec->GetPropertyManager();

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
    Ixx = max(prop_element->FindElementValueAsNumberConvertTo("ixx", "SLUG*FT2"), 1e-06);

  Sense_multiplier = 1.0;
  if (prop_element->HasAttribute("version")
      && prop_element->GetAttributeValueAsNumber("version") > 1.0)
      Sense_multiplier = -1.0;

  if (prop_element->FindElement("diameter"))
    Diameter = max(prop_element->FindElementValueAsNumberConvertTo("diameter", "FT"), 0.001);
  if (prop_element->FindElement("numblades"))
    numBlades = (int)prop_element->FindElementValueAsNumber("numblades");
  if (prop_element->FindElement("gearratio"))
    GearRatio = max(prop_element->FindElementValueAsNumber("gearratio"), 0.001);
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
        FGXMLLogging log(fdmex->GetLogger(), table_element, LogLevel::ERROR);
        log << "Unknown table type: " << name << " in propeller definition.\n";
      }
    } catch (BaseException& e) {
      XMLLogException err(fdmex->GetLogger(), table_element);
      err << "Error loading propeller table:" << name << ". " << e.what() << "\n";
      throw err;
    }
  }
  if( (cPower == nullptr) || (cThrust == nullptr)){
    XMLLogException err(fdmex->GetLogger(), prop_element);
    err << "Propeller configuration must contain C_THRUST and C_POWER tables!\n";
    throw err;
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
    XMLLogException err(fdmex->GetLogger(), local_element);
    err << "P-Factor value in propeller configuration file must be greater than zero\n";
    throw err;
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
  property_name = base_property_name + "/prop-induced-velocity_fps"; // [ft/sec]
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetInducedVelocity,
                      &FGPropeller::SetInducedVelocity );
  property_name = base_property_name + "/propeller-power-ftlbps"; // [ft-lbs/sec]
  PropertyManager->Tie( property_name.c_str(), &PowerRequired );
  property_name = base_property_name + "/propeller-torque-ftlb"; // [ft-lbs]
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetTorque);
  property_name = base_property_name + "/propeller-sense";
  PropertyManager->Tie( property_name.c_str(), &Sense );

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

void FGPropeller::ResetToIC(void)
{
  FGThruster::ResetToIC();
  Vinduced = 0.0;
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
  FGColumnVector3 vDXYZ = MassBalance->StructuralToBody(vXYZn);
  const FGMatrix33& mT = Transform();
  // Local air velocity is obtained from Stevens & Lewis' "Aircraft Control and
  // Simualtion (3rd edition)" eqn 8.2-1
  // Variables in.AeroUVW and in.AeroPQR include the wind and turbulence effects
  // as computed by FGAuxiliary.
  FGColumnVector3 localAeroVel = mT.Transposed() * (in.AeroUVW + in.AeroPQR*vDXYZ);
  double omega, PowerAvailable;

  double Vel = localAeroVel(eU);
  double rho = in.Density;
  double RPS = RPM/60.0;

  // Calculate helical tip Mach
  double Area = 0.25*Diameter*Diameter*M_PI;
  double Vtip = RPS * Diameter * M_PI;
  HelicalTipMach = sqrt(Vtip*Vtip + Vel*Vel) / in.Soundspeed;

  if (RPS > 0.01) J = Vel / (Diameter * RPS); // Calculate J normally
  else           J = Vel / Diameter;

  PowerAvailable = EnginePower - GetPowerRequired();

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

  // P-factor is simulated by a shift of the acting location of the thrust.
  // The shift is a multiple of the angle between the propeller shaft axis
  // and the relative wind that goes through the propeller disk.
  if (P_Factor > 0.0001) {
    double tangentialVel = localAeroVel.Magnitude(eV, eW);

    if (tangentialVel > 0.0001) {
      // The angle made locally by the air flow with respect to the propeller
      // axis is influenced by the induced velocity. This attenuates the
      // influence of a string cross wind and gives a more realistic behavior.
      double angle = atan2(tangentialVel, Vel+Vinduced);
      double factor = Sense * P_Factor * angle / tangentialVel;
      SetActingLocationY( GetLocationY() + factor * localAeroVel(eW));
      SetActingLocationZ( GetLocationZ() + factor * localAeroVel(eV));
    }
  }

  omega = RPS*2.0*M_PI;

  vFn(eX) = Thrust;
  vTorque(eX) = -Sense*EnginePower / max(0.01, omega);

  // The Ixx value and rotation speed given below are for rotation about the
  // natural axis of the engine. The transform takes place in the base class
  // FGForce::GetBodyForces() function.

  FGColumnVector3 vH(Ixx*omega*Sense*Sense_multiplier, 0.0, 0.0);

  if (omega > 0.01) ExcessTorque = PowerAvailable / omega;
  else             ExcessTorque = PowerAvailable / 1.0;

  RPM = (RPS + ((ExcessTorque / Ixx) / (2.0 * M_PI)) * in.TotalDeltaT) * 60.0;

  if (RPM < 0.0) RPM = 0.0; // Engine won't turn backwards

  // Transform Torque and momentum first, as PQR is used in this
  // equation and cannot be transformed itself.
  vMn = in.PQRi*(mT*vH) + mT*vTorque;

  return Thrust; // return thrust in pounds
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropeller::GetPowerRequired(void)
{
  double cPReq;

  if (MaxPitch == MinPitch) {   // Fixed pitch prop
    cPReq = cPower->GetValue(J);

  } else {                      // Variable pitch prop

    if (ConstantSpeed != 0) {   // Constant Speed Mode

      // do normal calculation when propeller is neither feathered nor reversed
      // Note:  This method of feathering and reversing was added to support the
      //        turboprop model.  It's left here for backward compatiblity, but
      //        now feathering and reversing should be done in Manual Pitch Mode.
      if (!Feathered) {
        if (!Reversed) {

          double rpmReq = MinRPM + (MaxRPM - MinRPM) * Advance;
          double dRPM = rpmReq - RPM;
          // The pitch of a variable propeller cannot be changed when the RPMs are
          // too low - the oil pump does not work.
          if (RPM > 200) Pitch -= dRPM * in.TotalDeltaT;
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

  double RPS = RPM / 60.0;
  double local_RPS = RPS < 0.01 ? 0.01 : RPS;

  PowerRequired = cPReq*local_RPS*local_RPS*local_RPS*D5*in.Density;

  return PowerRequired;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGPropeller::GetPFactor() const
{
  // These are moments in lbf per ft : the lever arm along Z generates a moment
  // along the pitch direction.
  double p_pitch = Thrust * Sense * (GetActingLocationZ() - GetLocationZ()) / 12.0;
  // The lever arm along Y generates a moment along the yaw direction.
  double p_yaw = Thrust * Sense * (GetActingLocationY() - GetLocationY()) / 12.0;

  return FGColumnVector3(0.0, p_pitch, p_yaw);
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
      FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
      log << "\n    Propeller Name: " << Name << "\n";
      log << "      IXX = " << Ixx << "\n";
      log << "      Diameter = " << Diameter << " ft." << "\n";
      log << "      Number of Blades  = " << numBlades << "\n";
      log << "      Gear Ratio  = " << GearRatio << "\n";
      log << "      Minimum Pitch  = " << MinPitch << "\n";
      log << "      Maximum Pitch  = " << MaxPitch << "\n";
      log << "      Minimum RPM  = " << MinRPM << "\n";
      log << "      Maximum RPM  = " << MaxRPM << "\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fdmex->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGPropeller\n";
    if (from == 1) log << "Destroyed:    FGPropeller\n";
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
