/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGEngine.h
 Author:       Jon S. Berndt
 Date started: 01/21/99

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

Based on Flightgear code, which is based on LaRCSim. This class simulates
a generic engine.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGENGINE_H
#define FGENGINE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_STRING
   SG_USING_STD(string);
#  ifdef SG_HAVE_STD_INCLUDES
#    include <vector>
#  else
#    include <vector.h>
#  endif
#else
#  include <vector>
#  include <string>
#endif

#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ENGINE "$Id: FGEngine.h,v 1.51 2002/08/25 13:57:11 jberndt Exp $"

using std::string;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFDMExec;
class FGState;
class FGAtmosphere;
class FGFCS;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPropulsion;
class FGPosition;
class FGAuxiliary;
class FGOutput;

using std::vector;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for all engines.
    This base class contains methods and members common to all engines, such as
    logic to drain fuel from the appropriate tank, etc.
    @author Jon S. Berndt
    @version $Id: FGEngine.h,v 1.51 2002/08/25 13:57:11 jberndt Exp $ 
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGEngine : public FGJSBBase
{
public:
  FGEngine(FGFDMExec* exec);
  virtual ~FGEngine();

  enum EngineType {etUnknown, etRocket, etPiston, etTurbine};

  virtual double  GetThrottleMin(void) { return MinThrottle; }
  virtual double  GetThrottleMax(void) { return MaxThrottle; }
  virtual double  GetThrottle(void) { return Throttle; }
  virtual double  GetMixture(void) { return Mixture; }
  virtual int     GetMagnetos(void) { return Magnetos; }
  virtual bool    GetStarter(void) { return Starter; }
  virtual double  GetThrust(void) { return Thrust; }
  virtual bool    GetStarved(void) { return Starved; }
  virtual bool    GetFlameout(void) { return Flameout; }
  virtual bool    GetRunning(void) { return Running; }
  virtual bool    GetCranking(void) { return Cranking; }
  virtual int     GetType(void) { return Type; }
  virtual string  GetName(void) { return Name; }

  virtual double getFuelFlow_gph () const {
    return FuelFlow_gph;
  }

  virtual double getManifoldPressure_inHg () const {
    return ManifoldPressure_inHg;
  }
  virtual double getExhaustGasTemp_degF () const {
    return (ExhaustGasTemp_degK - 273) * (9.0 / 5.0) + 32.0;
  }
  virtual double getCylinderHeadTemp_degF () const {
    return (CylinderHeadTemp_degK - 273) * (9.0 / 5.0) + 32.0;
  }
  virtual double getOilPressure_psi () const {
    return OilPressure_psi;
  }
  virtual double getOilTemp_degF () const {
    return (OilTemp_degK - 273.0) * (9.0 / 5.0) + 32.0;
  }

  virtual void SetStarved(bool tt) {Starved = tt;}
  virtual void SetStarved(void)    {Starved = true;}

  virtual void SetRunning(bool bb) { Running=bb; }
  virtual void SetName(string name) {Name = name;}
  virtual void AddFeedTank(int tkID);

  virtual void SetMagnetos(int m) { Magnetos = m; }
  virtual void SetStarter(bool s) { Starter = s;}

  /** Calculates the thrust of the engine, and other engine functions.
      @param PowerRequired this is the power required to run the thrusting device
             such as a propeller. This resisting effect must be provided to the 
             engine model.
      @return Thrust in pounds */
  virtual double Calculate(double PowerRequired) {return 0.0;};

  /** Reduces the fuel in the active tanks by the amount required.
      This function should be called from within the
      derived class' Calculate() function before any other calculations are
      done. This base class method removes fuel from the fuel tanks as
      appropriate, and sets the starved flag if necessary. */
  virtual void ConsumeFuel(void);

  /** The fuel need is calculated based on power levels and flow rate for that
      power level. It is also turned from a rate into an actual amount (pounds)
      by multiplying it by the delta T and the rate.
      @return Total fuel requirement for this engine in pounds. */
  virtual double CalcFuelNeed(void);

  /** The oxidizer need is calculated based on power levels and flow rate for that
      power level. It is also turned from a rate into an actual amount (pounds)
      by multiplying it by the delta T and the rate.
      @return Total oxidizer requirement for this engine in pounds. */
  virtual double CalcOxidizerNeed(void);

  /// Sets engine placement information
  virtual void SetPlacement(double x, double y, double z, double pitch, double yaw);

  /// Sets the engine number
  virtual void SetEngineNumber(int nn) {EngineNumber = nn;}

  virtual double GetPowerAvailable(void) {return 0.0;};

  virtual bool GetTrimMode(void) {return TrimMode;}
  virtual void SetTrimMode(bool state) {TrimMode = state;}

protected:
  string Name;
  EngineType Type;
  double X, Y, Z;
  double EnginePitch;
  double EngineYaw;
  double SLFuelFlowMax;
  double SLOxiFlowMax;
  double MaxThrottle;
  double MinThrottle;

  double Thrust;
  double Throttle;
  double Mixture;
  int   Magnetos;
  bool  Starter;
  double FuelNeed, OxidizerNeed;
  bool  Starved;
  bool  Flameout;
  bool  Running;
  bool  Cranking;
  double PctPower;
  int   EngineNumber;
  bool  TrimMode;

  double FuelFlow_gph;
  double ManifoldPressure_inHg;
  double ExhaustGasTemp_degK;
  double CylinderHeadTemp_degK;
  double OilPressure_psi;
  double OilTemp_degK;

  FGFDMExec*      FDMExec;
  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGPropulsion*   Propulsion;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGOutput*       Output;

  vector <int> SourceTanks;
  virtual void Debug(int from);
};

#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPropulsion.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

