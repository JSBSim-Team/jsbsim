/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGMassBalance.h
 Author:       Jon S. Berndt
 Date started: 09/12/2000

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMASSBALANCE_H
#define FGMASSBALANCE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGPropulsion.h"

#define ID_MASSBALANCE "$Id: FGMassBalance.h,v 1.11 2001/06/04 19:52:14 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMassBalance : public FGModel
{

public:
  FGMassBalance(FGFDMExec*);
  ~FGMassBalance();

  bool Run(void);

  inline float GetMass(void) {return Mass;}
  inline float GetWeight(void) {return Weight;}
  inline float GetIxx(void) {return Ixx;}
  inline float GetIyy(void) {return Iyy;}
  inline float GetIzz(void) {return Izz;}
  inline float GetIxz(void) {return Ixz;}
  inline float GetIyz(void) {return Iyz;}
  inline FGColumnVector& GetXYZcg(void) {return vXYZcg;}
  inline float GetXYZcg(int axis) {return vXYZcg(axis);}

  inline void SetEmptyWeight(float EW) { EmptyWeight = EW;}
  inline void SetBaseIxx(float bixx)   { baseIxx = bixx;}
  inline void SetBaseIyy(float biyy)   { baseIyy = biyy;}
  inline void SetBaseIzz(float bizz)   { baseIzz = bizz;}
  inline void SetBaseIxz(float bixz)   { baseIxz = bixz;}
  inline void SetBaseIyz(float biyz)   { baseIyz = biyz;}
  inline void SetBaseCG(const FGColumnVector& CG) {vbaseXYZcg = CG;}

private:
  float Weight;
  float EmptyWeight;
  float Mass;
  float Ixx;
  float Iyy;
  float Izz;
  float Ixz;
  float Iyz;
  float baseIxx;
  float baseIyy;
  float baseIzz;
  float baseIxz;
  float baseIyz;
  FGColumnVector vXYZcg;
  FGColumnVector vXYZtank;
  FGColumnVector vbaseXYZcg;
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
