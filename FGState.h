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
#  if defined(sgi) && !defined(__GNUC__)
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
#include "FGColumnVector4.h"

#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAerodynamics.h"
#include "FGOutput.h"
#include "FGAircraft.h"
#include "FGGroundReactions.h"
#include "FGPropulsion.h"


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STATE "$Id: FGState.h,v 1.65 2003/01/22 15:53:36 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the calculation of aircraft state.
    @author Jon S. Berndt
    @version $Id: FGState.h,v 1.65 2003/01/22 15:53:36 jberndt Exp $
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGState.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGState.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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

  /// returns the speed of sound in feet per second.
  inline double Geta(void) { return a; }

  /// Returns the simulation time in seconds.
  inline double Getsim_time(void) const { return sim_time; }
  /// Returns the simulation delta T.
  inline double Getdt(void) { return dt; }

  /// Suspends the simulation and sets the delta T to zero.
  inline void Suspend(void) {saved_dt = dt; dt = 0.0;}
  /// Resumes the simulation by resetting delta T to the correct value.
  inline void Resume(void)  {dt = saved_dt;}

  /** Sets the speed of sound.
      @param speed the speed of sound in feet per second.
      */
  inline void Seta(double speed) { a = speed; }

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

  /** Initializes the transformation matrices.
      @param phi the roll angle in radians.
      @param tht the pitch angle in radians.
      @param psi the heading angle in radians
      */
  void InitMatrices(double phi, double tht, double psi);

  /** Calculates the local-to-body and body-to-local conversion matrices.
      */
  void CalcMatrices(void);

  /** Integrates the quaternion.
      Given the supplied rotational rate vector and integration rate, the quaternion
      is integrated. The quaternion is later used to update the transformation
      matrices.
      @param vPQR the body rotational rate column vector.
      @param rate the integration rate in seconds.
      */
  void IntegrateQuat(FGColumnVector3 vPQR, int rate);
  
  // ======================================= General Purpose INTEGRATOR

  enum iType {AB4, AB3, AB2, AM3, EULER, TRAPZ};
  
  /** Multi-method integrator.
      @param type Type of intergation scheme to use. Can be one of:
             <ul>
             <li>AB4 - Adams-Bashforth, fourth order</li>
             <li>AB3 - Adams-Bashforth, third order</li>
             <li>AB2 - Adams-Bashforth, second order</li>
             <li>AM3 - Adams Moulton, third order</li>
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
      vResult = (delta_t/24.0)*(  55.0 * vTDeriv
                                - 59.0 * vLastArray[0]
                                + 37.0 * vLastArray[1]
                                -  9.0 * vLastArray[2] );
      vLastArray[2] = vLastArray[1];
      vLastArray[1] = vLastArray[0];
      vLastArray[0] = vTDeriv;
      break;
    case AB3:
      vResult = (delta_t/12.0)*(  23.0 * vTDeriv
                                - 16.0 * vLastArray[0]
                                +  5.0 * vLastArray[1] );
      vLastArray[1] = vLastArray[0];
      vLastArray[0] = vTDeriv;
      break;
    case AB2:
      vResult = (delta_t/2.0)*( 3.0 * vTDeriv - vLastArray[0] );
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

  /** Calculates Euler angles from the local-to-body matrix.
      @return a reference to the vEuler column vector.
      */
  FGColumnVector3& CalcEuler(void);

  /** Calculates and returns the stability-to-body axis transformation matrix.
      @return a reference to the stability-to-body transformation matrix.
      */
  FGMatrix33& GetTs2b(void);
  
  /** Calculates and returns the body-to-stability axis transformation matrix.
      @return a reference to the stability-to-body transformation matrix.
      */
  FGMatrix33& GetTb2s(void);

  /** Retrieves the local-to-body transformation matrix.
      @return a reference to the local-to-body transformation matrix.
      */
  FGMatrix33& GetTl2b(void) { return mTl2b; }

  /** Retrieves a specific local-to-body matrix element.
      @param r matrix row index.
      @param c matrix column index.
      @return the matrix element described by the row and column supplied.
      */
  double GetTl2b(int r, int c) { return mTl2b(r,c);}

  /** Retrieves the body-to-local transformation matrix.
      @return a reference to the body-to-local matrix.
      */
  FGMatrix33& GetTb2l(void) { return mTb2l; }

  /** Retrieves a specific body-to-local matrix element.
      @param r matrix row index.
      @param c matrix column index.
      @return the matrix element described by the row and column supplied.
      */
  double GetTb2l(int i, int j) { return mTb2l(i,j);}
  
  /** Prints a summary of simulator state (speed, altitude, 
      configuration, etc.)
  */
  void ReportState(void);
  
  void bind();
  void unbind();

private:
  double a;                          // speed of sound
  double sim_time, dt;
  double saved_dt;

  FGFDMExec* FDMExec;
  FGMatrix33 mTb2l;
  FGMatrix33 mTl2b;
  FGMatrix33 mTs2b;
  FGMatrix33 mTb2s;
  FGColumnVector4 vQtrn;
  FGColumnVector4 vQdot_prev[3];
  FGColumnVector4 vQdot;
  FGColumnVector3 vUVW;
  FGColumnVector3 vLocalVelNED;
  FGColumnVector3 vLocalEuler;
  
  FGColumnVector4 vTmp;
  FGColumnVector3 vEuler;

  FGAircraft* Aircraft;
  FGPosition* Position;
  FGTranslation* Translation;
  FGRotation* Rotation;
  FGOutput* Output;
  FGAtmosphere* Atmosphere;
  FGFCS* FCS;
  FGAerodynamics* Aerodynamics;
  FGGroundReactions* GroundReactions;
  FGPropulsion* Propulsion;
  FGPropertyManager* PropertyManager;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


#endif

