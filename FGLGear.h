/*******************************************************************************

 Header:       FGLGear.h
 Author:       Jon S. Berndt
 Date started: 11/18/99

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

HISTORY
--------------------------------------------------------------------------------
11/18/99   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGLGEAR_H
#define FGLGEAR_H

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
	   Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
	   School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
	   JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
	   NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
	   Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
	   1982 ISBN 0-471-08936-2

********************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <simgear/compiler.h>
#endif

#include <string>
#include "FGConfigFile.h"
#include "FGMatrix.h"
#include "FGFDMExec.h"
#include "FGState.h"

/*******************************************************************************
DEFINITIONS
*******************************************************************************/

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAircraft;
class FGPosition;
class FGRotation;

class FGLGear
{
public:
  FGLGear(FGConfigFile*, FGFDMExec*);
  ~FGLGear(void);

  FGColumnVector Force(void);
  FGColumnVector Moment(void) {return vMoment;}
  FGColumnVector GetBodyLocation(void) { return vWhlBodyVec; }

  inline string GetName(void)      {return name;          }
  inline bool   GetWOW(void)       {return WOW;           }
  inline float  GetCompLen(void)   {return compressLength;}
  inline float  GetCompVel(void)   {return compressSpeed; }
  inline float  GetCompForce(void) {return Force()(3);    }
  
  inline void SetReport(bool bb) { ReportEnable=bb; }
  inline bool GetReport(void)    { return ReportEnable; }
  

private:
  enum {eX=1, eY, eZ};
  FGColumnVector vXYZ;
  FGColumnVector vMoment;
  FGColumnVector vWhlBodyVec;
  float kSpring;
  float bDamp;
  float compressLength;
  float compressSpeed;
  float staticFCoeff, dynamicFCoeff;
  float brakePct;
  float maxCompLen;
  double SinkRate;
  double GroundSpeed;
  double DistanceTraveled;
  double MaximumStrutForce;
  double MaximumStrutTravel;
  bool WOW;
  bool FirstContact;
  bool Reported;
  bool ReportEnable;
  string name;
  string BrakeType;
  string SteerType;
  string GroupMember;
  float  maxSteerAngle;

  FGFDMExec*     Exec;
  FGState*       State;
  FGAircraft*    Aircraft;
  FGPosition*    Position;
  FGRotation*    Rotation;

  void Report(void);
};

#include "FGAircraft.h"
#include "FGPosition.h"
#include "FGRotation.h"

/******************************************************************************/
#endif
