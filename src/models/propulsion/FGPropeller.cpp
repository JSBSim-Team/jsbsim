/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropeller.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the propeller object

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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

#include <sstream>

#include "FGPropeller.h"
#include <models/FGPropagate.h>
#include <models/FGAtmosphere.h>
#include <models/FGAuxiliary.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGPropeller.cpp,v 1.20 2009/02/05 10:22:49 jberndt Exp $";
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
  if (prop_element->FindElement("maxrpm"))
    MaxRPM = prop_element->FindElementValueAsNumber("maxrpm");
  if (prop_element->FindElement("reversepitch"))
    ReversePitch = prop_element->FindElementValueAsNumber("reversepitch");
  for (int i=0; i<2; i++) {
    table_element = prop_element->FindNextElement("table");
    name = table_element->GetAttributeValue("name");
    if (name == "C_THRUST") {
      cThrust = new FGTable(PropertyManager, table_element);
    } else if (name == "C_POWER") {
      cPower = new FGTable(PropertyManager, table_element);
    } else {
      cerr << "Unknown table type: " << name << " in propeller definition." << endl;
    }
  }

  local_element = prop_element->GetParent()->FindElement("sense");
  if (local_element) {
    double Sense = local_element->GetDataAsNumber();
    SetSense(fabs(Sense)/Sense);
  }
  local_element = prop_element->GetParent()->FindElement("p_factor");
  if (local_element) {
    P_Factor = local_element->GetDataAsNumber();
  }
  if (P_Factor < 0) {
    cerr << "P-Factor value in config file must be greater than zero" << endl;
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

  string property_name, base_property_name;
  base_property_name = CreateIndexedPropertyName("propulsion/engine", EngineNum);
  property_name = base_property_name + "/advance-ratio";
  PropertyManager->Tie( property_name.c_str(), &J );
  property_name = base_property_name + "/blade-angle";
  PropertyManager->Tie( property_name.c_str(), &Pitch );
  property_name = base_property_name + "/thrust-coefficient";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetThrustCoefficient );
  property_name = base_property_name + "/propeller-rpm";
  PropertyManager->Tie( property_name.c_str(), this, &FGPropeller::GetRPM );

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropeller::~FGPropeller()
{
  delete cThrust;
  delete cPower;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// We must be getting the aerodynamic velocity here, NOT the inertial velocity.
// We need the velocity with respect to the wind.
//
// Note that PowerAvailable is the excess power available after the drag of the
// propeller has been subtracted. At equilibrium, PowerAvailable will be zero -
// indicating that the propeller will not accelerate or decelerate.
// Remembering that Torque * omega = Power, we can derive the torque on the
// propeller and its acceleration to give a new RPM. The current RPM will be
// used to calculate thrust.
//
// Because RPM could be zero, we need to be creative about what RPM is stated as.

double FGPropeller::Calculate(double PowerAvailable)
{
  double omega, alpha, beta;

  double Vel = fdmex->GetAuxiliary()->GetAeroUVW(eU);
  double rho = fdmex->GetAtmosphere()->GetDensity();
  double RPS = RPM/60.0;

  if (RPS > 0.00) J = Vel / (Diameter * RPS); // Calculate J normally
  else            J = 1000.0;                 // Set J to a high number

  if (MaxPitch == MinPitch)  ThrustCoeff = cThrust->GetValue(J);
  else                       ThrustCoeff = cThrust->GetValue(J, Pitch);
  ThrustCoeff *= CtFactor;

  if (P_Factor > 0.0001) {
    alpha = fdmex->GetAuxiliary()->Getalpha();
    beta  = fdmex->GetAuxiliary()->Getbeta();
    SetActingLocationY( GetLocationY() + P_Factor*alpha*Sense);
    SetActingLocationZ( GetLocationZ() + P_Factor*beta*Sense);
  }

  Thrust = ThrustCoeff*RPS*RPS*D4*rho;
  omega = RPS*2.0*M_PI;

  vFn(1) = Thrust;

  // The Ixx value and rotation speed given below are for rotation about the
  // natural axis of the engine. The transform takes place in the base class
  // FGForce::GetBodyForces() function.

  vH(eX) = Ixx*omega*Sense;
  vH(eY) = 0.0;
  vH(eZ) = 0.0;

  if (omega > 0.0) ExcessTorque = GearRatio * PowerAvailable / omega;
  else             ExcessTorque = GearRatio * PowerAvailable / 1.0;

  RPM = (RPS + ((ExcessTorque / Ixx) / (2.0 * M_PI)) * deltaT) * 60.0;

  if (RPM < 1.0) RPM = 0; // Engine friction stops rotation arbitrarily at 1 RPM.

  vMn = fdmex->GetPropagate()->GetPQR()*vH + vTorque;

  return Thrust; // return thrust in pounds
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropeller::GetPowerRequired(void)
{
  double cPReq, J;
  double rho = fdmex->GetAtmosphere()->GetDensity();
  double RPS = RPM / 60.0;

  if (RPS != 0) J = fdmex->GetAuxiliary()->GetAeroUVW(eU) / (Diameter * RPS);
  else          J = 1000.0; // Set J to a high number

  if (MaxPitch == MinPitch) { // Fixed pitch prop
    Pitch = MinPitch;
    cPReq = cPower->GetValue(J);
  } else {                      // Variable pitch prop

    if (MaxRPM != MinRPM) {   // fixed-speed prop

      // do normal calculation when propeller is neither feathered nor reversed
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

    } else { // Variable Speed Prop
      Pitch = MinPitch + (MaxPitch - MinPitch) * Advance;
    }
    cPReq = cPower->GetValue(J, Pitch);
  }
  cPReq *= CpFactor;

  if (RPS > 0) {
    PowerRequired = cPReq*RPS*RPS*RPS*D5*rho;
    vTorque(eX) = -Sense*PowerRequired / (RPS*2.0*M_PI);
  } else {
    PowerRequired = 0.0;
    vTorque(eX) = 0.0;
  }

  return PowerRequired;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGPropeller::GetPFactor()
{
  double px=0.0, py, pz;

  py = Thrust * Sense * (GetActingLocationY() - GetLocationY()) / 12.0;
  pz = Thrust * Sense * (GetActingLocationZ() - GetLocationZ()) / 12.0;

  return FGColumnVector3(px, py, pz);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGPropeller::GetThrusterLabels(int id, string delimeter)
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

string FGPropeller::GetThrusterValues(int id, string delimeter)
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
//      cout << "      Thrust Coefficient: " <<  endl;
//      cThrust->Print();
//      cout << "      Power Coefficient: " <<  endl;
//      cPower->Print();
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
