/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGState.h
 Author:       Jon S. Berndt
 Date started: 11/17/98

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

HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSTATE_H
#define FGSTATE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <fstream>
#include <string>
#include <map>
#include "FGJSBBase.h"
#include <initialization/FGInitialCondition.h>
#include <math/FGColumnVector3.h>
#include <math/FGQuaternion.h>
#include "FGFDMExec.h"
#include <models/FGAtmosphere.h>
#include <models/FGFCS.h>
#include <models/FGPropagate.h>
#include <models/FGAuxiliary.h>
#include <models/FGAerodynamics.h>
#include <models/FGAircraft.h>
#include <models/FGGroundReactions.h>
#include <models/FGPropulsion.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STATE "$Id: FGState.h,v 1.13 2008/07/22 02:42:17 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the calculation of aircraft state.
    <h3>Properties</h3>
    @property sim-time-sec (read only) cumulative simulation in seconds.
    @author Jon S. Berndt
    @version $Revision: 1.13 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGState : public FGJSBBase
{
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGState(FGFDMExec*);
  /// Destructor
  ~FGState();

  /** Initializes the simulation state based on parameters from an Initial Conditions object.
      @param FGIC pointer to an initial conditions object.
      @see FGInitialConditions.    */
  void Initialize(FGInitialCondition *FGIC);

  /// Returns the cumulative simulation time in seconds.
  inline double Getsim_time(void) const { return sim_time; }

  /// Returns the simulation delta T.
  inline double Getdt(void) {return dt;}

  /// Suspends the simulation and sets the delta T to zero.
  inline void SuspendIntegration(void) {saved_dt = dt; dt = 0.0;}

  /// Resumes the simulation by resetting delta T to the correct value.
  inline void ResumeIntegration(void)  {dt = saved_dt;}

  /** Returns the simulation suspension state.
      @return true if suspended, false if executing  */
  bool IntegrationSuspended(void) {return dt == 0.0;}

  /** Sets the current sim time.
      @param cur_time the current time
      @return the current simulation time.      */
  inline double Setsim_time(double cur_time) {
    sim_time = cur_time;
    return sim_time;
  }

  /** Sets the integration time step for the simulation executive.
      @param delta_t the time step in seconds.     */
  inline void  Setdt(double delta_t) { dt = delta_t; }

  /** Increments the simulation time.
      @return the new simulation time.     */
  inline double IncrTime(void) {
    sim_time+=dt;
    return sim_time;
  }

  /** Prints a summary of simulator state (speed, altitude,
      configuration, etc.)  */
//  void ReportState(void);

private:
  double sim_time, dt;
  double saved_dt;

  FGFDMExec* FDMExec;

  FGAircraft* Aircraft;
  FGPropagate* Propagate;
  FGAtmosphere* Atmosphere;
  FGFCS* FCS;
  FGAerodynamics* Aerodynamics;
  FGGroundReactions* GroundReactions;
  FGPropulsion* Propulsion;
  FGAuxiliary* Auxiliary;
  FGPropertyManager* PropertyManager;

  void bind();

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


#endif

