/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGOutputType.h
 Author:       Bertrand Coconnier
 Date started: 09/10/11

 ------------- Copyright (C) 2011 Bertrand Coconnier -------------

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
09/10/11   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGOUTPUTTYPE_H
#define FGOUTPUTTYPE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "models/FGModel.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFDMExec;
class Element;
class FGAerodynamics;
class FGAuxiliary;
class FGAircraft;
class FGAtmosphere;
class FGWinds;
class FGPropulsion;
class FGMassBalance;
class FGPropagate;
class FGAccelerations;
class FGFCS;
class FGGroundReactions;
class FGExternalReactions;
class FGBuoyantForces;
class FGPropertyValue;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Abstract class to provide functions generic to all the output directives.
    This class is used by the output manager FGOutput to manage a list of
    different output classes without needing to know the details of each one of
    them. It also provides the functions that are common to all the output
    classes.

    The class inherits from FGModelFunctions so it is possible to define
    functions that execute before or after the output is generated. Such
    functions need to be tagged with a "pre" or "post" type attribute to denote
    the sequence in which they should be executed.

    The class mimics some functionalities of FGModel (methods InitModel(),
    Run() and SetRate()). However it does not inherit from FGModel since it is
    conceptually different from the model paradigm.
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGOutputType : public FGModel
{
public:
  /** Constructor (implement the FGModel interface).
      @param fdmex a pointer to the parent executive object
   */
  FGOutputType(FGFDMExec* fdmex);

  /// Destructor
  ~FGOutputType() override;

  /** Set the idx for this output instance
      @param idx ID of the output instance that is constructed
   */
  void SetIdx(unsigned int idx);

  /** Set the output rate for this output instances.
      @param rtHz new output rate in Hz */
  void SetRateHz(double rtHz);

  /// Get the output rate in Hz for this output.
  double GetRateHz(void) const;

  /** Set the activated subsystems for this output instance.
      @param subSystems bitfield that describes the activated subsystems
      @param outputProperties list of properties that should be output
  */
  void SetSubSystems(int subSystems) { SubSystems = subSystems; }

  /** Set the list of properties that should be output for this output instance.
      @param outputProperties list of properties that should be output
  */
  void SetOutputProperties(std::vector<FGPropertyNode_ptr> & outputProperties);

  /** Overwrites the name identifier under which the output will be logged.
      This method is taken into account if it is called before
      FGFDMExec::RunIC() otherwise it is ignored until the next call to
      SetStartNewOutput().
      @param name new name */
  virtual void SetOutputName(const std::string& name) { Name = name; }

  /** Get the name identifier to which the output will be directed.
      @result the name identifier.*/
  virtual const std::string& GetOutputName(void) const { return Name; }

  /** Init the output directives from an XML file (implement the FGModel interface).
      @param element XML Element that is pointing to the output directives
  */
  bool Load(Element* el) override;

  /// Init the output model according to its configitation.
  bool InitModel(void) override;

  /** Executes the output directives (implement the FGModel interface).
      This method checks that the current time step matches the output
      rate and calls the registered "pre" functions, the output
      generation and finally the "post" functions.
      @result false if no error.
   */
  bool Run(void);

  /** Generate the output. This is a pure method so it must be implemented by
      the classes that inherits from FGOutputType. The Print name may not be
      relevant to all outputs but it has been kept for backward compatibility.
   */
  virtual void Print(void) = 0;

  /** Reset the output prior to a restart of the simulation. This method should
      be called when the simulation is restarted with, for example, new initial
      conditions. When this method is executed the output instance can take
      special actions such as closing the current output file and open a new
      one with a different name. */

  virtual void SetStartNewOutput(void) {}

  /// Enables the output generation.
  void Enable(void) { enabled = true; }
  /// Disables the output generation.
  void Disable(void) { enabled = false; }
  /** Toggles the output generation.
      @result the output generation status i.e. true if the output has been
              enabled, false if the output has been disabled. */
  bool Toggle(void) {enabled = !enabled; return enabled;}

  /// Subsystem types for specifying which will be output in the FDM data logging
  enum  eSubSystems {
    /** Subsystem: Simulation (= 1)          */ ssSimulation      = 1,
    /** Subsystem: Aerosurfaces (= 2)        */ ssAerosurfaces    = 2,
    /** Subsystem: Body rates (= 4)          */ ssRates           = 4,
    /** Subsystem: Velocities (= 8)          */ ssVelocities      = 8,
    /** Subsystem: Forces (= 16)             */ ssForces          = 16,
    /** Subsystem: Moments (= 32)            */ ssMoments         = 32,
    /** Subsystem: Atmosphere (= 64)         */ ssAtmosphere      = 64,
    /** Subsystem: Mass Properties (= 128)   */ ssMassProps       = 128,
    /** Subsystem: Coefficients (= 256)      */ ssAeroFunctions   = 256,
    /** Subsystem: Propagate (= 512)         */ ssPropagate       = 512,
    /** Subsystem: Ground Reactions (= 1024) */ ssGroundReactions = 1024,
    /** Subsystem: FCS (= 2048)              */ ssFCS             = 2048,
    /** Subsystem: Propulsion (= 4096)       */ ssPropulsion      = 4096
  } subsystems;

protected:
  unsigned int OutputIdx;
  int SubSystems;
  std::vector <FGPropertyValue*> OutputParameters;
  std::vector <std::string> OutputCaptions;
  bool enabled;

  FGAerodynamics* Aerodynamics;
  FGAuxiliary* Auxiliary;
  FGAircraft* Aircraft;
  FGAtmosphere* Atmosphere;
  FGWinds* Winds;
  FGPropulsion* Propulsion;
  FGMassBalance* MassBalance;
  FGPropagate* Propagate;
  FGAccelerations* Accelerations;
  FGFCS* FCS;
  FGGroundReactions* GroundReactions;
  FGExternalReactions* ExternalReactions;
  FGBuoyantForces* BuoyantForces;

  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
