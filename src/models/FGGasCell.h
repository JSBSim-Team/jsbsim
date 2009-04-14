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

#include <string>
using std::string;
using std::cerr;
using std::endl;
using std::cout;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_GASCELL "$Id: FGGasCell.h,v 1.8 2009/04/14 18:48:14 andgi Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGBallonet;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a gas cell.
    @author Anders Gidenstam

<h3>Configuration File Format:</h3>
@code
<buoyant_forces>
  <gas_cell type="{HYDROGEN | HELIUM | AIR}">
    <location unit="{M | IN}">
      <x> {number} </x>
      <y> {number} </y>
      <z> {number} </z>
    </location>
    <x_width unit="{M | IN}"> {number} </x_width>
    <y_radius unit="{M | IN}"> {number} </y_radius>
    <z_radius unit="{M | IN}"> {number} </z_radius>
    <max_overpressure unit="{PA | PSI}"> {number} </max_overpressure>
    <valve_coefficient unit="{M4*SEC/KG | FT4*SEC/SLUG}"> {number} </valve_coefficient>
    <fullness> {number} </fullness>  
    <heat>
      {heat transfer coefficients} [lbs ft / sec]
    </heat>
    <ballonet>
      <location unit="{M | IN}">
        <x> {number} </x>
        <y> {number} </y>
        <z> {number} </z>
      </location>
      <x_width unit="{M | IN}"> {number} </x_width>
      <y_radius unit="{M | IN}"> {number} </y_radius>
      <z_radius unit="{M | IN}"> {number} </z_radius>
      <max_overpressure unit="{PA | PSI}"> {number} </max_overpressure>
      <valve_coefficient unit="{M4*SEC/KG | FT4*SEC/SLUG}"> {number} </valve_coefficient>
      <fullness> {number} </fullness>  
      <heat>
       {heat transfer coefficients} [lb ft / (sec Rankine)]
      </heat>
      <blower_input>
       {input air flow function} [ft^3 / sec]
      </blower_input>
    </ballonet>
  </gas_cell>
</buoyant_forces>
@endcode

Definition of the gas cell configuration file parameters:
- <b>type</b> -
    One of HYDROGEN, HELIUM or AIR.
- <b>location</b> -
    Location of cell center in the aircraft's structural frame.
    Currently this is were the forces of the cell is applied.
- <b>{x|y|z}_radius</b> -
    Radius along in the respective direction (both ends).
- <b>{x|y|z}_width</b> -
    Width in the respective direction.
    <b>NOTE:</b> A 'x', 'y', 'z'-radius/width combination must be specified.
- <b>fullness</b> -
    Initial fullness of the cell, normally [0,1],
    values >1 initialize the cell at pressure.
- <b>max_overpressure</b> -
    Maximum cell overpressure (excess is automatically valved off).
- <b>valve_coefficient</b> -
    Capacity of the manual valve. The valve is
    considered to be located at the top of the cell.
    The valve coefficient determine the flow out
    of the cell according to:
    <i>dVolume/dt = ValveCoefficient * DeltaPressure</i>.
- <b>heat</b> -
    Zero or more FGFunction:s describing the heat flow from
    the atmosphere into the gas cell.
    Unit: [lb ft / (sec Rankine)].
    If there are no heat transfer functions at all the gas cell
    temperature will equal that of the surrounding atmosphere.
    A constant function returning 0 results in adiabatic behaviour.
- <b>ballonet</b> -
    Zero or more ballonets, i.e. air bags inside the gas cell.
    Ballonets are used to maintain the volume of the gas cell
    and keep its internal pressure higher than that of the
    surrounding environment.
  - <b>location</b> -
      Location of ballonet center in the aircraft's structural frame.
  - <b>{x|y|z}_radius</b> -
      Radius along in the respective direction (both ends).
  - <b>{x|y|z}_width</b> -
      Width in the respective direction.
  - <b>max_overpressure</b> -
      Maximum ballonet overpressure (excess is automatically valved off).
  - <b>valve_coefficient</b> -
      Capacity of the exit valve between the ballonet
      and the atmosphere. The valve coefficient
      determine the flow out of the cell according to:
      <i>dVolume/dt = ValveCoefficient * DeltaPressure</i>.
  - <b>heat</b> -
      Zero or more FGFunction:s describing the heat flow from
      the enclosing gas cell into the ballonet.
      Unit: [lb ft / (sec Rankine)]
  - <b>blower_input</b> -
      One FGFunction describing the air flow into the
      ballonet. Unit: [ft<sup>3</sup> / sec] (at the temperature and
      pressure of the ballonet.)
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
class FGGasCell : public FGForce
{
public:
  /** Constructor
      @param exec Executive a pointer to the parent executive object
      @param el   Pointer to configuration file XML node
      @param num  Gas cell index number. */
  FGGasCell(FGFDMExec* exec, Element* el, int num);
  ~FGGasCell();

  /** Runs the gas cell model; called by BuoyantForces
   */
  void Calculate(double dt);

  /** Get the index of this gas cell
      @return gas cell index. */
  int GetIndex(void) const {return CellNum;}

  /** Get the center of gravity location of the gas cell
      (including any ballonets)
      @return CoG location in the structural frame. */
  const FGColumnVector3& GetXYZ(void) const {return vXYZ;}

  /** Get the center of gravity location of the gas cell
      (including any ballonets)
      @return CoG location in the structural frame. */
  double GetXYZ(int idx) const {return vXYZ(idx);}

