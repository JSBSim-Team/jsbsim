/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGState.h
 Author:       Jon S. Berndt
 Date started: 11/17/98

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

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
#else
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <fstream.h>
#  else
#    include <fstream>
#  endif
#endif

#include <string>
#include <map>
#include "FGJSBBase.h"
#include <initialization/FGInitialCondition.h>
#include <math/FGMatrix33.h>
#include <math/FGColumnVector3.h>
#include <math/FGQuaternion.h>
#include "FGFDMExec.h"
#include <models/FGAtmosphere.h>
#include <models/FGFCS.h>
#include <models/FGPropagate.h>
#include <models/FGAuxiliary.h>
#include <models/FGAerodynamics.h>
#include <models/FGOutput.h>
#include <models/FGAircraft.h>
#include <models/FGGroundReactions.h>
#include <models/FGPropulsion.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STATE "$Id: FGState.h,v 1.3 2005/06/13 16:59:16 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the calculation of aircraft state.
    @author Jon S. Berndt
    @version $Id: FGState.h,v 1.3 2005/06/13 16:59:16 ehofman Exp $
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
      @see FGInitialConditions.
      */
  void Initialize(FGInitialCondition *FGIC);

  /// Returns the simulation time in seconds.
  inline double Getsim_time(void) const { return sim_time; }
  /// Returns the simulation delta T.
  inline double Getdt(void) {
    return dt;
  }

  /// Suspends the simulation and sets the delta T to zero.
  inline void Suspend(void) {saved_dt = dt; dt = 0.0;}
  /// Resumes the simulation by resetting delta T to the correct value.
  inline void Resume(void)  {dt = saved_dt;}

  /** Sets the current sim time.
      @param cur_time the current time
      @return the current time.
      */
  inline double Setsim_time(double cur_time) {
    sim_time = cur_time;
    return sim_time;
  }

  /** Sets the integration time step for the simulation executive.
      @param delta_t the time step in seconds.
      */
  inline void  Setdt(double delta_t) {
    dt = delta_t;
  }

  /** Increments the simulation time.
      @return the new simulation time.
      */
  inline double IncrTime(void) {
    sim_time+=dt;
    return sim_time;
  }

  /** Calculates and returns the stability-to-body axis transformation matrix.
      @return a reference to the stability-to-body transformation matrix.
      */
  FGMatrix33& GetTs2b(void);

  /** Calculates and returns the body-to-stability axis transformation matrix.
      @return a reference to the stability-to-body transformation matrix.
      */
  FGMatrix33& GetTb2s(void);

  /** Prints a summary of simulator state (speed, altitude,
      configuration, etc.)
  */
  void ReportState(void);

  void bind();
  void unbind();

private:
  double sim_time, dt;
  double saved_dt;

  FGFDMExec* FDMExec;
  FGMatrix33 mTs2b;
  FGMatrix33 mTb2s;

  FGAircraft* Aircraft;
  FGPropagate* Propagate;
  FGOutput* Output;
  FGAtmosphere* Atmosphere;
  FGFCS* FCS;
  FGAerodynamics* Aerodynamics;
  FGGroundReactions* GroundReactions;
  FGPropulsion* Propulsion;
  FGAuxiliary* Auxiliary;
  FGPropertyManager* PropertyManager;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


#endif

