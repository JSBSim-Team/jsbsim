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
#include <vector>

#define ID_MASSBALANCE "$Id: FGMassBalance.h,v 1.14 2001/12/06 20:56:54 jberndt Exp $"

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
  inline double GetIxy(void) {return Ixy;}
  inline double GetIxz(void) {return Ixz;}
  inline FGColumnVector3& GetXYZcg(void) {return vXYZcg;}
  inline double GetXYZcg(int axis) {return vXYZcg(axis);}

  inline void SetEmptyWeight(double EW) { EmptyWeight = EW;}
  inline void SetBaseIxx(double bixx)   { baseIxx = bixx;}
  inline void SetBaseIyy(double biyy)   { baseIyy = biyy;}
  inline void SetBaseIzz(double bizz)   { baseIzz = bizz;}
  inline void SetBaseIxy(double bixy)   { baseIxy = bixy;}
  inline void SetBaseIxz(double bixz)   { baseIxz = bixz;}
  inline void SetBaseCG(const FGColumnVector3& CG) {vbaseXYZcg = CG;}
  
  void AddPointMass(double weight, double X, double Y, double Z);
  double GetPointMassWeight(void);
  FGColumnVector3& GetPointMassCG(void);
  double GetPMIxx(void);
  double GetPMIyy(void);
  double GetPMIzz(void);
  double GetPMIxy(void);
  double GetPMIxz(void);

private:
  double Weight;
  double EmptyWeight;
  double Mass;
  double Ixx;
  double Iyy;
  double Izz;
  double Ixy;
  double Ixz;
  double baseIxx;
  double baseIyy;
  double baseIzz;
  double baseIxy;
  double baseIxz;
  FGColumnVector3 vXYZcg;
  FGColumnVector3 vXYZtank;
  FGColumnVector3 vbaseXYZcg;
  vector <FGColumnVector3> PointMassLoc;
  vector <double> PointMassWeight;
  FGColumnVector3 PointMassCG;
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
