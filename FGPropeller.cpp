/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropeller.cpp
 Author:       Jon S. Berndt
 Date started: 08/24/00
 Purpose:      Encapsulates the propeller object

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
08/24/00  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPropeller.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPropeller.cpp,v 1.9 2001/01/23 12:28:21 jsb Exp $";
static const char *IdHdr = ID_PROPELLER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGPropeller::FGPropeller(FGFDMExec* exec, FGConfigFile* Prop_cfg) : FGThruster(exec)
{
  string token;
  int rows, cols;

  PropName = Prop_cfg->GetValue("NAME");
  cout << "\n    Propeller Name: " << PropName << endl;
  Prop_cfg->GetNextConfigLine();
  while (Prop_cfg->GetValue() != "/PROPELLER") {
    *Prop_cfg >> token;
    if (token == "IXX") {
      *Prop_cfg >> Ixx;
      cout << "      IXX = " << Ixx << endl;
    } else if (token == "DIAMETER") {
      *Prop_cfg >> Diameter;
      cout << "      Diameter = " << Diameter << endl;
    } else if (token == "NUMBLADES") {
      *Prop_cfg >> numBlades;
      cout << "      Number of Blades  = " << numBlades << endl;
    } else if (token == "EFFICIENCY") {
       *Prop_cfg >> rows >> cols;
       if (cols == 1) Efficiency = new FGTable(rows);
	     else           Efficiency = new FGTable(rows, cols);
       *Efficiency << *Prop_cfg;
       cout << "      Efficiency: " <<  endl;
       Efficiency->Print();
    } else if (token == "C_THRUST") {
       *Prop_cfg >> rows >> cols;
       if (cols == 1) cThrust = new FGTable(rows);
	     else           cThrust = new FGTable(rows, cols);
       *cThrust << *Prop_cfg;
       cout << "      Thrust Coefficient: " <<  endl;
       cThrust->Print();
    } else if (token == "C_POWER") {
       *Prop_cfg >> rows >> cols;
       if (cols == 1) cPower = new FGTable(rows);
	     else           cPower = new FGTable(rows, cols);
       *cPower << *Prop_cfg;
       cout << "      Power Coefficient: " <<  endl;
       cPower->Print();
    } else {
      cout << "Unhandled token in Propeller config file: " << token << endl;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropeller::~FGPropeller(void)
{
  if (Efficiency) delete Efficiency;
  if (cThrust)    delete cThrust;
  if (cPower)     delete cPower;
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

float FGPropeller::Calculate(float PowerAvailable)
{
  float J, C_Thrust, omega;
  float Vel = (fdmex->GetTranslation()->GetUVW())(1);
  float rho = fdmex->GetAtmosphere()->GetDensity();

  if (RPM > 0.10) {
    J = Vel / (Diameter * RPM / 60.0);
  } else {
    J = 0.0;
  }

  if (MaxPitch == MinPitch) { // Fixed pitch prop
    C_Thrust = cThrust->GetValue(J);
  } else {                    // Variable pitch prop
    C_Thrust = cThrust->GetValue(J, Pitch);
  }

  Thrust = C_Thrust*RPM*RPM*Diameter*Diameter*Diameter*Diameter*rho/(3600.0);

  omega = (RPM/60.0)*2.0*M_PI;

  if (omega <= 500) omega = 500.0;

  Torque = PowerAvailable / omega;

  RPM += ((Torque / Ixx) * 60.0 / (2.0 * M_PI)) * deltaT;

  return Thrust; // return thrust in pounds
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGPropeller::GetPowerRequired(void)
{
  if (RPM <= 0.10) return 0.0; // If the prop ain't turnin', the fuel ain't burnin'.

  float J = (fdmex->GetTranslation()->GetUVW())(1) / (Diameter * RPM / 60.0);
  float rho = fdmex->GetAtmosphere()->GetDensity();
  float cPReq;

  if (MaxPitch == MinPitch) { // Fixed pitch prop
    cPReq = cPower->GetValue(J);
  } else {                    // Variable pitch prop
    cPReq = cPower->GetValue(J, Pitch);
  }

  PowerRequired = cPReq*RPM*RPM*RPM*Diameter*Diameter*Diameter*Diameter
                                                       *Diameter*rho/(216000.0);

  return PowerRequired;
}

