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
 
Based on Flightgear code, which is based on LaRCSim. This class wraps all
global state variables (such as velocity, position, orientation, etc.).
 
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
#  if defined(sgi) && !defined(_GNUC_)
#    include <fstream.h>
#  else
#    include <fstream>
#  endif
#endif

#include <string>
#include <map>
#include "FGDefs.h"
#include "FGInitialCondition.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STATE "$Id: FGState.h,v 1.39 2001/08/10 12:21:07 jberndt Exp $"

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

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the calculation of aircraft state.
    @author Jon S. Berndt
    @version $Id: FGState.h,v 1.39 2001/08/10 12:21:07 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGState {
public:
  enum {ePhi=1, eTht, ePsi};
  enum {eP=1, eQ, eR};
  enum {eU=1, eV, eW};
  enum {eDrag=1, eSide, eLift};

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
      @param windir direction the wind is coming from, in degrees measured clockwise from north.
      @param winmag magnitude of the wind (the wind speed) in feet per second.
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
                  float windir,
                  float winmag);

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
  eParam GetParameterIndex(string val_string);

  inline void Seta(float tt) { a = tt; }

  inline float Setsim_time(float tt) {
    sim_time = tt;
    return sim_time;
  }
  inline void  Setdt(float tt) { dt = tt; }

  void SetParameter(eParam, float);

  inline float IncrTime(void) {
    sim_time+=dt;
    return sim_time;
  }
  void InitMatrices(float phi, float tht, float psi);
  void CalcMatrices(void);
  void IntegrateQuat(FGColumnVector3 vPQR, int rate);
  FGColumnVector3& CalcEuler(void);
  FGMatrix33& GetTs2b(float alpha, float beta);
  FGMatrix33& GetTl2b(void) { return mTl2b; }
  float GetTl2b(int i, int j) { return mTl2b(i,j);}
  FGMatrix33& GetTb2l(void) { return mTb2l; }
  float GetTb2l(int i, int j) { return mTb2l(i,j);}
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

  FGAircraft* Aircraft;
  FGPosition* Position;
  FGTranslation* Translation;
  FGRotation* Rotation;
  FGOutput* Output;
  FGAtmosphere* Atmosphere;
  FGFCS* FCS;
  FGAerodynamics* Aerodynamics;

  typedef map<string, eParam> CoeffMap;
  CoeffMap coeffdef;
  void Debug(void);
  int ActiveEngine;

  FGColumnVector4 vQdot;
  FGColumnVector4 vTmp;
  FGColumnVector3 vEuler;
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

#endif

