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
#  ifdef FG_HAVE_STD_INCLUDES
#    include <vector>
#  else
#    include <vector.h>
#  endif
#else
#  include <vector>
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

#define ID_PROPULSION "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPropulsion.h,v 1.14 2001/01/11 06:34:02 jsb Exp $"

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
    @version $Id: FGPropulsion.h,v 1.14 2001/01/11 06:34:02 jsb Exp $
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
  ~FGPropulsion(void);
  
  bool Run(void);
  /** Loads the propulsion system (engine[s], tank[s], thruster[s]).
      Characteristics of the propulsion system are read in from the config file.
      @param AC_cfg pointer to the config file instance that describes the
             aircraft being modeled.
      @return true if successfully loaded, otherwise false */
  bool LoadPropulsion(FGConfigFile* AC_cfg);
  /// Retrieves the number of engines defined for the aircraft.
  inline int GetNumEngines(void) {return Engines.size();}
  /** Retrieves an engine object pointer from the list of engines.
      @param index the engine index within the vector container
      @return the address of the specific engine, or zero if no such engine is
              available */
  inline FGEngine* GetEngine(int index) {
                      if (index <= Engines.size()-1) return Engines[index];
                      else                           return 0L;      }
  /** Retrieves a tank object pointer from the list of tanks.
      @param index the tank index within the vector container
      @return the address of the specific tank, or zero if no such tank is
              available */
  inline FGTank* GetTank(int index) {
                      if (index <= Tanks.size()-1) return Tanks[index];
                      else                         return 0L;        }
  /** Retrieves a thruster object pointer from the list of thrusters.
      @param index the thruster index within the vector container
      @return the address of the specific thruster, or zero if no such thruster is
              available */
  inline FGThruster* GetThruster(int index) {
                      if (index <= Thrusters.size()-1) return Thrusters[index];
                      else                             return 0L;    }

  /** Returns the number of fuel tanks currently actively supplying fuel */
  inline int GetnumSelectedFuelTanks(void) {return numSelectedFuelTanks;}
  /** Returns the number of oxidizer tanks currently actively supplying oxidizer */
  inline int GetnumSelectedOxiTanks(void)  {return numSelectedOxiTanks;}

private:
  vector <FGEngine*>   Engines;
  vector <FGTank*>     Tanks;
  vector <FGThruster*> Thrusters;
  int numSelectedFuelTanks;
  int numSelectedOxiTanks;
  int numFuelTanks;
  int numOxiTanks;
  int numEngines;
  int numTanks;
  int numThrusters;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

