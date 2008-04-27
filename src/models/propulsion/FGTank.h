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

#define ID_TANK "$Id: FGTank.h,v 1.6 2008/04/27 13:56:44 dpculp Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a fuel tank.
    @author Jon Berndt
    @see Akbar, Raza et al. "A Simple Analysis of Fuel Addition to the CWT of
    747", California Institute of Technology, 1998
<P>
    Fuel temperature is calculated using the following assumptions:
<P>
    Fuel temperature will only be calculated for tanks which have an initial fuel
    temperature specified in the configuration file.
<P>
    The surface area of the tank is estimated from the capacity in pounds.  It
    is assumed that the tank is a wing tank with dimensions h by 4h by 10h. The
    volume of the tank is then 40(h)(h)(h). The area of the upper or lower 
    surface is then 40(h)(h).  The volume is also equal to the capacity divided
    by 49.368 lbs/cu-ft, for jet fuel.  The surface area of one side can then be
    derived from the tank's capacity.  
<P>
    The heat capacity of jet fuel is assumed to be 900 Joules/lbm/K, and the 
    heat transfer factor of the tank is 1.115 Watts/sq-ft/K.
<P>
Configuration File Format
<pre>
\<AC_TANK TYPE="\<FUEL | OXIDIZER>" NUMBER="\<n>">
  XLOC        \<x location>
  YLOC        \<y location>
  ZLOC        \<z location>
  RADIUS      \<radius>
  CAPACITY    \<capacity>
  CONTENTS    \<contents>
  TEMPERATURE \<fuel temperature>
  STANDPIPE   \<standpipe>
\</AC_TANK>
</pre>
Definition of the tank configuration file parameters:
<pre>
<b>TYPE</b> - One of FUEL or OXIDIZER.
<b>XLOC</b> - Location of tank on aircraft's x-axis, inches.
<b>YLOC</b> - Location of tank on aircraft's y-axis, inches.
<b>ZLOC</b> - Location of tank on aircraft's z-axis, inches.
<b>RADIUS</b> - Equivalent radius of tank, inches, for modeling slosh.
<b>CAPACITY</b> - Capacity in pounds.
<b>CONTENTS</b> - Initial contents in pounds.
<b>TEMPERATURE</b> - Initial temperature in degrees Fahrenheit.
<b>STANDPIPE<b> - Minimum contents (pounds) to which tank can dump.
</pre>
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTank : public FGJSBBase
{
public:
  FGTank(FGFDMExec* exec, Element* el, int tank_number);
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

