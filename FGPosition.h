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

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGPosition : public FGModel
{
public:
  FGPosition(FGFDMExec*);
  ~FGPosition(void);

  inline float GetFn(void) {return Fn;}
  inline float GetFe(void) {return Fe;}
  inline float GetFd(void) {return Fd;}

  inline float GetVn(void) {return Vn;}
  inline float GetVe(void) {return Ve;}
  inline float GetVd(void) {return Vd;}

  inline float GetT(int r, int c) {return T[r][c];}
  inline void SetT(float t1, float t2, float t3, float t4, float t5, float t6,
                                                   float t7, float t8, float t9)
                                         {T[1][1]=t1; T[1][2]=t2 ;T[1][3]=t3;
                                          T[2][1]=t4; T[2][2]=t5 ;T[2][3]=t6;
                                          T[3][1]=t7; T[3][2]=t8 ;T[3][3]=t9;}

  bool Run(void);

protected:

private:
  float T[4][4];
  float Q0, Q1, Q2, Q3;
  float Fn, Fe, Fd;
  float Fx, Fy, Fz;
  float U, V, W;
  float Vn, Ve, Vd, Vee;
  float invMass, invRadius;
  double Radius;
  float AccelN, AccelE, AccelD;
  float lastAccelN, lastAccelE, lastAccelD;
  float LatitudeDot, LongitudeDot, RadiusDot;
  float lastLatitudeDot, lastLongitudeDot, lastRadiusDot;
  float Longitude, Latitude;
  float dt;

  void GetState(void);
  void PutState(void);
};

/******************************************************************************/
#endif
