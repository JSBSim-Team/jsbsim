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

#define ID_MASSBALANCE "$Id: FGMassBalance.h,v 1.13 2001/11/14 23:53:27 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMassBalance : public FGModel
{

public:
  FGMassBalance(FGFDMExec*);
  ~FGMassBalance();

  bool Run(void);

  inline double GetMass(void) {return Mass;}
  inline double GetWeight(void) {return Weight;}
  inline double GetIxx(void) {return Ixx;}
  inline double GetIyy(void) {return Iyy;}
  inline double GetIzz(void) {return Izz;}
  inline double GetIxz(void) {return Ixz;}
  inline double GetIyz(void) {return Iyz;}
  inline FGColumnVector3& GetXYZcg(void) {return vXYZcg;}
  inline double GetXYZcg(int axis) {return vXYZcg(axis);}

  inline void SetEmptyWeight(double EW) { EmptyWeight = EW;}
  inline void SetBaseIxx(double bixx)   { baseIxx = bixx;}
  inline void SetBaseIyy(double biyy)   { baseIyy = biyy;}
  inline void SetBaseIzz(double bizz)   { baseIzz = bizz;}
  inline void SetBaseIxz(double bixz)   { baseIxz = bixz;}
  inline void SetBaseIyz(double biyz)   { baseIyz = biyz;}
  inline void SetBaseCG(const FGColumnVector3& CG) {vbaseXYZcg = CG;}

private:
  double Weight;
  double EmptyWeight;
  double Mass;
  double Ixx;
  double Iyy;
  double Izz;
  double Ixz;
  double Iyz;
  double baseIxx;
  double baseIyy;
  double baseIzz;
  double baseIxz;
  double baseIyz;
  FGColumnVector3 vXYZcg;
  FGColumnVector3 vXYZtank;
  FGColumnVector3 vbaseXYZcg;
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
