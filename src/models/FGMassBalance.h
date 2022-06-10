/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGMassBalance.h
 Author:       Jon S. Berndt
 Date started: 09/12/2000

 ------------- Copyright (C) 2000  Jon S. Berndt (jon@jsbsim.org) --------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

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

#include <memory>

#include "FGModel.h"
#include "math/FGMatrix33.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONSS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGPropagate;
class FGGroundReactions;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models weight, balance and moment of inertia information.

    Maintains a vector of point masses. Sums the contribution of all, and
    provides this to FGPropagate.  Loads the <mass_balance> section of the
    aircraft configuration file. There can be any number of <pointmasses>. Each
    can also have a shape which - if present - causes an associated moment of
    inertia to be calculated based on the shape. Note that a cylinder is solid,
    a tube is hollow, a ball is solid and a sphere is hollow.

    The inertia tensor must be specified in the structural frame (x axis
    positive aft, y axis positive out of the right wing and z axis upward). The
    sign of the inertia cross products are optional by JSBSim.
    if negated_crossproduct_inertia == "true", then define:
        ixy = -integral( x * y * dm ),
        ixz = -integral( x * z * dm ),
        iyz = -integral( y * z * dm ).
    else if negated_crossproduct_inertia == "false", then define:
        ixy =  integral( x * y * dm ),
        ixz =  integral( x * z * dm ),
        iyz =  integral( y * z * dm ).
    default is negated_crossproduct_inertia = "true".
    We strongly recommend defining negated_crossproduct_inertia = "false", 
    which is consistent with the specifications in the field of flight dynamics. 
    
    <h3>Configuration File Format for \<mass_balance> Section:</h3>
@code{.xml}
    <mass_balance negated_crossproduct_inertia="true|false">
        <ixx unit="{SLUG*FT2 | KG*M2}"> {number} </ixx>
        <iyy unit="{SLUG*FT2 | KG*M2}"> {number} </iyy>
        <izz unit="{SLUG*FT2 | KG*M2}"> {number} </izz>
        <ixy unit="{SLUG*FT2 | KG*M2}"> {number} </ixy>
        <ixz unit="{SLUG*FT2 | KG*M2}"> {number} </ixz>
        <iyz unit="{SLUG*FT2 | KG*M2}"> {number} </iyz>
        <emptywt unit="{LBS | KG"> {number} </emptywt>
        <location name="CG" unit="{IN | FT | M}">
            <x> {number} </x>
            <y> {number} </y>
            <z> {number} </z>
        </location>
        [<pointmass name="{string}">
            <form shape="{tube | cylinder | sphere | ball}">
               <radius unit="{IN | FT | M}"> {number} </radius>
               <length unit="{IN | FT | M}"> {number} </length>
            </form> 
            <weight unit="{LBS | KG}"> {number} </weight>
            <location name="{string}" unit="{IN | FT | M}">
                <x> {number} </x>
                <y> {number} </y>
                <z> {number} </z>
            </location>
        </pointmass>
        ... other point masses ...]
    </mass_balance>
