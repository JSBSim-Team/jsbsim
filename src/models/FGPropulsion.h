/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropulsion.h
 Author:       Jon S. Berndt
 Date started: 08/20/00

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
#include <fstream>

#include "FGModel.h"
#include <models/propulsion/FGEngine.h>
#include <models/propulsion/FGTank.h>
#include <math/FGMatrix33.h>
#include <input_output/FGXMLFileRead.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPULSION "$Id: FGPropulsion.h,v 1.21 2009/06/09 03:23:55 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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
    </propulsion>
  @endcode

    @author Jon S. Berndt
    @version $Id: FGPropulsion.h,v 1.21 2009/06/09 03:23:55 jberndt Exp $
    @see
    FGEngine
    FGTank
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPropulsion : public FGModel, public FGXMLFileRead
{
public:
  /// Constructor
  FGPropulsion(FGFDMExec*);
  /// Destructor
  ~FGPropulsion();

  /** Executes the propulsion model.
      The initial plan for the FGPropulsion class calls for Run() to be executed,
      calculating the power available from the engine.

      [Note: Should we be checking the Starved flag here?] */
  bool Run(void);

  bool InitModel(void);

  /** Loads the propulsion system (engine[s] and tank[s]).
      Characteristics of the propulsion system are read in from the config file.
      @param el pointer to an XML element that contains the engine information.
      @return true if successfully loaded, otherwise false */
  bool Load(Element* el);

  /// Retrieves the number of engines defined for the aircraft.
  inline unsigned int GetNumEngines(void) const {return (unsigned int)Engines.size();}

  /** Retrieves an engine object pointer from the list of engines.
      @param index the engine index within the vector container
      @return the address of the specific engine, or zero if no such engine is
              available */
  inline FGEngine* GetEngine(unsigned int index) {
                      if (index <= Engines.size()-1) return Engines[index];
                      else                           return 0L;      }

  /// Retrieves the number of tanks defined for the aircraft.
  inline unsigned int GetNumTanks(void) const {return (unsigned int)Tanks.size();}

  /** Retrieves a tank object pointer from the list of tanks.
      @param index the tank index within the vector container
      @return the address of the specific tank, or zero if no such tank is
              available */
  inline FGTank* GetTank(unsigned int index) {
                      if (index <= Tanks.size()-1) return Tanks[index];
                      else                         return 0L;        }

  /** Returns the number of fuel tanks currently actively supplying fuel */
  inline int GetnumSelectedFuelTanks(void) const {return numSelectedFuelTanks;}

  /** Returns the number of oxidizer tanks currently actively supplying oxidizer */
  inline int GetnumSelectedOxiTanks(void) const {return numSelectedOxiTanks;}

  /** Loops the engines until thrust output steady (used for trimming) */
  bool GetSteadyState(void);

  /** Sets up the engines as running */
  void InitRunning(int n);

  string GetPropulsionStrings(string delimeter);
  string GetPropulsionValues(string delimeter);

  inline FGColumnVector3& GetForces(void)  {return vForces; }
  inline double GetForces(int n) const { return vForces(n);}
  inline FGColumnVector3& GetMoments(void) {return vMoments;}
  inline double GetMoments(int n) const {return vMoments(n);}

  inline bool GetRefuel(void) const {return refuel;}
  inline void SetRefuel(bool setting) {refuel = setting;}
  inline bool GetFuelDump(void) const {return dump;}
  inline void SetFuelDump(bool setting) {dump = setting;}
  double Transfer(int source, int target, double amount);
  void DoRefuel(double time_slice);
  void DumpFuel(double time_slice);

  FGColumnVector3& GetTanksMoment(void);
  double GetTanksWeight(void);

  ifstream* FindEngineFile(string filename);
  string FindEngineFullPathname(string engine_filename);
  inline int GetActiveEngine(void) const {return ActiveEngine;}
  inline bool GetFuelFreeze(void) {return fuel_freeze;}
  double GetTotalFuelQuantity(void) const {return TotalFuelQuantity;}

  void SetMagnetos(int setting);
  void SetStarter(int setting);
  void SetCutoff(int setting=0);
  void SetActiveEngine(int engine);
  void SetFuelFreeze(bool f);
  FGMatrix33& CalculateTankInertias(void);

private:
  vector <FGEngine*>   Engines;
  vector <FGTank*>     Tanks;
  unsigned int numSelectedFuelTanks;
  unsigned int numSelectedOxiTanks;
  unsigned int numFuelTanks;
  unsigned int numOxiTanks;
  unsigned int numEngines;
  unsigned int numTanks;
  int ActiveEngine;
  FGColumnVector3 vForces;
  FGColumnVector3 vMoments;
  FGColumnVector3 vTankXYZ;
  FGColumnVector3 vXYZtank_arm;
  FGMatrix33 tankJ;
  bool refuel;
  bool dump;
  bool fuel_freeze;
  double TotalFuelQuantity;
  double DumpRate;
  bool IsBound;
  bool HavePistonEngine;
  bool HaveTurbineEngine;
  bool HaveTurboPropEngine;
  bool HaveRocketEngine;
  bool HaveElectricEngine;

  int InitializedEngines;
  bool HasInitializedEngines;

  void bind();
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

