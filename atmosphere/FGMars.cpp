/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGMars.cpp
 Author:       Jon Berndt
 Date started: 1/4/04
 Purpose:      Models the Martian atmosphere very simply
 Called by:    FGFDMExec

 ------------- Copyright (C) 2004  Jon S. Berndt (jsb@hal-pc.org) -------------

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
Models the Martian atmosphere.

HISTORY
--------------------------------------------------------------------------------
1/04/2004   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGMars.h"
#include "FGState.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGMars.cpp,v 1.3 2004/03/26 04:47:32 jberndt Exp $";
static const char *IdHdr = ID_MARS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMars::FGMars(FGFDMExec* fdmex) : FGAtmosphere(fdmex)
{
  Name = "FGMars";
  Reng = 53.5 * 44.01;

/*
  lastIndex = 0;
  h = 0.0;
  psiw = 0.0;

  MagnitudedAccelDt = MagnitudeAccel = Magnitude = 0.0;
//   turbType = ttNone;
  turbType = ttStandard;
//   turbType = ttBerndt;
  TurbGain = 0.0;
  TurbRate = 1.0;
*/

  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
FGMars::~FGMars()
{
  unbind();
  Debug(1);
}
*/
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMars::InitModel(void)
{
  FGModel::InitModel();

  Calculate(h);
  SLtemperature = intTemperature;
  SLpressure    = intPressure;
  SLdensity     = intDensity;
  SLsoundspeed  = sqrt(SHRatio*Reng*intTemperature);
  rSLtemperature = 1.0/intTemperature;
  rSLpressure    = 1.0/intPressure;
  rSLdensity     = 1.0/intDensity;
  rSLsoundspeed  = 1.0/SLsoundspeed;
  temperature = &intTemperature;
  pressure = &intPressure;
  density = &intDensity;

  useExternal=false;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMars::Run(void)
{
  if (!FGModel::Run()) {  // if false then execute this Run()

    //do temp, pressure, and density first
    if (!useExternal) {
      h = Position->Geth();
      Calculate(h);
    }

    if (turbType != ttNone) {
      Turbulence();
      vWindNED += vTurbulence;
    }

    if (vWindNED(1) != 0.0) psiw = atan2( vWindNED(2), vWindNED(1) );

    if (psiw < 0) psiw += 2*M_PI;

    soundspeed = sqrt(SHRatio*Reng*(*temperature));

    Debug(2);

    return false;
  } else {                               // skip Run() execution this time
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMars::Calculate(double altitude)
{
  //Calculate reftemp, refpress, and density

  // LIMIT the temperatures so they do not descend below absolute zero.

  if (altitude < 22960.0) {
    intTemperature = -25.68 - 0.000548*altitude; // Deg Fahrenheit
  } else {
    intTemperature = -10.34 - 0.001217*altitude; // Deg Fahrenheit
  }
  intPressure = 14.62*exp(-0.00003*altitude); // psf - 14.62 psf =~ 7 millibars
  intDensity = intPressure/(Reng*intTemperature); // slugs/ft^3 (needs deg R. as input

  //cout << "Atmosphere:  h=" << altitude << " rho= " << intDensity << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// square a value, but preserve the original sign

static inline double
square_signed (double value)
{
  if (value < 0)
    return value * value * -1;
  else
    return value * value;
}

void FGMars::Turbulence(void)
{
  switch (turbType) {
  case ttStandard: {
    vDirectiondAccelDt(eX) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eY) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eZ) = 1 - 2.0*(double(rand())/double(RAND_MAX));

    MagnitudedAccelDt = 1 - 2.0*(double(rand())/double(RAND_MAX)) - Magnitude;
                                // Scale the magnitude so that it moves
                                // away from the peaks
    MagnitudedAccelDt = ((MagnitudedAccelDt - Magnitude) /
                         (1 + fabs(Magnitude)));
    MagnitudeAccel    += MagnitudedAccelDt*rate*TurbRate*State->Getdt();
    Magnitude         += MagnitudeAccel*rate*State->Getdt();

    vDirectiondAccelDt.Normalize();

                                // deemphasise non-vertical forces
    vDirectiondAccelDt(eX) = square_signed(vDirectiondAccelDt(eX));
    vDirectiondAccelDt(eY) = square_signed(vDirectiondAccelDt(eY));

    vDirectionAccel += vDirectiondAccelDt*rate*TurbRate*State->Getdt();
    vDirectionAccel.Normalize();
    vDirection      += vDirectionAccel*rate*State->Getdt();

    vDirection.Normalize();

                                // Diminish turbulence within three wingspans
                                // of the ground
    vTurbulence = TurbGain * Magnitude * vDirection;
    double HOverBMAC = Position->GetHOverBMAC();
    if (HOverBMAC < 3.0)
        vTurbulence *= (HOverBMAC / 3.0) * (HOverBMAC / 3.0);

    vTurbulenceGrad = TurbGain*MagnitudeAccel * vDirection;

    vBodyTurbGrad = Rotation->GetTl2b()*vTurbulenceGrad;
    vTurbPQR(eP) = vBodyTurbGrad(eY)/Aircraft->GetWingSpan();
//     if (Aircraft->GetHTailArm() != 0.0)
//       vTurbPQR(eQ) = vBodyTurbGrad(eZ)/Aircraft->GetHTailArm();
//     else
//       vTurbPQR(eQ) = vBodyTurbGrad(eZ)/10.0;

    if (Aircraft->GetVTailArm())
      vTurbPQR(eR) = vBodyTurbGrad(eX)/Aircraft->GetVTailArm();
    else
      vTurbPQR(eR) = vBodyTurbGrad(eX)/10.0;

                                // Clear the horizontal forces
                                // actually felt by the plane, now
                                // that we've used them to calculate
                                // moments.
    vTurbulence(eX) = 0.0;
    vTurbulence(eY) = 0.0;

    break;
  }
  case ttBerndt: {
    vDirectiondAccelDt(eX) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eY) = 1 - 2.0*(double(rand())/double(RAND_MAX));
    vDirectiondAccelDt(eZ) = 1 - 2.0*(double(rand())/double(RAND_MAX));


    MagnitudedAccelDt = 1 - 2.0*(double(rand())/double(RAND_MAX)) - Magnitude;
    MagnitudeAccel    += MagnitudedAccelDt*rate*State->Getdt();
    Magnitude         += MagnitudeAccel*rate*State->Getdt();

    vDirectiondAccelDt.Normalize();
    vDirectionAccel += vDirectiondAccelDt*rate*State->Getdt();
    vDirectionAccel.Normalize();
    vDirection      += vDirectionAccel*rate*State->Getdt();

                                // Diminish z-vector within two wingspans
                                // of the ground
    double HOverBMAC = Position->GetHOverBMAC();
    if (HOverBMAC < 2.0)
        vDirection(eZ) *= HOverBMAC / 2.0;

    vDirection.Normalize();

    vTurbulence = TurbGain*Magnitude * vDirection;
    vTurbulenceGrad = TurbGain*MagnitudeAccel * vDirection;

    vBodyTurbGrad = Rotation->GetTl2b()*vTurbulenceGrad;
    vTurbPQR(eP) = vBodyTurbGrad(eY)/Aircraft->GetWingSpan();
    if (Aircraft->GetHTailArm() != 0.0)
      vTurbPQR(eQ) = vBodyTurbGrad(eZ)/Aircraft->GetHTailArm();
    else
      vTurbPQR(eQ) = vBodyTurbGrad(eZ)/10.0;

    if (Aircraft->GetVTailArm())
      vTurbPQR(eR) = vBodyTurbGrad(eX)/Aircraft->GetVTailArm();
    else
      vTurbPQR(eR) = vBodyTurbGrad(eX)/10.0;

    break;
  }
  default:
    break;
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

void FGMars::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGMars" << endl;
    if (from == 1) cout << "Destroyed:    FGMars" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 32) { // Turbulence
    if (frame == 0 && from == 2) {
      cout << "vTurbulence(X), vTurbulence(Y), vTurbulence(Z), "
           << "vTurbulenceGrad(X), vTurbulenceGrad(Y), vTurbulenceGrad(Z), "
           << "vDirection(X), vDirection(Y), vDirection(Z), "
           << "Magnitude, "
           << "vTurbPQR(P), vTurbPQR(Q), vTurbPQR(R), " << endl;
    } else if (from == 2) {
      cout << vTurbulence << ", " << vTurbulenceGrad << ", " << vDirection << ", " << Magnitude << ", " << vTurbPQR << endl;
    }
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim
