/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGEngine.h
 Author:       Jon S. Berndt
 Date started: 01/21/99

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
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

#include <vector>
#include <string>

#include "math/FGModelFunctions.h"
#include "math/FGColumnVector3.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFDMExec;
class FGThruster;
class Element;
class FGPropertyManager;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for all engines.
    This base class contains methods and members common to all engines, such as
    logic to drain fuel from the appropriate tank, etc. 
    <br>
    <h3>Configuration File Format:</h3>
@code
        <engine file="{string}">
            <location unit="{IN | M}">
                <x> {number} </x>
                <y> {number} </y>
                <z> {number} </z>
            </location>
            <!-- optional orientation definition -->
            <orient unit="{RAD | DEG}">
                <roll>  {number} </roll>
                <pitch> {number} </pitch>
                <yaw> {number} </yaw>
            </orient>
            <feed> {integer} </feed>
            ... optional more feed tank index numbers ... 
            <thruster file="{string}">
                <location unit="{IN | M}">
                    <x> {number} </x>
                    <y> {number} </y>
                    <z> {number} </z>
                </location>
                <orient unit="{RAD | DEG}">
                    <roll> {number} </roll>
                    <pitch> {number} </pitch>
                    <yaw> {number} </yaw>
                </orient>
            </thruster>
        </engine>
@endcode
<pre>
    NOTES:

  Not all thruster types can be matched with a given engine type.  See the class
  documentation for engine and thruster classes.
</pre>     
    @author Jon S. Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGEngine : public FGModelFunctions
{
public:
  struct Inputs {
    double Pressure;
    double PressureRatio;
    double Temperature;
    double Density;
    double DensityRatio;
    double Soundspeed;
    double TotalPressure;
    double TAT_c;
    double Vt;
    double Vc;
    double qbar;
    double alpha;
    double beta;
    double H_agl;
    FGColumnVector3 AeroUVW;
    FGColumnVector3 AeroPQR;
    FGColumnVector3 PQRi;
    std::vector <double> ThrottleCmd;
    std::vector <double> MixtureCmd;
    std::vector <double> ThrottlePos;
    std::vector <double> MixturePos;
    std::vector <double> PropAdvance;
    std::vector <bool> PropFeather;
    double TotalDeltaT;
  };

  FGEngine(int engine_number, struct Inputs& input);
  ~FGEngine() override;

  enum EngineType {etUnknown, etRocket, etPiston, etTurbine, etTurboprop, etElectric};

  EngineType GetType(void) const { return Type; }
  virtual const std::string&  GetName(void) const { return Name; }

  // Engine controls
  virtual double  GetThrottleMin(void) const { return MinThrottle; }
  virtual double  GetThrottleMax(void) const { return MaxThrottle; }
  virtual bool    GetStarter(void) const     { return Starter; }

  virtual double getFuelFlow_gph () const {return FuelFlow_gph;}
  virtual double getFuelFlow_pph () const {return FuelFlow_pph;}
  virtual double GetFuelFlowRate(void) const {return FuelFlowRate;}
  virtual double GetFuelFlowRateGPH(void) const {return FuelFlowRate*3600/FuelDensity;}
  virtual double GetFuelUsedLbs(void) const {return FuelUsedLbs;}
  virtual bool   GetStarved(void) const { return Starved; }
  virtual bool   GetRunning(void) const { return Running; }
  virtual bool   GetCranking(void) const { return Cranking; }

  virtual void SetStarved(bool tt) { Starved = tt; }
  virtual void SetStarved(void)    { Starved = true; }

  virtual void SetRunning(bool bb) { Running=bb; }
  virtual void SetName(const std::string& name) { Name = name; }
  virtual void SetFuelFreeze(bool f) { FuelFreeze = f; }
  virtual void SetFuelDensity(double d) { FuelDensity = d; }

  virtual void SetStarter(bool s) { Starter = s; }

  virtual int InitRunning(void){ return 1; }

  /** Resets the Engine parameters to the initial conditions */
  virtual void ResetToIC(void);

  /** Calculates the thrust of the engine, and other engine functions. */
  virtual void Calculate(void) = 0;

  virtual double GetThrust(void) const;
    
  /// Sets engine placement information
  virtual void SetPlacement(const FGColumnVector3& location, const FGColumnVector3& orientation);

  /** The fuel need is calculated based on power levels and flow rate for that
      power level. It is also turned from a rate into an actual amount (pounds)
      by multiplying it by the delta T and the rate.
      @return Total fuel requirement for this engine in pounds. */
  virtual double CalcFuelNeed(void);

  virtual double CalcOxidizerNeed(void) {return 0.0;}

  virtual double GetPowerAvailable(void) {return 0.0;};

  virtual const FGColumnVector3& GetBodyForces(void);
  virtual const FGColumnVector3& GetMoments(void);

  void LoadThruster(FGFDMExec* exec, Element *el);
  FGThruster* GetThruster(void) const {return Thruster;}

  unsigned int GetSourceTank(unsigned int i) const;
  size_t GetNumSourceTanks() const {return SourceTanks.size();}

  virtual std::string GetEngineLabels(const std::string& delimiter) = 0;
  virtual std::string GetEngineValues(const std::string& delimiter) = 0;

  struct Inputs& in;
  void LoadThrusterInputs();

protected:

  std::string Name;
  const int   EngineNumber;
  EngineType Type;
  double X, Y, Z;
  double EnginePitch;
  double EngineYaw;
  double SLFuelFlowMax;
  double MaxThrottle;
  double MinThrottle;

  double FuelExpended;
  double FuelFlowRate;
  double PctPower;
  bool  Starter;
  bool  Starved;
  bool  Running;
  bool  Cranking;
  bool  FuelFreeze;

  double FuelFlow_gph;
  double FuelFlow_pph;
  double FuelUsedLbs;
  double FuelDensity;

  FGThruster* Thruster;

  std::vector <int> SourceTanks;

  bool Load(FGFDMExec *exec, Element *el);
  void Debug(int from);
};
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
