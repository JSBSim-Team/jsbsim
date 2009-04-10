/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTank.h
 Author:       Jon S. Berndt
 Date started: 01/21/99

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

Based on Flightgear code, which is based on LaRCSim. This class simulates
a generic Tank.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTank_H
#define FGTank_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include <FGJSBBase.h>
#include <input_output/FGXMLElement.h>
#include <math/FGColumnVector3.h>
#include <string>

using std::string;
using std::cerr;
using std::endl;
using std::cout;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TANK "$Id: FGTank.h,v 1.14 2009/04/10 11:40:36 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a fuel tank.

<h3>Fuel Temperature:</h3>
 
    Fuel temperature is calculated using the following assumptions:

    Fuel temperature will only be calculated for tanks which have an initial fuel
    temperature specified in the configuration file.

    The surface area of the tank is estimated from the capacity in pounds.  It
    is assumed that the tank is a wing tank with dimensions h by 4h by 10h. The
    volume of the tank is then 40(h)(h)(h). The area of the upper or lower 
    surface is then 40(h)(h).  The volume is also equal to the capacity divided
    by 49.368 lbs/cu-ft, for jet fuel.  The surface area of one side can then be
    derived from the tank's capacity.  

    The heat capacity of jet fuel is assumed to be 900 Joules/lbm/K, and the 
    heat transfer factor of the tank is 1.115 Watts/sq-ft/K.

<h3>Fuel Dump:</h3>

    Fuel dumping is handled by the FGPropulsion class.  A standpipe can be defined
    here for each tank which sets the level of contents (in pounds) which is not dumpable.
    Default standpipe level is zero, making all contents dumpable.

<h3>Fuel Transfer:</h3>

    Fuel transfer is handled by the FGPropulsion class, however the contents of tanks
    may be manipulated directly using the SetContents() function here, or via the property
    tree at <tt>propulsion/tank[i]/contents-lbs</tt>, where i is the tank number (Tanks
    are automatically numbered, starting at zero, in the order in which they are read in
    the aircraft configuration file).  The latter method allows one to use a system of FCS
    components to control tank contents. 

<h3>Configuration File Format:</h3>

@code
<tank type="{FUEL | OXIDIZER}">
  <grain_config type="{CYLINDRICAL | ENDBURNING}">
    <length unit="{IN | FT | M}"> {number} </radius>
  </grain_config>
  <location unit="{FT | M | IN}">
    <x> {number} </x>
    <y> {number} </y>
    <z> {number} </z>
  </location>
  <drain_location unit="{FT | M | IN}">
    <x> {number} </x>
    <y> {number} </y>
    <z> {number} </z>
  </drain_location>
  <radius unit="{IN | FT | M}"> {number} </radius>
  <capacity unit="{LBS | KG}"> {number} </capacity>
  <contents unit="{LBS | KG}"> {number} </contents>
  <temperature> {number} </temperature> <!-- must be degrees fahrenheit -->
  <standpipe unit="{LBS | KG"}> {number} </standpipe>
</tank>
@endcode

<h3>Definition of the tank configuration file parameters:</h3>

- \b type - One of FUEL or OXIDIZER.  This is required.
- \b radius - Equivalent radius of tank for modeling slosh, defaults to inches.
- \b grain_config type - One of CYLINDRICAL or ENDBURNING.
- \b length - length of tank for modeling solid fuel propellant grain, defaults to inches.
- \b capacity - Capacity, defaults to pounds.
- \b contents - Initial contents, defaults to pounds.
- \b temperature - Initial temperature, defaults to degrees Fahrenheit.
- \b standpipe - Minimum contents to which tank can dump, defaults to pounds.

location:
- \b x - Location of tank on aircraft's x-axis, defaults to inches.
- \b y - Location of tank on aircraft's y-axis, defaults to inches.
- \b z - Location of tank on aircraft's z-axis, defaults to inches.

drain_location:
- \b x - Location of tank drain on aircraft's x-axis, defaults to inches.
- \b y - Location of tank drain on aircraft's y-axis, defaults to inches.
- \b z - Location of tank drain on aircraft's z-axis, defaults to inches.

<h3>Default values of the tank configuration file parameters:</h3>