  /** Get the current mass of the gas cell (including any ballonets)
      @return gas mass in slug. */
  double GetMass(void) const {return Mass;}

  /** Get the moments of inertia of the gas cell (including any ballonets)
      @return moments of inertia matrix relative the gas cell location
      in slug ft<sup>2</sup>. */
  const FGMatrix33& GetInertia(void) const {return gasCellJ;}

  /** Get the moment due to mass of the gas cell (including any ballonets)

      Note that the buoyancy of the gas cell is handled separately by the
      FGForce part and not included here.
      @return moment vector in lbs ft. */
  const FGColumnVector3& GetMassMoment(void) const {return gasCellM;}

  /** Get the current gas temperature inside the gas cell
      @return gas temperature in Rankine. */
  double GetTemperature(void) const {return Temperature;}

  /** Get the current gas pressure inside the gas cell
      @return gas pressure in lbs / ft<sup>2</sup>. */
  double GetPressure(void) const {return Pressure;}

private:

  enum GasType {ttUNKNOWN, ttHYDROGEN, ttHELIUM, ttAIR};

  GasType Type;
  string type;
  int CellNum;
  // Structural constants
  double MaxVolume;                 // [ft�]
  double MaxOverpressure;           // [lbs/ft�]
  FGColumnVector3 vXYZ;             // [in]
  double Xradius, Yradius, Zradius; // [ft]
  double Xwidth, Ywidth, Zwidth;    // [ft]
  double ValveCoefficient;          // [ft^4 sec / slug]
  typedef vector <FGFunction*> CoeffArray;
  CoeffArray HeatTransferCoeff;
  typedef vector <FGBallonet*> BallonetArray;
  BallonetArray Ballonet;
  // Variables
  double Pressure;          // [lbs/ft�]
  double Contents;          // [mol]
  double Volume;            // [ft�]
  double dVolumeIdeal;      // [ft�]
  double Temperature;       // [Rankine]
  double Buoyancy;          // [lbs] Note: Gross lift.
                            // Does not include the weight of the gas itself.
  double ValveOpen;         // 0 <= ValveOpen <= 1 (or higher).
  double Mass;              // [slug]
  FGMatrix33 gasCellJ;      // [slug foot�]
  FGColumnVector3 gasCellM; // [lbs ft]

  FGAuxiliary* Auxiliary;
  FGAtmosphere* Atmosphere;
  FGPropertyManager* PropertyManager;
  FGInertial* Inertial;
  FGMassBalance* MassBalance;
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

  double Cv_gas() {               // [??]
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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/** Models a ballonet inside a gas cell.
    Models a ballonet inside a gas cell.
    Not intended to be used outside FGGasCell.
    See FGGasCell for the configuration file format.
    @author Anders Gidenstam
*/
class FGBallonet : public FGJSBBase
{
public:
  FGBallonet(FGFDMExec* exec, Element* el, int num, FGGasCell* parent);
  ~FGBallonet();

  /** Runs the ballonet model; called by FGGasCell
   */
  void Calculate(double dt);


  /** Get the center of gravity location of the ballonet
      @return CoG location in the structural frame. */
  const FGColumnVector3& GetXYZ(void) const {return vXYZ;}
  /** Get the center of gravity location of the ballonet
      @return CoG location in the structural frame. */
  double GetXYZ(int idx) const {return vXYZ(idx);}

  /** Get the current mass of the ballonets
      @return mass in slug. */
  double GetMass(void) const {return Contents * M_air;}

  /** Get the moments of inertia of the ballonet
      @return moments of inertia matrix in slug ft<sup>2</sup>. */
  const FGMatrix33& GetInertia(void) const {return ballonetJ;}

  /** Get the current volume of the ballonet
      @return volume in ft<sup>3</sup>. */
  double GetVolume(void) const {return Volume;}
  /** Get the current heat flow into the ballonet
      @return heat flow in lbs ft / sec. */
  double GetHeatFlow(void) const {return dU;}       // [lbs ft / sec]

private:
  int CellNum;
  // Structural constants
  double MaxVolume;                 // [ft�]
  double MaxOverpressure;           // [lbs/ft�]
  FGColumnVector3 vXYZ;             // [in]
  double Xradius, Yradius, Zradius; // [ft]
  double Xwidth, Ywidth, Zwidth;    // [ft]
  double ValveCoefficient;          // [ft^4 sec / slug]
  typedef vector <FGFunction*> CoeffArray;
  CoeffArray HeatTransferCoeff;     // [lbs ft / sec]
  FGFunction* BlowerInput;          // [ft^3 / sec]
  FGGasCell* Parent;
  // Variables
  double Pressure;         // [lbs/ft�]
  double Contents;         // [mol]
  double Volume;           // [ft�]
  double dVolumeIdeal;     // [ft�]
  double dU;               // [lbs ft / sec]
  double Temperature;      // [Rankine]
  double ValveOpen;        // 0 <= ValveOpen <= 1 (or higher).
  FGMatrix33 ballonetJ;     // [slug foot�]

  FGAuxiliary* Auxiliary;
  FGAtmosphere* Atmosphere;
  FGPropertyManager* PropertyManager;
  FGInertial* Inertial;
  void Debug(int from);

  /* Constants. */
  const static double R;          // [lbs ft/(mol Rankine)]
  const static double M_air;      // [slug/mol]
  const static double Cv_air;     // [??]
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
