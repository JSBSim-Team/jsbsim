/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropulsion.h
 Author:       Jon S. Berndt
 Date started: 08/20/00

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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

HISTORY
--------------------------------------------------------------------------------
08/20/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPULSION_H
#define FGPROPULSION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include <iosfwd>

#include "FGModel.h"
#include "propulsion/FGEngine.h"
#include "math/FGMatrix33.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGTank;
class FGEngine;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Propulsion management class.
    The Propulsion class is the container for the entire propulsion system, which is
    comprised of engines, and tanks. Once the Propulsion class gets the config file,
    it reads in the \<propulsion> section. Then:

    -# The appropriate engine type instance is created
    -# At least one tank object is created, and is linked to an engine.

    At Run time each engine's Calculate() method is called.

    <h3>Configuration File Format:</h3>

  @code
    <propulsion>
        <engine file="{string}">
          ... see FGEngine, FGThruster, and class for engine type ...
        </engine>
        ... more engines ...
        <tank type="{FUEL | OXIDIZER}"> 
          ... see FGTank ...
        </tank>
        ... more tanks ...
        <dump-rate unit="{LBS/MIN | KG/MIN}"> {number} </dump-rate>
        <refuel-rate unit="{LBS/MIN | KG/MIN}"> {number} </refuel-rate>
    </propulsion>
  @endcode

    @author Jon S. Berndt
    @see
    FGEngine
    FGTank
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGPropulsion : public FGModel
{
public:
  /// Constructor
  FGPropulsion(FGFDMExec*);
  /// Destructor
  ~FGPropulsion() override;

  /** Executes the propulsion model.
      The initial plan for the FGPropulsion class calls for Run() to be executed,
      calculating the power available from the engine.
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;

  bool InitModel(void) override;

  /** Loads the propulsion system (engine[s] and tank[s]).
      Characteristics of the propulsion system are read in from the config file.
      @param el pointer to an XML element that contains the engine information.
      @return true if successfully loaded, otherwise false */
  bool Load(Element* el) override;

  /// Retrieves the number of engines defined for the aircraft.
  size_t GetNumEngines(void) const {return Engines.size();}

  /** Retrieves an engine object pointer from the list of engines.
      @param index the engine index within the vector container
      @return the address of the specific engine, or zero if no such engine is
              available */
  auto GetEngine(unsigned int index) const {
    assert(index < Engines.size());
    return Engines[index];
  }

  /// Retrieves the number of tanks defined for the aircraft.
  size_t GetNumTanks(void) const {return Tanks.size();}

  /** Retrieves a tank object pointer from the list of tanks.
      @param index the tank index within the vector container
      @return the address of the specific tank, or zero if no such tank is
              available */
  auto GetTank(unsigned int index) const {
    assert(index < Tanks.size());
    return Tanks[index];
  }

  /** Loops the engines until thrust output steady (used for trimming) */
  bool GetSteadyState(void);

  /** Sets up the engines as running */
  void InitRunning(int n);
  void SetEngineRunning(int index);

  std::string GetPropulsionStrings(const std::string& delimiter) const;
  std::string GetPropulsionValues(const std::string& delimiter) const;
  std::string GetPropulsionTankReport();

  const FGColumnVector3& GetForces(void) const {return vForces; }
  double GetForces(int n) const { return vForces(n);}
  const FGColumnVector3& GetMoments(void) const {return vMoments;}
  double GetMoments(int n) const {return vMoments(n);}

  double Transfer(int source, int target, double amount);
  void DoRefuel(double time_slice);
  void DumpFuel(double time_slice);

  const FGColumnVector3& GetTanksMoment(void);
  double GetTanksWeight(void) const;

  SGPath FindFullPathName(const SGPath& path) const override;
  inline int GetActiveEngine(void) const {return ActiveEngine;}
  inline bool GetFuelFreeze(void) const {return FuelFreeze;}

  void SetMagnetos(int setting);
  void SetStarter(int setting);
  int GetStarter(void) const;
  void SetCutoff(int setting=0);
  int GetCutoff(void) const;
  void SetActiveEngine(int engine);
  void SetFuelFreeze(bool f);
  const FGMatrix33& CalculateTankInertias(void);

  struct FGEngine::Inputs in;

private:
  std::vector<std::shared_ptr<FGEngine>> Engines;
  std::vector<std::shared_ptr<FGTank>>   Tanks;
  int ActiveEngine;
  FGColumnVector3 vForces;
  FGColumnVector3 vMoments;
  FGColumnVector3 vTankXYZ;
  FGColumnVector3 vXYZtank_arm;
  FGMatrix33 tankJ;
  bool refuel;
  bool dump;
  bool FuelFreeze;
  double TotalFuelQuantity;
  double TotalOxidizerQuantity;
  double DumpRate;
  double RefuelRate;
  void ConsumeFuel(FGEngine* engine);

  bool ReadingEngine;

  void bind();
  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
