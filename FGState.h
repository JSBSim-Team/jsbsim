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
#include "FGDefs.h"
#include "FGJSBBase.h"
#include "FGInitialCondition.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STATE "$Id: FGState.h,v 1.47 2001/10/29 17:47:34 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAircraft;
class FGTranslation;
class FGRotation;
class FGAtmosphere;
class FGOutput;
class FGPosition;
class FGFDMExec;
class FGGroundReactions;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the calculation of aircraft state.
    @author Jon S. Berndt
    @version $Id: FGState.h,v 1.47 2001/10/29 17:47:34 jberndt Exp $
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

  /** Specifies the Reset file to use.
      The reset file normally resides in the same directory as an aircraft config file.
      it includes the following information:
      <ul>
      <li>U, the body X-Axis velocity</li>
      <li>V, the body Y-Axis velocity</li>
      <li>W, the body Z-Axis velocity</li>
      <li>Latitude measured in radians from the equator, negative values are south.</li>
      <li>Longitude, measured in radians from the Greenwich meridian, negative values are west.</li>
      <li>Phi, the roll angle in radians.</li>
      <li>Theta, the pitch attitude in radians.</li>
      <li>Psi, the heading angle in radians.</li>
      <li>H, the altitude in feet</li>
      <li>Wind Direction, the direction the wind is coming <u>from</u>.</li>
      <li>Wind magnitude, the wind speed in fps.</li>
      </ul>
      @param path the path string leading to the specific aircraft file, i.e. "aircraft".
      @param aircraft the name of the aircraft, i.e. "c172".
      @param filename the name of the reset file without an extension, i.e. "reset00".
      @return true if successful, false if the file could not be opened.
      */
  bool Reset(string path, string aircraft, string filename);

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
  void Initialize(float U,
                  float V,
                  float W,
                  float lat,
                  float lon,
                  float phi,
                  float tht,
                  float psi,
                  float h,
                  float wnorth,
                  float weast,
                  float wdown);

  /** Initializes the simulation state based on parameters from an Initial Conditions object.
      @param FGIC pointer to an initial conditions object.
      @see FGInitialConditions.
      */
  void Initialize(FGInitialCondition *FGIC);

  /** Stores state data in the supplied file name.
      @param filename the file to store the data in.
      @return true if successful.
      */
  bool StoreData(string filename);

  /// returns the speed of sound in feet per second.
  inline float Geta(void) { return a; }

  /// Returns the simulation time in seconds.
  inline float Getsim_time(void) { return sim_time; }
  /// Returns the simulation delta T.
  inline float Getdt(void) { return dt; }

  /// Suspends the simulation and sets the delta T to zero.
  inline void Suspend(void) {saved_dt = dt; dt = 0.0;}
  /// Resumes the simulation by resetting delta T to the correct value.
  inline void Resume(void)  {dt = saved_dt;}

  /** Retrieves a parameter.
      The parameters that can be retrieved are enumerated in FGDefs.h.
      @param val_idx one of the enumerated JSBSim parameters.
      @return the value of the parameter.
      */
  float GetParameter(eParam val_idx);

  /** Retrieves a parameter.
      The parameters that can be retrieved are enumerated in FGDefs.h.
      @param val_string a string representing one of the enumerated JSBSim parameters,
             i.e. "FG_QBAR".
      @return the value of the parameter.
      */
  float GetParameter(string val_string);

  /** Retrieves the JSBSim parameter enumerated item given the text string.
      @param val_string the parameter string, i.e. "FG_QBAR".
      @return the JSBSim parameter index (an enumerated type) for the supplied string.
      */
  eParam GetParameterIndex(string val_string);

  /** Sets the speed of sound.
      @param speed the speed of sound in feet per second.
      */
  inline void Seta(float speed) { a = speed; }

  /** Sets the current sim time.
      @param cur_time the current time
      @return the current time.
      */
  inline float Setsim_time(float cur_time) {
    sim_time = cur_time;
    return sim_time;
  }
  
  /** Sets the integration time step for the simulation executive.
      @param delta_t the time step in seconds.
      */
  inline void  Setdt(float delta_t) { dt = delta_t; }

  /** Sets the JSBSim parameter to the supplied value.
      @param prm the JSBSim parameter to set, i.e. FG_RUDDER_POS.
      @param val the value to give the parameter.
      */
  void SetParameter(eParam prm, float val);

  /** Increments the simulation time.
      @return the new simulation time.
      */
  inline float IncrTime(void) {
    sim_time+=dt;
    return sim_time;
  }

  /** Initializes the transformation matrices.
      @param phi the roll angle in radians.
      @param tht the pitch angle in radians.
      @param psi the heading angle in radians
      */
  void InitMatrices(float phi, float tht, float psi);

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

  /** Calculates Euler angles from the local-to-body matrix.
      @return a reference to the vEuler column vector.
      */
  FGColumnVector3& CalcEuler(void);

  /** Calculates and returns the stability-to-body axis transformation matrix.
      @param alpha angle of attack in radians.
      @param beta angle of sideslip in radians.
      @return a reference to the stability-to-body transformation matrix.
      */
  FGMatrix33& GetTs2b(float alpha, float beta);

  /** Retrieves the local-to-body transformation matrix.
      @return a reference to the local-to-body transformation matrix.
      */
  FGMatrix33& GetTl2b(void) { return mTl2b; }

  /** Retrieves a specific local-to-body matrix element.
      @param r matrix row index.
      @param c matrix column index.
      @return the matrix element described by the row and column supplied.
      */
  float GetTl2b(int r, int c) { return mTl2b(r,c);}

  /** Retrieves the body-to-local transformation matrix.
      @return a reference to the body-to-local matrix.
      */
  FGMatrix33& GetTb2l(void) { return mTb2l; }

  /** Retrieves a specific body-to-local matrix element.
      @param r matrix row index.
      @param c matrix column index.
      @return the matrix element described by the row and column supplied.
      */
  float GetTb2l(int i, int j) { return mTb2l(i,j);}
  
  /** Prints a summary of simulator state (speed, altitude, 
      configuration, etc.)
  */
  void ReportState(void);


  typedef map<eParam, string> ParamMap;
  ParamMap paramdef;

private:
  float a;                          // speed of sound
  float sim_time, dt;
  float saved_dt;

  FGFDMExec* FDMExec;
  FGMatrix33 mTb2l;
  FGMatrix33 mTl2b;
  FGMatrix33 mTs2b;
  FGColumnVector4 vQtrn;
  FGColumnVector4 vlastQdot;
  FGColumnVector3 vUVW;
  FGColumnVector3 vLocalVelNED;
  FGColumnVector3 vLocalEuler;
  FGColumnVector4 vQdot;
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

  typedef map<string, eParam> CoeffMap;
  CoeffMap coeffdef;
  void Debug(void);
  int ActiveEngine;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

#endif

