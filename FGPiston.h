/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPiston.h
 Author:       Jon S. Berndt
 Date started: 09/12/2000

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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

HISTORY
--------------------------------------------------------------------------------
09/12/2000  JSB  Created
10/01/2001  DPM  Modified to use equations from Dave Luff's piston model.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPISTON_H
#define FGPISTON_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"
#include "FGConfigFile.h"
#include "FGTable.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PISTON "$Id: FGPiston.h,v 1.35 2003/12/02 12:56:04 jberndt Exp $";

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models Dave Luff's engine model as ported into JSBSim by David Megginson.
    @author Jon S. Berndt (Engine framework code and framework-related mods)
    @author Dave Luff (engine operational code)
    @author David Megginson (porting and additional code)
    @version $Id: FGPiston.h,v 1.35 2003/12/02 12:56:04 jberndt Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPiston : public FGEngine
{
public:
  /// Constructor
  FGPiston(FGFDMExec* exec, FGConfigFile* Eng_cfg);
  /// Destructor
  ~FGPiston();

  double Calculate(double PowerRequired);
  double GetPowerAvailable(void) {return PowerAvailable;}
  double CalcFuelNeed(void);

  void SetMagnetos(int magnetos) {Magnetos = magnetos;}

  double  GetEGT(void) { return EGT_degC; }
  int     GetMagnetos(void) {return Magnetos;}

  double getExhaustGasTemp_degF(void) {return KelvinToFahrenheit(ExhaustGasTemp_degK);}
  double getManifoldPressure_inHg(void) const {return ManifoldPressure_inHg;}
  double getCylinderHeadTemp_degF(void) {return KelvinToFahrenheit(CylinderHeadTemp_degK);}
  double getOilPressure_psi(void) const {return OilPressure_psi;}
  double getOilTemp_degF (void) {return KelvinToFahrenheit(OilTemp_degK);}
  double getRPM(void) {return RPM;} 

private:
  int crank_counter;

  double BrakeHorsePower;
  double SpeedSlope;
  double SpeedIntercept;
  double AltitudeSlope;
  double PowerAvailable;

  // timestep
  double dt;

  void doEngineStartup(void);
  void doManifoldPressure(void);
  void doAirFlow(void);
  void doFuelFlow(void);
  void doEnginePower(void);
  void doEGT(void);
  void doCHT(void);
  void doOilPressure(void);
  void doOilTemperature(void);

  //
  // constants
  //

  const double R_air;
  const double rho_fuel;    // kg/m^3
  const double calorific_value_fuel;  // W/Kg (approximate)
  const double Cp_air;      // J/KgK
  const double Cp_fuel;     // J/KgK

  FGTable *Lookup_Combustion_Efficiency;
  FGTable *Power_Mixture_Correlation;

  //
  // Configuration
  //
  double MinManifoldPressure_inHg; // Inches Hg
  double MaxManifoldPressure_inHg; // Inches Hg
  double Displacement;             // cubic inches
  double MaxHP;                    // horsepower
  double Cycles;                   // cycles/power stroke
  double IdleRPM;                  // revolutions per minute

  //
  // Inputs (in addition to those in FGEngine).
  //
  double p_amb;              // Pascals
  double p_amb_sea_level;    // Pascals
  double T_amb;              // degrees Kelvin
  double RPM;                // revolutions per minute
  double IAS;                // knots
  bool Magneto_Left;
  bool Magneto_Right;
  bool Magnetos;

  //
  // Outputs (in addition to those in FGEngine).
  //
  double rho_air;
  double volumetric_efficiency;
  double m_dot_air;
  double equivalence_ratio;
  double m_dot_fuel;
  double Percentage_Power;
  double HP;
  double combustion_efficiency;
  double ExhaustGasTemp_degK;
  double EGT_degC;
  double ManifoldPressure_inHg;
  double CylinderHeadTemp_degK;
  double OilPressure_psi;
  double OilTemp_degK;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
