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
#include "FGInitialCondition.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGQuaternion.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGPropagate.h"
#include "FGAuxiliary.h"
#include "FGAerodynamics.h"
#include "FGOutput.h"
#include "FGAircraft.h"
#include "FGGroundReactions.h"
#include "FGPropulsion.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STATE "$Id: FGState.h,v 1.80 2004/04/17 21:21:26 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the calculation of aircraft state.
    @author Jon S. Berndt
    @version $Id: FGState.h,v 1.80 2004/04/17 21:21:26 jberndt Exp $
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

 /** Initializes the simulation state based on the passed-in parameters.
      @param U the body X-Axis velocity in fps.
      @param V the body Y-Axis velocity in fps.
      @param W the body Z-Axis velocity in fps.
      @param lat latitude measured in radians from the equator, negative values are south.
      @param lon longitude, measured in radians from the Greenwich meridian, negative values are west.
      @param phi the roll angle in radians.
      @param tht the pitch angle in radians.
      @param psi the heading angle in radians measured clockwise from north.
      @param h altitude in feet.
      @param wnorth north velocity in feet per second
      @param weast eastward velocity in feet per second
      @param wdown downward velocity in feet per second
      */
  void Initialize(double U,
                  double V,
                  double W,
                  double P,
                  double Q,
                  double R,
                  double lat,
                  double lon,
                  double phi,
                  double tht,
                  double psi,
                  double h,
                  double wnorth,
                  double weast,
                  double wdown);

  /** Initializes the simulation state based on parameters from an Initial Conditions object.
      @param FGIC pointer to an initial conditions object.
      @see FGInitialConditions.
      */
  void Initialize(FGInitialCondition *FGIC);

  /// Returns the simulation time in seconds.
  inline double Getsim_time(void) const { return sim_time; }
  /// Returns the simulation delta T.
  inline double Getdt(void) { return dt; }

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
  inline void  Setdt(double delta_t) { dt = delta_t; }

  /** Increments the simulation time.
      @return the new simulation time.
      */
  inline double IncrTime(void) {
    sim_time+=dt;
    return sim_time;
  }

  /** Integrates the quaternion.
      Given the supplied rotational rate vector and integration rate, the quaternion
      is integrated. The quaternion is later used to update the transformation
      matrices.
      @param vPQR the body rotational rate column vector.
      @param rate the integration rate in seconds.
      */
  void IntegrateQuat(FGColumnVector3 vPQR, int rate);

  // ======================================= General Purpose INTEGRATOR

  enum iType {AB4, AB3, AB2, AM3, AM4, EULER, TRAPZ};

  /** Multi-method integrator.
      @param type Type of intergation scheme to use. Can be one of:
             <ul>
             <li>AB4 - Adams-Bashforth, fourth order</li>
             <li>AB3 - Adams-Bashforth, third order</li>
             <li>AB2 - Adams-Bashforth, second order</li>
             <li>AM3 - Adams Moulton, third order</li>
             <li>AM4 - Adams Moulton, fourth order</li>
             <li>EULER - Euler</li>
             <li>TRAPZ - Trapezoidal</li>
             </ul>
      @param delta_t the integration time step in seconds
      @param vTDeriv a reference to the current value of the time derivative of
             the quantity being integrated (i.e. if vUVW is being integrated
             vTDeriv is the current value of vUVWdot)
      @param vLastArray an array of previously calculated and saved values of
             the quantity being integrated (i.e. if vUVW is being integrated
             vLastArray[0] is the past value of vUVWdot, vLastArray[1] is the value of
             vUVWdot prior to that, etc.)
      @return the current, incremental value of the item integrated to add to the
              previous value. */

  template <class T> T Integrate(iType type, double delta_t, T& vTDeriv, T *vLastArray)
  {
    T vResult;

    switch (type) {
    case AB4:
      vResult = (delta_t/24.0)*(  55.0 * vLastArray[0]
                                - 59.0 * vLastArray[1]
                                + 37.0 * vLastArray[2]
                                -  9.0 * vLastArray[3] );
      vLastArray[3] = vLastArray[2];
      vLastArray[2] = vLastArray[1];
      vLastArray[1] = vLastArray[0];
      vLastArray[0] = vTDeriv;
      break;
    case AB3:
      vResult = (delta_t/12.0)*(  23.0 * vLastArray[0]
                                - 16.0 * vLastArray[1]
                                +  5.0 * vLastArray[2] );
      vLastArray[2] = vLastArray[1];
      vLastArray[1] = vLastArray[0];
      vLastArray[0] = vTDeriv;
      break;
    case AB2:
      vResult = (delta_t/2.0)*( 3.0 * vLastArray[0] - vLastArray[1] );
      vLastArray[1] = vLastArray[0];
      vLastArray[0] = vTDeriv;
      break;
    case AM4:
      vResult = (delta_t/24.0)*(   9.0 * vTDeriv
                                + 19.0 * vLastArray[0]
                                -  5.0 * vLastArray[1]
                                +  1.0 * vLastArray[2] );
      vLastArray[2] = vLastArray[1];
      vLastArray[1] = vLastArray[0];
      vLastArray[0] = vTDeriv;
      break;
    case AM3:
      vResult = (delta_t/12.0)*(  5.0 * vTDeriv
                                + 8.0 * vLastArray[0]
                                - 1.0 * vLastArray[1] );
      vLastArray[1] = vLastArray[0];
      vLastArray[0] = vTDeriv;
      break;
    case EULER:
      vResult = delta_t * vTDeriv;
      break;
    case TRAPZ:
      vResult = (delta_t*0.5) * (vTDeriv + vLastArray[0]);
      vLastArray[0] = vTDeriv;
      break;
    }

    return vResult;
  }

  // =======================================

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

