/*******************************************************************************
 
 Header:       FGForce.h
 Author:       Tony Peden
 Date started: 5/20/00
 
 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------
 
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
5/20/00  TP   Created
 
 
FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
 
The purpose of this class is to provide storage for computed forces and
encapsulate all the functionality associated with transforming those 
forces from their native coord system to the body system.  This includes
computing the moments due to the difference between the point of application
and the cg.

CAVEAT:  if the custom transform is used for wind-to-body transforms then the
         user *must* always pass this class the negative of beta. This is true
         because sideslip angle does not follow the right hand rule i.e. it is 
         positive for aircraft nose left sideslip.  Note that use of the custom 
         transform for this purpose shouldn't be necessary as it is already 
         provided by SetTransform(tWindBody) and is not subject to the same
         restriction.
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGFORCE_H
#define FGFORCE_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFDMExec.h"
#include "FGMatrix.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

typedef enum { tNone, tWindBody, tLocalBody, tCustom } TransformType;

class FGForce : public FGModel {
public:


  FGForce(FGFDMExec *FDMExec);
  ~FGForce(void);

  inline void SetNativeForces(float Fnx, float Fny, float Fnz) {
    vFn(1)=Fnx;
    vFn(2)=Fny;
    vFn(3)=Fnz;
  }
  inline void SetNativeForces(FGColumnVector vv) { vFn = vv; };

  inline void SetNativeMoments(float Ln,float Mn, float Nn) {
    vMn(1)=Ln;
    vMn(2)=Mn;
    vMn(3)=Nn;
  }
  inline void SetNativeMoments(FGColumnVector vv) { vMn = vv; }

  inline FGColumnVector GetNativeForces(void) { return vFn; }
  inline FGColumnVector GetNativeMoments(void) { return vMn; }


  FGColumnVector GetBodyForces(void);

  inline FGColumnVector GetMoments(void) { return vM; }

  //point of application, JSBsim structural coords
  //(inches, x +back, y +right, z +up)
  inline void SetLocation(float x, float y, float z) {
    vXYZn(1) = x;
    vXYZn(2) = y;
    vXYZn(3) = z;
  }
  inline void SetLocation(FGColumnVector vv) { vXYZn = vv; }
  FGColumnVector GetLocation(void) { return vXYZn; }

  //these angles are relative to body axes, not earth!!!!!
  //I'm using these because pitch, roll, and yaw are easy to visualize,
  //there's no equivalent to roll in wind axes i.e. alpha, ? , beta
  //making up new names or using these is a toss-up: either way people
  //are going to get confused.
  //They are in radians.

  void SetAnglesToBody(float broll, float bpitch, float byaw);
  inline void  SetAnglesToBody(FGColumnVector vv) { SetAnglesToBody(vv(1), vv(2), vv(3));}
  
  inline void SetSense(float x, float y, float z) { vSense(1)=x, vSense(2)=y, vSense(3)=z; }
  inline void SetSense(FGColumnVector vv) { vSense=vv; }
  
  inline FGColumnVector GetSense(void) { return vSense; }
  
  inline void SetTransformType(TransformType ii) { ttype=ii; }
  inline TransformType GetTransformType(void) { return ttype; }

  FGMatrix Transform(void);

protected:
  FGColumnVector vFn;
  FGColumnVector vMn;

private:
  FGColumnVector vFb;
  FGColumnVector vM;
  FGColumnVector vXYZn;
  FGColumnVector vDXYZ;
  FGColumnVector vSense;
  
  FGMatrix mT;


  FGFDMExec *fdmex;
  TransformType ttype;


};


#endif










