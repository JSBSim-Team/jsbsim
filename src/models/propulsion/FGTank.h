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

#include <FGJSBBase.h>
#include <input_output/FGXMLElement.h>
#include <math/FGColumnVector3.h>
// #include <FGAuxiliary.h>

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_STRING
  SG_USING_STD(string);
  SG_USING_STD(cerr);
  SG_USING_STD(endl);
  SG_USING_STD(cout);
#else
# include <string>
  using std::string;
# if !defined(sgi) || defined(__GNUC__) || (_COMPILER_VERSION >= 740)
   using std::cerr;
   using std::endl;
   using std::cout;
# endif
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TANK "$Id: FGTank.h,v 1.7 2008/04/29 04:38:48 jberndt Exp $"

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
  <location unit="{FT | M}">
    <x> {number} </x>
    <y> {number} </y>
    <z> {number} </z>
  </location>
  <radius unit="{FT | M}"> {number} </radius>
  <capacity unit="{LBS | KG}"> {number} </capacity>
  <contents unit="{LBS | KG}"> {number} </contents>
  <temperature> {number} </temperature> <!-- must be degrees fahrenheit -->
  <standpipe unit="{LBS | KG"}> {number} </standpipe>
</tank>
@endcode

<h3>Definition of the tank configuration file parameters:</h3>

- \b type - One of FUEL or OXIDIZER.  This is required.
- \b x - Location of tank on aircraft's x-axis, defaults to inches.
- \b y - Location of tank on aircraft's y-axis, defaults to inches.
- \b z - Location of tank on aircraft's z-axis, defaults to inches.
- \b radius - Equivalent radius of tank for modeling slosh, defaults to inches.
- \b capacity - Capacity, defaults to pounds.
- \b contents - Initial contents, defaults to pounds.
- \b temperature - Initial temperature, defaults to degrees Fahrenheit.
- \b standpipe - Minimum contents to which tank can dump, defaults to pounds.

<h3>Default values of the tank configuration file parameters:</h3>

- \b type - ttUNKNOWN  (causes a load error in the propulsion configuration)
- \b location - optional, but a warning message will be printed to console
- \b x - 0.0  
- \b y - 0.0
- \b z - 0.0
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

  double Drain(double);
  double Calculate(double dt);
  int GetType(void) {return Type;}
  bool GetSelected(void) {return Selected;}
  double GetPctFull(void) {return PctFull;}
  double GetContents(void) const {return Contents;}
  double GetTemperature_degC(void) {return Temperature;}
  double GetTemperature(void) {return CelsiusToFahrenheit(Temperature);}
  double GetStandpipe(void) {return Standpipe;}
  const FGColumnVector3& GetXYZ(void) {return vXYZ;}
  double GetXYZ(int idx) {return vXYZ(idx);}

  double Fill(double amount);
  void SetContents(double amount);
  void SetTemperature(double temp) { Temperature = temp; }
  void SetStandpipe(double amount) { Standpipe = amount; }

  enum TankType {ttUNKNOWN, ttFUEL, ttOXIDIZER};

private:
  TankType Type;
  int TankNumber;
  string type;
  FGColumnVector3 vXYZ;
  double Capacity;
  double Radius;
  double PctFull;
  double Contents;
  double Area;
  double Temperature;  
  double Standpipe;    
  bool  Selected;
  FGAuxiliary* Auxiliary;
  FGPropertyManager* PropertyManager;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

