/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSimTurbine.h
 Author:       David Culp
 Date started: 03/11/2003

 ------------- Copyright (C) 2003  David Culp (davidculp2@attbi.com)------------

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
03/11/2003  DPC  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSIMTURBINE_H
#define FGSIMTURBINE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include "FGEngine.h"
#include "FGConfigFile.h"
#include "FGCoefficient.h"

#define ID_SIMTURBINE "$Id: FGSimTurbine.h,v 1.7 2003/07/26 09:06:02 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGSimTurbine : public FGEngine
{
public:
  FGSimTurbine(FGFDMExec* exec, FGConfigFile* Eng_cfg);
  ~FGSimTurbine();

  double Calculate(double);
  double CalcFuelNeed(void);
  double GetPowerAvailable(void);
  
private:

  typedef vector<FGCoefficient*> CoeffArray;
  CoeffArray ThrustTables;

  double MaxMilThrust;     // Maximum Rated Thrust, static @ S.L. (lbf)
  double BypassRatio;      // Bypass Ratio
  double TSFC;             // Thrust Specific Fuel Consumption (lbm/hr/lbf)
  double ATSFC;            // Augmented TSFC (lbm/hr/lbf)
  double IdleN1;           // Idle N1
  double IdleN2;           // Idle N2
  double MaxN1;            // N1 at 100% throttle
  double MaxN2;            // N2 at 100% throttle
  double IdleFF;           // Idle Fuel Flow (lbm/hr)
  double delay;            // Inverse spool-up time from idle to 100% (seconds)
  double dt;               // Simulator time slice
  double N1_factor;        // factor to tie N1 and throttle
  double N2_factor;        // factor to tie N2 and throttle
  double ThrottleCmd;      // FCS-supplied throttle position
  double throttle;         // virtual throttle position
  int Augmented;           // = 1 if augmentation installed
  int Injected;            // = 1 if water injection installed
  int AugMethod;           // = 0 if using property /engine[n]/augmentation
                           // = 1 if using last 1% of throttle movement

  void SetDefaults(void);
  bool Load(FGConfigFile *ENG_cfg);
  void Debug(int from);

};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

