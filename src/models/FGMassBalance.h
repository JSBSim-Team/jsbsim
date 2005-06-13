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
#include <math/FGColumnVector3.h>
#include <math/FGMatrix33.h>
#include <input_output/FGXMLElement.h>
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MASSBALANCE "$Id: FGMassBalance.h,v 1.3 2005/06/13 16:59:18 ehofman Exp $"

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

  bool Load(Element* el);

  bool Run(void);

  inline double GetMass(void) const {return Mass;}
  inline double GetWeight(void) const {return Weight;}
  inline FGColumnVector3& GetXYZcg(void) {return vXYZcg;}
  inline double GetXYZcg(int axis) const  {return vXYZcg(axis);}

  /** Computes the inertia contribution of a pointmass.
      Computes and returns the inertia matrix of a pointmass of mass
      slugs at the given vector r in the structural frame. The units
      should be for the mass in slug and the vector in the structural
      frame as usual in inches.
      @param slugs the mass of this single pointmass given in slugs
      @param r the location of this single pointmass in the structural frame
   */
  FGMatrix33 GetPointmassInertia(double slugs, const FGColumnVector3& r) const
  {
    FGColumnVector3 v = StructuralToBody( r );
    FGColumnVector3 sv = slugs*v;
    double xx = sv(1)*v(1);
    double yy = sv(2)*v(2);
    double zz = sv(3)*v(3);
    double xy = -sv(1)*v(2);
    double xz = -sv(1)*v(3);
    double yz = -sv(2)*v(3);
    return FGMatrix33( yy+zz, xy, xz,
                       xy, xx+zz, yz,
                       xz, yz, xx+yy );
  }

  /** Conversion from the structural frame to the body frame.
      Converts the location given in the structural frame
      coordinate system to the body frame. The units of the structural
      frame are assumed to be in inches. The unit of the result is in
      ft.
      @param r vector coordinate in the structural reference frame (X positive
               aft, measurements in inches).
      @return vector coordinate in the body frame, in feet.
   */
  FGColumnVector3 StructuralToBody(const FGColumnVector3& r) const;

  inline void SetEmptyWeight(double EW) { EmptyWeight = EW;}
  inline void SetBaseCG(const FGColumnVector3& CG) {vbaseXYZcg = vXYZcg = CG;}

  void AddPointMass(Element* el);
  double GetPointMassWeight(void);
  FGColumnVector3& GetPointMassMoment(void);
  FGMatrix33& GetJ(void) {return mJ;}
  FGMatrix33& GetJinv(void) {return mJinv;}
  void SetAircraftBaseInertias(FGMatrix33 BaseJ) {baseJ = BaseJ;}

  void bind(void);
  void unbind(void);

private:
  double Weight;
  double EmptyWeight;
  double Mass;
  FGMatrix33 mJ;
  FGMatrix33 mJinv;
  FGMatrix33 pmJ;
  FGMatrix33 baseJ;
  FGColumnVector3 vXYZcg;
  FGColumnVector3 vXYZtank;
  FGColumnVector3 vbaseXYZcg;
  FGColumnVector3 vPMxyz;
  FGColumnVector3 PointMassCG;
  FGMatrix33& CalculatePMInertias(void);

  struct PointMass {
    PointMass(double w, double x, double y, double z) {
      Weight = w;
      Location.InitMatrix(x, y, z);
    }
    FGColumnVector3 Location;
    double Weight;
  };

  vector <struct PointMass> PointMasses;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