- \b type - ttUNKNOWN  (causes a load error in the propulsion configuration)
- \b location, \b drain_location - both optional, but a warning message will
be printed to the console if the location is not given
- \b x - 0.0  (both full and drained CG locations)
- \b y - 0.0  (both full and drained CG locations)
- \b z - 0.0  (both full and drained CG locations)
- \b radius - 0.0
- \b capacity - 0.0
- \b contents - 0.0
- \b temperature - -9999.0
- \b standpipe - 0.0

    @author Jon Berndt, Dave Culp
    @see Akbar, Raza et al. "A Simple Analysis of Fuel Addition to the CWT of
    747", California Institute of Technology, 1998,
    http://www.galcit.caltech.edu/EDL/projects/JetA/reports/lumped.pdf
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTank : public FGJSBBase
{
public:
  /** Constructor.
      The constructor reads in the defining parameters from a configuration file.
      @param exec a pointer to the base FGFDMExec instance.
      @param el a pointer to the Tank element.
      @param tank_number the tank number (zero based).
  */
  FGTank(FGFDMExec* exec, Element* el, int tank_number);
  /// Destructor
  ~FGTank();

  /** Removes fuel from the tank.
      This function removes fuel from a tank. If the tank empties, it is
      deselected.
      @param used the amount of fuel used in lbs.
      @return the remaining contents of the tank in lbs.
  */
  double Drain(double used);

  /** Performs local, tanks-specific calculations, such as fuel temperature.
      This function calculates the temperature of the fuel in the tank.
      @param dt the time step for this model.
      @return the current temperature in degrees Celsius.
  */
  double Calculate(double dt);

  /** Retrieves the type of tank: Fuel or Oxidizer.
      @return the tank type, 0 for undefined, 1 for fuel, and 2 for oxidizer.
  */
  int GetType(void) {return Type;}

  /** Resets the tank parameters to the initial conditions */
  void ResetToIC(void);

  /** If the tank is supplying fuel, this function returns true.
      @return true if this tank is feeding an engine.*/
  bool GetSelected(void) {return Selected;}

  /** Gets the tank fill level.
      @return the fill level in percent, from 0 to 100.*/
  double GetPctFull(void) {return PctFull;}

  /** Gets the capacity of the tank.
      @return the capacity of the tank in pounds. */
  double GetCapacity(void) {return Capacity;}

  /** Gets the contents of the tank.
      @return the contents of the tank in pounds. */
  double GetContents(void) const {return Contents;}

  /** Gets the temperature of the fuel.
      The temperature of the fuel is calculated if an initial tempearture is
      given in the configuration file. 
      @return the temperature of the fuel in degrees C IF an initial temperature
      is given, otherwise 0.0 C is returned. */
  double GetTemperature_degC(void) {return Temperature;}

  /** Gets the temperature of the fuel.
      The temperature of the fuel is calculated if an initial tempearture is
      given in the configuration file. 
      @return the temperature of the fuel in degrees F IF an initial temperature
      is given, otherwise 32 degrees F is returned. */
  double GetTemperature(void) {return CelsiusToFahrenheit(Temperature);}

  double GetIxx(void) {return Ixx;}
  double GetIyy(void) {return Iyy;}
  double GetIzz(void) {return Izz;}

  double GetStandpipe(void) {return Standpipe;}

  const FGColumnVector3 GetXYZ(void);
  const double GetXYZ(int idx);

  double Fill(double amount);
  void SetContents(double amount);
  void SetTemperature(double temp) { Temperature = temp; }
  void SetStandpipe(double amount) { Standpipe = amount; }

  enum TankType {ttUNKNOWN, ttFUEL, ttOXIDIZER};
  enum GrainType {gtUNKNOWN, gtCYLINDRICAL, gtENDBURNING};

private:
  TankType Type;
  GrainType grainType;
  int TankNumber;
  string type;
  string strGType;
  FGColumnVector3 vXYZ;
  FGColumnVector3 vXYZ_drain;
  double Capacity;
  double Radius;
  double InnerRadius;
  double Length;
  double Volume;
  double Density;
  double Ixx;
  double Iyy;
  double Izz;
  double PctFull;
  double Contents, InitialContents;
  double Area;
  double Temperature, InitialTemperature;
  double Standpipe, InitialStandpipe;
  bool  Selected;
  FGFDMExec* Exec;
  FGPropertyManager* PropertyManager;
  void CalculateInertias(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

