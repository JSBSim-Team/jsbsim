/*******************************************************************************

 Header:       FGPosition.h
 Author:       Jon S. Berndt
 Date started: 1/5/99

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
01/05/99   JSB   Created

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGPOSITION_H
#define FGPOSITION_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGModel.h"
#include "FGMatrix.h"

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGPosition : public FGModel
{
  FGColumnVector vUVW;
  FGColumnVector vVel;

  float Vee, invMass, invRadius;
  double Radius, h;
  float LatitudeDot, LongitudeDot, RadiusDot;
  float lastLatitudeDot, lastLongitudeDot, lastRadiusDot;
  float Longitude, Latitude;
  float dt;
  double RunwayElevation;
  double DistanceAGL;

  void GetState(void);

public:
  FGPosition(FGFDMExec*);
  ~FGPosition(void);

  inline FGColumnVector GetVel(void) {return vVel;}
  inline FGColumnVector GetUVW(void) {return vUVW;}
  inline float GetVn(void)  {return vVel(1);}
  inline float GetVe(void)  {return vVel(2);}
  inline float GetVd(void)  {return vVel(3);}
  inline float Geth(void)  {return h;}
  inline float GetLatitude(void) {return Latitude;}
  inline float GetLongitude(void) {return Longitude;}
  inline double GetRunwayElevation(void) {return RunwayElevation;}
  inline double GetDistanceAGL(void)  {return DistanceAGL;}

  void SetvVel(const FGColumnVector& v) {vVel = v;}
  void SetLatitude(float tt) {Latitude = tt;}
  void SetLongitude(float tt) {Longitude = tt;}
  void Seth(float tt) {h = tt;}
  void SetRunwayElevation(double tt) {RunwayElevation = tt;}
  void SetDistanceAGL(double tt) {DistanceAGL = tt;}

  bool Run(void);
};

/******************************************************************************/
#endif
