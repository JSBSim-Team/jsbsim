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
#include "FGFCS.h"

static const char *IdSrc = "$Id: FGPropeller.cpp,v 1.37 2001/12/04 13:08:17 jberndt Exp $";
static const char *IdHdr = ID_PROPELLER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGPropeller::FGPropeller(FGFDMExec* exec, FGConfigFile* Prop_cfg) : FGThruster(exec)
{
  string token;
  int rows, cols;

  MaxPitch = MinPitch = P_Factor = Sense = Pitch = 0.0;

  Name = Prop_cfg->GetValue("NAME");
  Prop_cfg->GetNextConfigLine();
  while (Prop_cfg->GetValue() != string("/FG_PROPELLER")) {
    *Prop_cfg >> token;
    if (token == "IXX") {
      *Prop_cfg >> Ixx;
    } else if (token == "DIAMETER") {
      *Prop_cfg >> Diameter;
      Diameter /= 12.0;
    } else if (token == "NUMBLADES") {
      *Prop_cfg >> numBlades;
    } else if (token == "MINPITCH") {
      *Prop_cfg >> MinPitch;
    } else if (token == "MAXPITCH") {
      *Prop_cfg >> MaxPitch;
    } else if (token == "MINRPM") {
      *Prop_cfg >> MinRPM;
    } else if (token == "MAXRPM") {
      *Prop_cfg >> MaxRPM;
    } else if (token == "EFFICIENCY") {
      *Prop_cfg >> rows >> cols;
      if (cols == 1) Efficiency = new FGTable(rows);
      else           Efficiency = new FGTable(rows, cols);
      *Efficiency << *Prop_cfg;
    } else if (token == "C_THRUST") {
      *Prop_cfg >> rows >> cols;
      if (cols == 1) cThrust = new FGTable(rows);
      else           cThrust = new FGTable(rows, cols);
      *cThrust << *Prop_cfg;
    } else if (token == "C_POWER") {
      *Prop_cfg >> rows >> cols;
      if (cols == 1) cPower = new FGTable(rows);
      else           cPower = new FGTable(rows, cols);
      *cPower << *Prop_cfg;
    } else if (token == "EOF") {
      cerr << "      End of file reached" <<  endl;
      break;
    } else {
      cerr << "Unhandled token in Propeller config file: " << token << endl;
    }
  }

  if (debug_lvl > 0) {
    cout << "\n    Propeller Name: " << Name << endl;
    cout << "      IXX = " << Ixx << endl;
    cout << "      Diameter = " << Diameter << " ft." << endl;
    cout << "      Number of Blades  = " << numBlades << endl;
    cout << "      Minimum Pitch  = " << MinPitch << endl;
    cout << "      Maximum Pitch  = " << MaxPitch << endl;
    cout << "      Efficiency: " <<  endl;
    Efficiency->Print();
    cout << "      Thrust Coefficient: " <<  endl;
    cThrust->Print();
    cout << "      Power Coefficient: " <<  endl;
    cPower->Print();
  }

  Type = ttPropeller;
  RPM = 0;

  if (debug_lvl & 2) cout << "Instantiated: FGPropeller" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropeller::~FGPropeller()
{
  if (Efficiency) delete Efficiency;
  if (cThrust)    delete cThrust;
  if (cPower)     delete cPower;
  if (debug_lvl & 2) cout << "Destroyed:    FGPropeller" << endl;
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
  double J, C_Thrust, omega;
  double Vel = fdmex->GetTranslation()->GetvAeroUVW(eU);
  double rho = fdmex->GetAtmosphere()->GetDensity();
  double RPS = RPM/60.0;
  double alpha, beta;

  if (RPM > 0.10) {
    J = Vel / (Diameter * RPS);
  } else {
    J = 0.0;
  }

  if (MaxPitch == MinPitch) { // Fixed pitch prop
    C_Thrust = cThrust->GetValue(J);
  } else {                    // Variable pitch prop
    C_Thrust = cThrust->GetValue(J, Pitch);
  }

  if (P_Factor > 0.0001) {
    alpha = fdmex->GetTranslation()->Getalpha();
    beta  = fdmex->GetTranslation()->Getbeta();
    SetLocationY( GetLocationY() + P_Factor*alpha*fabs(Sense)/Sense);
    SetLocationZ( GetLocationZ() + P_Factor*beta*fabs(Sense)/Sense);
  } else if (P_Factor < 0.000) {
    cerr << "P-Factor value in config file must be greater than zero" << endl;
  }

  Thrust = C_Thrust*RPS*RPS*Diameter*Diameter*Diameter*Diameter*rho;
  vFn(1) = Thrust;
  omega = RPS*2.0*M_PI;

  // The Ixx value and rotation speed given below are for rotation about the
  // natural axis of the engine. The transform takes place in the base class
  // FGForce::GetBodyForces() function.

  vH(eX) = Ixx*omega*fabs(Sense)/Sense;
  vH(eY) = 0.0;
  vH(eZ) = 0.0;

  if (omega <= 5) omega = 1.0;

  Torque = PowerAvailable / omega;
  RPM = (RPS + ((Torque / Ixx) / (2.0 * M_PI)) * deltaT) * 60.0;

  vMn = fdmex->GetRotation()->GetPQR()*vH + Torque*Sense;

  return Thrust; // return thrust in pounds
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGPropeller::GetPowerRequired(void)
{
  if (RPM <= 0.10) return 0.0; // If the prop ain't turnin', the fuel ain't burnin'.

  double cPReq, RPS = RPM / 60.0;

  double J = fdmex->GetTranslation()->GetvAeroUVW(eU) / (Diameter * RPS);
  double rho = fdmex->GetAtmosphere()->GetDensity();

  if (MaxPitch == MinPitch) { // Fixed pitch prop
    cPReq = cPower->GetValue(J);
  } else {                    // Variable pitch prop
    double advance = fdmex->GetFCS()->GetPropAdvance(ThrusterNumber);

    if (MaxRPM != MinRPM) {   // fixed-speed prop
      double rpmReq = MinRPM + (MaxRPM - MinRPM) * advance;
      double dRPM = rpmReq - RPM;

      Pitch -= dRPM / 10;

      if (Pitch < MinPitch)       Pitch = MinPitch;
      else if (Pitch > MaxPitch)  Pitch = MaxPitch;

    } else {
      Pitch = MaxPitch - (MaxPitch - MinPitch) * advance;
    }
    cPReq = cPower->GetValue(J, Pitch);
  }

  PowerRequired = cPReq*RPS*RPS*RPS*Diameter*Diameter*Diameter*Diameter
                                                       *Diameter*rho;
  return PowerRequired;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropeller::Debug(void)
{
    //TODO: Add your source code here
}

