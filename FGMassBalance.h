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
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMASSBALANCE_H
#define FGMASSBALANCE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGColumnVector3.h"
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MASSBALANCE "$Id: FGMassBalance.h,v 1.25 2004/03/03 11:56:52 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONSS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models weight and balance information.
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMassBalance : public FGModel
{

public:
  FGMassBalance(FGFDMExec*);
  ~FGMassBalance();

  bool Run(void);

  inline double GetMass(void) const {return Mass;}
  inline double GetWeight(void) const {return Weight;}
  inline double GetIxx(void) const {return Ixx;}
  inline double GetIyy(void) const {return Iyy;}
  inline double GetIzz(void) const {return Izz;}
  inline double GetIxy(void) const {return Ixy;}
  inline double GetIxz(void) const {return Ixz;}
  inline double GetIyz(void) const {return Iyz;}
  inline FGColumnVector3& GetXYZcg(void) {return vXYZcg;}
  inline double GetXYZcg(int axis) const  {return vXYZcg(axis);}

  /** Conversion from the structural frame to the body frame.
   * Converts the argument \parm r given in the reference frame
   * coordinate system to the body frame. The units of the structural
   * frame are assumed to be in inches. The unit of the result is in
   * ft.
   */
  FGColumnVector3 StructuralToBody(const FGColumnVector3& r) const;

  inline void SetEmptyWeight(double EW) { EmptyWeight = EW;}
  inline void SetBaseIxx(double bixx)   { baseIxx = bixx;}
  inline void SetBaseIyy(double biyy)   { baseIyy = biyy;}
  inline void SetBaseIzz(double bizz)   { baseIzz = bizz;}
  inline void SetBaseIxy(double bixy)   { baseIxy = bixy;}
  inline void SetBaseIxz(double bixz)   { baseIxz = bixz;}
  inline void SetBaseIyz(double biyz)   { baseIyz = biyz;}
  inline void SetBaseCG(const FGColumnVector3& CG) {vbaseXYZcg = vXYZcg = CG;}

  void AddPointMass(double weight, double X, double Y, double Z);
  double GetPointMassWeight(void);
  FGColumnVector3& GetPointMassMoment(void);

  void bind(void);
  void unbind(void);

private:
  double Weight;
  double EmptyWeight;
  double Mass;
  double Ixx;
  double Iyy;
  double Izz;
  double Ixy;
  double Ixz;
  double Iyz;
  double pmIxx;
  double pmIyy;
  double pmIzz;
  double pmIxy;
  double pmIxz;
  double pmIyz;
  double baseIxx;
  double baseIyy;
  double baseIzz;
  double baseIxy;
  double baseIxz;
  double baseIyz;
  FGColumnVector3 vXYZcg;
  FGColumnVector3 vXYZtank;
  FGColumnVector3 vbaseXYZcg;
  FGColumnVector3 vPMxyz;
  vector <FGColumnVector3> PointMassLoc;
  vector <double> PointMassWeight;
  FGColumnVector3 PointMassCG;
  void CalculatePMInertia(void);

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
