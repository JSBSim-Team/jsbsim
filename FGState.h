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
#  include <fstream>
#endif

#include <string>
#include <map>
#include "FGDefs.h"
#include "FGInitialCondition.h"
#include "FGMatrix.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_STATE "$Id: FGState.h,v 1.35 2001/05/29 20:13:31 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAircraft;
class FGTranslation;
class FGRotation;
class FGAtmosphere;
class FGOutput;
class FGPosition;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFDMExec;

class FGState {
public:
  FGState(FGFDMExec*);
  ~FGState();

  enum {ePhi=1, eTht, ePsi};
  enum {eP=1, eQ, eR};
  enum {eU=1, eV, eW};
  enum {eDrag=1, eSide, eLift};

  bool Reset(string, string, string);
  void Initialize(float, float, float, float, float, float, float, float, float);
  void Initialize(FGInitialCondition *FGIC);
  bool StoreData(string);

  inline float Geta(void) { return a; }

  inline float Getsim_time(void) { return sim_time; }
  inline float Getdt(void) { return dt; }

  inline void Suspend(void) {saved_dt = dt; dt = 0.0;}
  inline void Resume(void)  {dt = saved_dt;}

  float GetParameter(eParam val_idx);
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
  void IntegrateQuat(FGColumnVector vPQR, int rate);
  FGColumnVector CalcEuler(void);
  FGMatrix GetTs2b(float alpha, float beta);
  FGMatrix GetTl2b(void) { return mTl2b; }
  float GetTl2b(int i, int j) { return mTl2b(i,j);}
  FGMatrix GetTb2l(void) { return mTb2l; }
  float GetTb2l(int i, int j) { return mTb2l(i,j);}
  typedef map<eParam, string> ParamMap;
  ParamMap paramdef;

private:

  float a;                          // speed of sound
  float sim_time, dt;
  float saved_dt;

  FGFDMExec* FDMExec;
  FGMatrix mTb2l;
  FGMatrix mTl2b;
  FGMatrix mTs2b;
  FGColumnVector vQtrn;
  FGColumnVector vlastQdot;

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

  FGColumnVector vQdot;
  FGColumnVector vTmp;
  FGColumnVector vEuler;
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

