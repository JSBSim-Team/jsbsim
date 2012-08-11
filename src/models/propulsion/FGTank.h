/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTank.h
 Author:       Jon S. Berndt
 Date started: 01/21/99

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

#include "FGJSBBase.h"
#include "math/FGColumnVector3.h"
#include <string>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TANK "$Id: FGTank.h,v 1.27 2012/08/11 14:57:38 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;
class FGPropertyManager;
class FGFDMExec;

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

    There is also a property <tt>propulsion/tank[i]/external-flow-rate-pps</tt>. Setting
    this property to a positive value causes the tank to fill at the rate specified.
    Setting a negative number causes the tank to drain. The value is the rate in pounds
    of fuel per second. The tank will not fill past 100% full and will not drain below 0%.
    Fuel may be transfered between two tanks by setting the source tank's external flow rate
    to a negative value and the destination's external flow rate to the same positive value.
    Care must be taken to stop fuel flow before the source tank becomes empty to prevent
    phantom fuel being created.

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
  <priority> {integer} </priority>
  <density unit="{KG/L | LBS/GAL}"> {number} </density>
  <type> {string} </type> <!-- will override previous density setting -->
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
- \b priority - Establishes feed sequence of tank. "1" is the highest priority.
- \b density - Density of liquid tank contents.
- \b type - Named fuel type. One of AVGAS, JET-A, JET-A1, JET-B, JP-1, JP-2, JP-3,
- \b        JP-4, JP-5, JP-6, JP-7, JP-8, JP-8+100, RP-1, T-1, ETHANOL, HYDRAZINE,
- \b        F-34, F-35, F-40, F-44, AVTAG, AVCAT

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
- \b capacity - 0.00001 (tank capacity must not be zero)
- \b contents - 0.0
- \b temperature - -9999.0 (flag which indicates no temperature is set)
- \b standpipe - 0.0 (all contents may be dumped)
- \b priority - 1 (highest feed sequence priority)
- \b density - 6.6

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

  enum TankType {ttUNKNOWN, ttFUEL, ttOXIDIZER};
  enum GrainType {gtUNKNOWN, gtCYLINDRICAL, gtENDBURNING};

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
      @param TempC the Total Air Temperature in degrees Celsius.
      @return the current temperature in degrees Celsius.
  */
  double Calculate(double dt, double TempC);

  /** Retrieves the type of tank: Fuel or Oxidizer.
      @return the tank type, 0 for undefined, 1 for fuel, and 2 for oxidizer.
  */
  int GetType(void) const {return Type;}

  /** Resets the tank parameters to the initial conditions */
  void ResetToIC(void);

  /** If the tank is set to supply fuel, this function returns true.
      @return true if this tank is set to a non-zero priority.*/
  bool GetSelected(void) const {return Selected;}

  /** Gets the tank fill level.
      @return the fill level in percent, from 0 to 100.*/
  double GetPctFull(void) const {return PctFull;}

  /** Gets the capacity of the tank.
      @return the capacity of the tank in pounds. */
  double GetCapacity(void) const {return Capacity;}

  /** Gets the capacity of the tank.
      @return the capacity of the tank in gallons. */
  double GetCapacityGallons(void) const {return Capacity/Density;}

  /** Gets the contents of the tank.
      @return the contents of the tank in pounds. */
  double GetContents(void) const {return Contents;}

  /** Gets the contents of the tank.
      @return the contents of the tank in gallons. */
  double GetContentsGallons(void) const {return Contents/Density;}

  /** Gets the temperature of the fuel.
      The temperature of the fuel is calculated if an initial tempearture is
      given in the configuration file. 
      @return the temperature of the fuel in degrees C IF an initial temperature
      is given, otherwise 0.0 C is returned. */
  double GetTemperature_degC(void) const {return Temperature;}

  /** Gets the temperature of the fuel.
      The temperature of the fuel is calculated if an initial tempearture is
      given in the configuration file. 
      @return the temperature of the fuel in degrees F IF an initial temperature
      is given, otherwise 32 degrees F is returned. */
  double GetTemperature(void) const {return CelsiusToFahrenheit(Temperature);}

  /** Returns the density of a named fuel type.
      @return the density, in lbs/gal, or 6.6 if name cannot be resolved. */
  double ProcessFuelName(const std::string& name); 

  double GetIxx(void) const {return Ixx;}
  double GetIyy(void) const {return Iyy;}
  double GetIzz(void) const {return Izz;}

  double GetStandpipe(void) const {return Standpipe;}

  int  GetPriority(void) const {return Priority;}
  void SetPriority(int p) { Priority = p; Selected = p>0 ? true:false; } 

  double GetDensity(void) const {return Density;}
  void   SetDensity(double d) { Density = d; }

  double GetExternalFlow(void) const {return ExternalFlow;}
  void   SetExternalFlow(double f) { ExternalFlow = f; }

  FGColumnVector3 GetXYZ(void) const;
  double GetXYZ(int idx) const;

  const GrainType GetGrainType(void) const {return grainType;}

  double Fill(double amount);
  void SetContents(double amount);
  void SetContentsGallons(double gallons);
  void SetTemperature(double temp) { Temperature = temp; }
  void SetStandpipe(double amount) { Standpipe = amount; }
  void SetSelected(bool sel) { sel==true ? SetPriority(1):SetPriority(0); }

private:
  TankType Type;
  GrainType grainType;
  int TankNumber;
  std::string type;
  std::string strGType;
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
  double InertiaFactor;
  double PctFull;
  double Contents, InitialContents;
  double PreviousUsed;
  double Area;
  double Temperature, InitialTemperature;
  double Standpipe, InitialStandpipe;
  double ExternalFlow;
  bool  Selected;
  int Priority, InitialPriority;
  FGFDMExec* Exec;
  FGPropertyManager* PropertyManager;

  void CalculateInertias(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

