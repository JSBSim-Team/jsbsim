/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGBrussLessDCMotor.h
 Author:       Paolo Becchi
 Date started: 1-1-2022

 ----- Copyright (C) 2022  Paolo Becchi (pbecchi@aerobusiness.it) --------------
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
  found on the world wide web at http://www.gnu.org

HISTORY
--------------------------------------------------------------------------------
1/01/2022   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGBrushLessDCMotor_H
#define FGBrushLessDCMotor_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models an electric brushless DC motor or more appropriately permanent magnet
    synchronous motor. as alternative to basic electric motor.
BLDC motor code is based on basic "3 constant motor equations"
It require 3 basic physical motor properties (available from manufactures):
Kv speed motor constant      [RPM/Volt]
Rm internal coil resistance  [Ohms]
I0 no load current           [Amperes]
additional inputs :
maxvolts                     nominal voltage  from battery

Input format :
  <brushless_dc_motor>
    <maxvolts units="VOLTS">         {number} </maxvolts>
    <velocityconstant units="RPM/V"> {number} </velocityconstant>
    <coilresistance units="OHMS">    {number} </coilresistance>
    <noloadcurrent units="AMPERES">  {number} </noloadcurrent>
  </brushless_dc_motor>

  */

// conversion factors
constexpr double NMtoftpound = 1.3558;
constexpr double hptowatts = 745.7;
constexpr double WattperRPMtoftpound = 60 / (2 * M_PI * NMtoftpound);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGBrushLessDCMotor : public FGEngine
{
public:
  /// Constructor
  FGBrushLessDCMotor(FGFDMExec* exec, Element *el, int engine_number, FGEngine::Inputs& input);
  /// Destructor
  ~FGBrushLessDCMotor();

  void Calculate(void);
  double GetPowerAvailable(void) {return (HP * hptoftlbssec);}
  double CalcFuelNeed(void) { return 0.; }
  std::string GetEngineLabels(const std::string& delimiter);
  std::string GetEngineValues(const std::string& delimiter);

private:
  double ZeroTorqueCurrent; // Zero torque current [A]
  double CoilResistance;    // Internal resistance [Ohm]
  double PowerWatts;        // Maximum engine power
  double MaxVolts;          // Max voltage available from battery [V]
  double Kv;                // Speed constant of brusless DC motors [RPM/V]
  double HP;                // Engine output, in horsepower
  double Current;           // Current [A]
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
