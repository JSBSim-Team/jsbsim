/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropulsion.h
 Author:       Jon S. Berndt
 Date started: 08/20/00
 
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

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <vector>
#    include <iterator>
#  else
#    include <vector.h>
#    include <iterator.h>
#  endif
#else
#  include <vector>
#  include <iterator>
#endif

#include "FGModel.h"

#include "FGRocket.h"
#include "FGPiston.h"
#include "FGTurboShaft.h"
#include "FGTurboJet.h"
#include "FGTurboProp.h"
#include "FGTank.h"
#include "FGPropeller.h"
#include "FGNozzle.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPULSION "$Id: FGPropulsion.h,v 1.39 2002/01/19 03:01:59 dmegginson Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Propulsion management class.
    FGPropulsion manages all aspects of propulsive force generation, including
    containment of engines, tanks, and thruster class instances in STL vectors,
    and the interaction and communication between them.
    @author Jon S. Berndt
    @version $Id: FGPropulsion.h,v 1.39 2002/01/19 03:01:59 dmegginson Exp $
    @see FGEngine
    @see FGTank
    @see FGThruster
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPropulsion : public FGModel {
public:
  FGPropulsion(FGFDMExec*);
  ~FGPropulsion();

  /** Executes the propulsion model.
      The initial plan for the FGPropulsion class calls for Run() to be executed,
      performing the following tasks:
      <ol>
  <li>Determine the drag - or power required - for the attached thrust effector
      for this engine so that any feedback to the engine can be performed. This
      is done by calling FGThruster::CalculatePReq()</li>
  <li>Given 1, above, calculate the power available from the engine. This is
      done by calling FGEngine::CalculatePAvail()</li>
  <li>Next, calculate the thrust output from the thruster model given the power
      available and the power required. This may also result in new performance
      numbers for the thruster in the case of the propeller, at least. This
      result is returned from a call to Calculate().</li></ol>

      [Note: Should we be checking the Starved flag here?] */
  bool Run(void);

  /** Loads the propulsion system (engine[s], tank[s], thruster[s]).
      Characteristics of the propulsion system are read in from the config file.
      @param AC_cfg pointer to the config file instance that describes the
             aircraft being modeled.
      @return true if successfully loaded, otherwise false */
  bool Load(FGConfigFile* AC_cfg);

  /// Retrieves the number of engines defined for the aircraft.
  inline unsigned int GetNumEngines(void) {return Engines.size();}

  /** Retrieves an engine object pointer from the list of engines.
      @param index the engine index within the vector container
      @return the address of the specific engine, or zero if no such engine is
              available */
  inline FGEngine* GetEngine(unsigned int index) {
                      if (index <= Engines.size()-1) return Engines[index];
                      else                           return 0L;      }

  // Retrieves the number of tanks defined for the aircraft.
  inline unsigned int GetNumTanks(void) {return Tanks.size();}

  /** Retrieves a tank object pointer from the list of tanks.
      @param index the tank index within the vector container
      @return the address of the specific tank, or zero if no such tank is
              available */
  inline FGTank* GetTank(unsigned int index) {
                      if (index <= Tanks.size()-1) return Tanks[index];
                      else                         return 0L;        }

  /** Retrieves a thruster object pointer from the list of thrusters.
      @param index the thruster index within the vector container
      @return the address of the specific thruster, or zero if no such thruster is
              available */
  inline FGThruster* GetThruster(unsigned int index) {
                      if (index <= Thrusters.size()-1) return Thrusters[index];
                      else                             return 0L;    }

  /** Returns the number of fuel tanks currently actively supplying fuel */
  inline int GetnumSelectedFuelTanks(void) {return numSelectedFuelTanks;}

  /** Returns the number of oxidizer tanks currently actively supplying oxidizer */
  inline int GetnumSelectedOxiTanks(void)  {return numSelectedOxiTanks;}

  /** Loops the engines/thrusters until thrust output steady (used for trimming) */
  bool GetSteadyState(void);
  
  /** starts the engines in IC mode (dt=0).  All engine-specific setup must
      be done before calling this (i.e. magnetos, starter engage, etc.) */
  bool ICEngineStart(void);
  
  string GetPropulsionStrings(void);
  string GetPropulsionValues(void);

  inline FGColumnVector3& GetForces(void)  {return vForces; }
  inline double GetForces(int n) { return vForces(n);}
  inline FGColumnVector3& GetMoments(void) {return vMoments;}
  inline double GetMoments(int n) {return vMoments(n);}
  
  FGColumnVector3& GetTanksCG(void);
  double GetTanksWeight(void);

  double GetTanksIxx(const FGColumnVector3& vXYZcg);
  double GetTanksIyy(const FGColumnVector3& vXYZcg);
  double GetTanksIzz(const FGColumnVector3& vXYZcg);
  double GetTanksIxz(const FGColumnVector3& vXYZcg);
  double GetTanksIxy(const FGColumnVector3& vXYZcg);

private:
  vector <FGEngine*>   Engines;
  vector <FGTank*>     Tanks;
  vector <FGTank*>::iterator iTank;
  vector <FGThruster*> Thrusters;
  unsigned int numSelectedFuelTanks;
  unsigned int numSelectedOxiTanks;
  unsigned int numFuelTanks;
  unsigned int numOxiTanks;
  unsigned int numEngines;
  unsigned int numTanks;
  unsigned int numThrusters;
  double dt;
  FGColumnVector3 vForces;
  FGColumnVector3 vMoments;
  FGColumnVector3 vXYZtank;
  void Debug(int from);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

