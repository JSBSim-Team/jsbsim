/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGEngine.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
See header file.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created
09/03/99   JSB   Changed Rocket thrust equation to correct -= Thrust instead of
                 += Thrust (thanks to Tony Peden)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
#else
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <fstream.h>
#  else
#    include <fstream>
#  endif
#endif

#include "FGEngine.h"
#include "FGTank.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGEngine.cpp,v 1.63 2004/04/26 16:39:57 dpculp Exp $";
static const char *IdHdr = ID_ENGINE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGEngine::FGEngine(FGFDMExec* exec)
{
  Name = "";
  Type = etUnknown;
  X = Y = Z = 0.0;
  EnginePitch = EngineYaw = 0.0;
  SLFuelFlowMax = SLOxiFlowMax = 0.0;
  MaxThrottle = 1.0;
  MinThrottle = 0.0;
  Thrust = 0.0;
  Throttle = 0.0;
  Mixture = 1.0;
  Starter = false;
  FuelNeed = OxidizerNeed = 0.0;
  Starved = Running = Cranking = false;
  PctPower = 0.0;
  EngineNumber = -1;
  TrimMode = false;
  FuelFlow_gph = 0.0;
  FuelFlow_pph = 0.0;

  FDMExec = exec;
  State = FDMExec->GetState();
  Atmosphere = FDMExec->GetAtmosphere();
  FCS = FDMExec->GetFCS();
  Propulsion = FDMExec->GetPropulsion();
  Aircraft = FDMExec->GetAircraft();
  Propagate = FDMExec->GetPropagate();
  Auxiliary = FDMExec->GetAuxiliary();
  Output = FDMExec->GetOutput();

  PropertyManager = FDMExec->GetPropertyManager();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGEngine::~FGEngine()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// This base class function should be called from within the
// derived class' Calculate() function before any other calculations are done.
// This base class method removes fuel from the fuel tanks as appropriate,
// and sets the starved flag if necessary.

void FGEngine::ConsumeFuel(void)
{
  double Fshortage, Oshortage;
  FGTank* Tank;

  if (TrimMode) return;
  Fshortage = Oshortage = 0.0;
  for (unsigned int i=0; i<SourceTanks.size(); i++) {
    Tank = Propulsion->GetTank(SourceTanks[i]);
    if (Tank->GetType() == FGTank::ttFUEL) {
      Fshortage += Tank->Reduce(CalcFuelNeed()/SourceTanks.size());
    } else {
      Oshortage += Tank->Reduce(CalcOxidizerNeed()/SourceTanks.size());
    }
  }

  if (Fshortage < 0.00 || Oshortage < 0.00) Starved = true;
  else Starved = false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGEngine::CalcFuelNeed(void)
{
  FuelNeed = SLFuelFlowMax*PctPower*State->Getdt()*Propulsion->GetRate();
  return FuelNeed;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGEngine::CalcOxidizerNeed(void)
{
  OxidizerNeed = SLOxiFlowMax*PctPower*State->Getdt()*Propulsion->GetRate();
  return OxidizerNeed;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::SetPlacement(double x, double y, double z, double pitch, double yaw)
{
  X = x;
  Y = y;
  Z = z;
  EnginePitch = pitch;
  EngineYaw = yaw;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGEngine::AddFeedTank(int tkID)
{
  SourceTanks.push_back(tkID);
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

void FGEngine::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGEngine" << endl;
    if (from == 1) cout << "Destroyed:    FGEngine" << endl;
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