@endcode
    
    @see Stevens and Lewis, "Flight Control & Simulation"
    @see Bernard Etkin, " Dynamics Of Atmosferic Flight"
    @see https://en.wikipedia.org/wiki/Moment_of_inertia#Inertia_tensor
    @see https://www.mathworks.com/help/physmod/sm/ug/specify-custom-inertia.html
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGMassBalance : public FGModel
{

public:
  explicit FGMassBalance(FGFDMExec*);
  ~FGMassBalance();

  bool Load(Element* el) override;
  bool InitModel(void) override;
  /** Runs the Mass Balance model; called by the Executive
      Can pass in a value indicating if the executive is directing the
      simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim
                     from advancing time. Some models may ignore this flag, such
                     as the Input model, which may need to be active to listen
                     on a socket for the "Resume" command to be given.  @return
                     false if no error */
  bool Run(bool Holding) override;

  double GetMass(void) const {return Mass;}
  double GetWeight(void) const {return Weight;}
  double GetEmptyWeight(void) const {return EmptyWeight;}
  /** Returns the coordinates of the center of gravity expressed in the
      structural frame. */
  const FGColumnVector3& GetXYZcg(void) const {return vXYZcg;}
  double GetXYZcg(int axis) const  {return vXYZcg(axis);}
  const FGColumnVector3& GetDeltaXYZcg(void) const {return vDeltaXYZcg;}
  double GetDeltaXYZcg(int axis) const  {return vDeltaXYZcg(axis);}

  /** Computes the inertia contribution of a pointmass.
      Computes and returns the inertia matrix of a pointmass of mass
      slugs at the given vector r in the structural frame. The units
      should be for the mass in slug and the vector in the structural
      frame as usual in inches.
      @param mass_sl the mass of this single pointmass given in slugs
      @param r the location of this single pointmass in the structural frame
   */
  FGMatrix33 GetPointmassInertia(double mass_sl, const FGColumnVector3& r) const
  {
    FGColumnVector3 v = StructuralToBody( r );
    FGColumnVector3 sv = mass_sl*v;
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

  void SetEmptyWeight(double EW) { EmptyWeight = EW;}
  void SetBaseCG(const FGColumnVector3& CG) {vbaseXYZcg = vXYZcg = CG;}

  void AddPointMass(Element* el);
  double GetTotalPointMassWeight(void) const;

  const FGColumnVector3& GetPointMassMoment(void);
  /// Returns the inertia matrix expressed in the body frame.
  const FGMatrix33& GetJ(void) const {return mJ;}
  /// Returns the inverse of the inertia matrix expressed in the body frame.
  const FGMatrix33& GetJinv(void) const {return mJinv;}
  void SetAircraftBaseInertias(const FGMatrix33& BaseJ) {baseJ = BaseJ;}
  void GetMassPropertiesReport(int i);
  
  struct Inputs {
    double GasMass;
    double TanksWeight;
    FGColumnVector3 GasMoment;
    FGMatrix33 GasInertia;
    FGColumnVector3 TanksMoment;
    FGMatrix33 TankInertia;
    bool WOW;
  } in;

private:
  std::shared_ptr<FGPropagate> Propagate;
  double Weight;
  double EmptyWeight;
  double Mass;
  FGMatrix33 mJ;
  FGMatrix33 mJinv;
  FGMatrix33 pmJ;
  FGMatrix33 baseJ;
  FGColumnVector3 vXYZcg;
  FGColumnVector3 vLastXYZcg;
  FGColumnVector3 vDeltaXYZcg;
  FGColumnVector3 vDeltaXYZcgBody;
  FGColumnVector3 vXYZtank;
  FGColumnVector3 vbaseXYZcg;
  FGColumnVector3 vPMxyz;
  FGColumnVector3 PointMassCG;
  const FGMatrix33& CalculatePMInertias(void);
  double GetIxx(void) const { return mJ(1,1); }
  double GetIyy(void) const { return mJ(2,2); }
  double GetIzz(void) const { return mJ(3,3); }
  double GetIxy(void) const { return -mJ(1,2); }
  double GetIxz(void) const { return mJ(1,3); }
  double GetIyz(void) const { return -mJ(2,3); }

  /** The PointMass structure encapsulates a point mass object, moments of inertia
     mass, location, etc. */
  struct PointMass {
    PointMass(double w, FGColumnVector3& vXYZ) :
      eShapeType(esUnspecified), Location(vXYZ), Weight(w), Radius(0.0),
      Length(0.0) {}

    void CalculateShapeInertia(void) {
      switch(eShapeType) {
        case esTube:
          mPMInertia(1,1) = (Weight/(slugtolb))*Radius*Radius; // mr^2
          mPMInertia(2,2) = (Weight/(slugtolb*12))*(6*Radius*Radius + Length*Length);
          mPMInertia(3,3) = mPMInertia(2,2);
          break;
        case esCylinder:
          mPMInertia(1,1) = (Weight/(slugtolb*2))*Radius*Radius; // 0.5*mr^2
          mPMInertia(2,2) = (Weight/(slugtolb*12))*(3*Radius*Radius + Length*Length);
          mPMInertia(3,3) = mPMInertia(2,2);
          break;
        case esSphere:
          mPMInertia(1,1) = (Weight/(slugtolb*3))*Radius*Radius*2; // (2mr^2)/3
          mPMInertia(2,2) = mPMInertia(1,1);
          mPMInertia(3,3) = mPMInertia(1,1);
          break;
        case esBall:
          mPMInertia(1,1) = (Weight/(slugtolb*5))*Radius*Radius*2; // (2mr^2)/5
          mPMInertia(2,2) = mPMInertia(1,1);
          mPMInertia(3,3) = mPMInertia(1,1);
          break;
        default:
          break;
      }
    }

    enum esShape {esUnspecified, esTube, esCylinder, esSphere, esBall} eShapeType;
    FGColumnVector3 Location;
    double Weight; /// Weight in pounds.
    double Radius; /// Radius in feet.
    double Length; /// Length in feet.
    std::string Name;
    FGMatrix33 mPMInertia;

    double GetPointMassLocation(int axis) const {return Location(axis);}
    double GetPointMassWeight(void) const {return Weight;}
    esShape GetShapeType(void) {return eShapeType;}
    const FGColumnVector3& GetLocation(void) {return Location;}
    const FGMatrix33& GetPointMassInertia(void) {return mPMInertia;}
    const std::string& GetName(void) {return Name;}

    void SetPointMassLocation(int axis, double value) {Location(axis) = value;}
    void SetPointMassWeight(double wt) {
      Weight = wt;
      CalculateShapeInertia();
    }
    void SetPointMassShapeType(esShape st) {eShapeType = st;}
    void SetRadius(double r) {Radius = r;}
    void SetLength(double l) {Length = l;}
    void SetName(const std::string& name) {Name = name;}
    void SetPointMassMoI(const FGMatrix33& MoI) { mPMInertia = MoI; }
    double GetPointMassMoI(int r, int c) {return mPMInertia(r,c);}

    void bind(FGPropertyManager* PropertyManager, unsigned int num);
  };

  std::vector <struct PointMass*> PointMasses;

  void bind(void);
  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
