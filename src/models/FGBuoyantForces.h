/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGBuoyantForces.h
 Author:       Anders Gidenstam, Jon S. Berndt
 Date started: 01/21/08

 ------------- Copyright (C) 2008 - 2011  Anders Gidenstam        -------------
 ------------- Copyright (C) 2008  Jon S. Berndt (jon@jsbsim.org) -------------

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
01/21/08   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGBUOYANTFORCES_H
#define FGBUOYANTFORCES_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>
#include <map>

#include "FGModel.h"
#include "FGGasCell.h"
#include "math/FGColumnVector3.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the Buoyant forces calculations.
    This class owns and contains the list of force/coefficients that define the
    Buoyant properties of an air vehicle.
    
    Here's an example of a gas cell specification:

    @code
    <buoyant_forces>

      <!-- Interface properties -->
      <property>ballonets/in-flow-ft3ps[0]</property>

      <gas_cell type="HYDROGEN">
        <location unit="M">
          <x> 18.8 </x>
          <y> 0.0 </y>
          <z> 0.0 </z>
        </location>
        <x_radius unit="M"> 22.86 </x_radius>
        <y_radius unit="M">  4.55 </y_radius>
        <z_radius unit="M">  4.55 </z_radius>
        <max_overpressure unit="PA"> 340.0 </max_overpressure> 
        <valve_coefficient unit="M4*SEC/KG"> 0.015 </valve_coefficient>
      </gas_cell>
      
      ... {other gas cells} ...
      
    </buoyant_forces>
    @endcode

    See FGGasCell for the full configuration file format for gas cells.

    @author Anders Gidenstam, Jon S. Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGBuoyantForces : public FGModel
{

public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGBuoyantForces(FGFDMExec* Executive);
  /// Destructor
  ~FGBuoyantForces() override;

  bool InitModel(void) override;

  /** Runs the Buoyant forces model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;

  /** Loads the Buoyant forces model.
      The Load function for this class expects the XML parser to
      have found the Buoyant_forces keyword in the configuration file.
      @param element pointer to the current XML element for Buoyant forces parameters.
      @return true if successful */
  bool Load(Element* element) override;

  /** Gets the total Buoyant force vector.
      @return a force vector in lbs. */
  const FGColumnVector3& GetForces(void) const {return vTotalForces;}

  /** Gets a component of the total Buoyant force vector.
      @return a component of the force vector in lbs. */
  double GetForces(int idx) const {return vTotalForces(idx);}

  /** Gets the total Buoyancy moment vector.
      @return a moment vector in the body frame in lbs ft. */
  const FGColumnVector3& GetMoments(void) const {return vTotalMoments;}

  /** Gets a component of the total Buoyancy moment vector.
      @return a component of the moment vector in the body frame in lbs ft. */
  double GetMoments(int idx) const {return vTotalMoments(idx);}

  /** Gets the total gas mass. The gas mass is part of the aircraft's
      inertia.
      @return mass in slugs. */
  double GetGasMass(void) const;

  /** Gets the total moment from the gas mass.
      @return a moment vector in the structural frame in lbs in. */
  const FGColumnVector3& GetGasMassMoment(void);

  /** Gets the total moments of inertia for the gas mass in the body frame.
      @return moments of inertia matrix in the body frame
      in slug ft<sup>2</sup>. */
  const FGMatrix33& GetGasMassInertia(void);

  /** Gets the strings for the current set of gas cells.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the descriptive names for all parameters */
  std::string GetBuoyancyStrings(const std::string& delimeter);

  /** Gets the coefficient values.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the numeric values for the current set of
      parameters */
  std::string GetBuoyancyValues(const std::string& delimeter);

  FGGasCell::Inputs in;

private:
  std::vector <FGGasCell*> Cells;
  // Buoyant forces and moments. Excluding the gas weight.
  FGColumnVector3 vTotalForces;  // [lbs]
  FGColumnVector3 vTotalMoments; // [lbs ft]

  // Gas mass related masses, inertias and moments.
  FGMatrix33 gasCellJ;             // [slug ft^2]
  FGColumnVector3 vGasCellXYZ;
  FGColumnVector3 vXYZgasCell_arm; // [lbs in]

  bool NoneDefined;

  void bind(void);

  void Debug(int from) override;
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
