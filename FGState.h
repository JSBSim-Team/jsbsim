/*******************************************************************************

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

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGSTATE_H
#define FGSTATE_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  include STL_STRING
#  ifdef FG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
   FG_USING_STD(string);
#else
#  include <string>
#  include <fstream>
#endif

#include <map>
#include "FGDefs.h"
#include "FGInitialCondition.h"
#include "FGMatrix.h"

/*******************************************************************************
DEFINES
*******************************************************************************/

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFDMExec;

class FGState
{
public:
   FGState(FGFDMExec*);
  ~FGState(void);

  bool Reset(string, string, string);
  void Initialize(float, float, float, float, float, float, float, float, float);
  void Initialize(FGInitialCondition *FGIC);
  bool StoreData(string);

  inline float GetVt(void) {return Vt;}

  inline float Getlatitude(void) {return latitude;}
  inline float Getlongitude(void) {return longitude;}
  inline float GetGeodeticLat(void) {return GeodeticLat;}

  inline float Getadot(void) {return adot;}
  inline float Getbdot(void) {return bdot;}

  inline float GetLocalAltitudeOverRunway(void) {return LocalAltitudeOverRunway;}
  inline float Geth(void) {return h;}
  inline float Geta(void) {return a;}
  inline float GetMach(void) {return Mach;}

  inline float Getsim_time(void) {return sim_time;}
  inline float Getdt(void) {return dt;}

  inline float Getqbar(void) {return qbar;}
  float GetParameter(int val_idx);
  float GetParameter(string val_string);
  int GetParameterIndex(string val_string);

  inline void SetVt(float tt) {Vt = tt;}

  inline void Setlatitude(float tt) {latitude = tt;}
  inline void Setlongitude(float tt) {longitude = tt;}
  inline void SetGeodeticLat(float tt) {GeodeticLat = tt;}

  inline void Setadot(float tt) {adot = tt;}
  inline void Setbdot(float tt) {bdot = tt;}

  inline void Setqbar(float tt) {qbar = tt;}

  inline void SetLocalAltitudeOverRunway(float tt) {LocalAltitudeOverRunway = tt;}
  inline void Seth(float tt) {h = tt;}
  inline void Seta(float tt) {a = tt;}
  inline void SetMach(float tt) {Mach = tt;}

  inline float Setsim_time(float tt) {sim_time = tt; return sim_time;}
  inline void  Setdt(float tt) {dt = tt;}

  inline float IncrTime(void) {sim_time+=dt;return sim_time;}
  void CalcMatrices(float phi, float tht, float psi);
  void IntegrateQuat(float P, float Q, float R);


private:

  float Vt;                         // Total velocity
  float latitude, longitude;        // position
  float GeodeticLat;                // Geodetic Latitude
  float adot, bdot;                 // alpha dot and beta dot
  float h, a;                       // altitude above sea level, speed of sound
  float qbar;                       // dynamic pressure
  float sim_time, dt;
  float Mach;                       // Mach number

  FGFDMExec* FDMExec;
  float LocalAltitudeOverRunway;
  FGMatrix* Tb2l;
  FGMatrix* Tl2b;
  FGMatrix* Ts2b;
  FGColumnVector* Qtrn;

  typedef map<string, long> CoeffMap;
  CoeffMap coeffdef;

protected:

};

/******************************************************************************/
#endif
