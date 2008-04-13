/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGasCell.h
 Author:       Anders Gidenstam
 Date started: 01/21/2006

 ----- Copyright (C) 2006 - 2008  Anders Gidenstam (anders(at)gidenstam.org) --

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

This class simulates a generic gas cell for static buoyancy.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGGasCell_H
#define FGGasCell_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <FGJSBBase.h>
#include <input_output/FGXMLElement.h>
#include <math/FGColumnVector3.h>
#include <models/propulsion/FGForce.h>
#include <math/FGFunction.h>

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

#define ID_GASCELL "$Id: FGGasCell.h,v 1.4 2008/04/13 15:14:22 andgi Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a gas cell.
    @author Anders Gidenstam

Configuration File Format
@code
<buoyant_forces>
  <gas_cell type="{HYDROGEN | HELIUM | AIR}">
    <location unit="M">
      <x> ... </x>
      <y> ... </y>
      <z> ... </z>
    </location>

    <x_width unit="M"> ... </x_width>
    <y_radius unit="M"> ... </y_radius>
    <z_radius unit="M"> ... </z_radius>
    <max_overpressure unit="Pa"> 0.0 </max_overpressure>
    <valve_coefficient unit="M4*SEC/KG"> ... </valve_coefficient>

    <fullness> ... </fullness>  
    <heat>
      {heat transfer coefficients} [lb ft / (s R)]
    </heat>
  </gas_cell>
</buoyant_forces>
@endcode
Definition of the gas cell configuration file parameters:
- <b>type</b> - One of HYDROGEN, HELIUM or AIR.
- <b>location</b> - Location of cell center in the aircraft's structural frame.
                  Currently this is were the forces of the cell is applied.
- <b>{x|y|z}_radius</b> - Radius along in the respective direction (both ends).
- <b>{x|y|z}_width</b> - Width in the respective direction.
- <b>NOTE:</b> A 'x', 'y', 'z'-radius/width combination must be specified.
- <b>fullness</b> - Initial fullness of the cell, normally [0,1],
                  values >1 initialize the cell at pressure.
- <b>max_overpressure</b> - Maximum cell overpressure (excess is automatically
                          valved off).
- <b>valve_coefficient</b> - Capacity of the manual valve. The valve is
                           considered to be located at the top of the cell. 
- <b>NOTE:</b> The valve coefficient determine the gas flow out of the cell
according to: dVolume/dt = ValveCoefficient * DeltaPressure;
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGGasCell : public FGForce
{
public:
  FGGasCell(FGFDMExec* exec, Element* el, int num);
  ~FGGasCell();

  void Calculate(double dt);
  int GetType(void) {return Type;}
  const FGColumnVector3& GetXYZ(void) {return vXYZ;}
  double GetXYZ(int idx) {return vXYZ(idx);}

  double GetMass() {return Contents * M_gas();}   // [slug]
  FGMatrix33& GetInertia(void) {return gasCellJ;} // [slug foot²]

  enum GasType {ttUNKNOWN, ttHYDROGEN, ttHELIUM, ttAIR};

private:
  GasType Type;
  string type;
  int CellNum;
  // Structural constants
  double MaxVolume;                 // [ft³]
  double MaxOverpressure;           // [lbs/ft²]
  FGColumnVector3 vXYZ;             // [in]
  double Xradius, Yradius, Zradius; // [ft]
  double Xwidth, Ywidth, Zwidth;    // [ft]
  double ValveCoefficient;          // [ft^4 sec / slug]
  typedef vector <FGFunction*> CoeffArray;
  CoeffArray HeatTransferCoeff;
  // Variables
  double Pressure;         // [lbs/ft²]
  double Contents;         // [mol]
  double Volume;           // [ft³]
  double dVolumeIdeal;     // [ft³]
  double Temperature;      // [Rankine]
  double Buoyancy;         // [lbs] Note: Does not include the weight of the gas itself.
  double ValveOpen;        // 0 <= ValveOpen <= 1.
  FGMatrix33 gasCellJ;     // [slug foot²]

  FGAuxiliary* Auxiliary;
  FGAtmosphere* Atmosphere;
  FGPropertyManager* PropertyManager;
  FGInertial* Inertial;
  void Debug(int from);

  /* Constants. */
  const static double R;          // [lbs ft/(mol Rankine)]
  const static double M_air;      // [slug/mol]
  const static double M_hydrogen; // [slug/mol]
  const static double M_helium;   // [slug/mol]

  double M_gas() {                // [slug/mol]
    switch (Type) {
    case ttHYDROGEN:
      return M_hydrogen;
    case ttHELIUM:
      return M_helium;
    case ttAIR:
      return M_air;
    default:
      return M_air;
    }
  }

  double Cv_gas() {                             // [??]
    switch (Type) {
    case ttHYDROGEN:
      return 5.0/2.0;
    case ttHELIUM:
      return 3.0/2.0;
    case ttAIR:
      return 5.0/2.0;
    default:
      return 5.0/2.0;
    }
  }

};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
