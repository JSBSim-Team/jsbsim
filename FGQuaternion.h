/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGQuaternion.h
 Author:       Jon Berndt, Mathis Froehlich
 Date started: 12/02/98

 ------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) ------------------
 -------           (C) 2004  Mathias Froehlich (Mathias.Froehlich@web.de) ----

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
-------------------------------------------------------------------------------
12/02/98   JSB   Created
15/01/04   MF    Quaternion class from old FGColumnVector4

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGQUATERNION_H
#define FGQUATERNION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  INCLUDES
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  DEFINITIONS
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_QUATERNION "$Id: FGQuaternion.h,v 1.2 2004/03/29 02:15:51 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  CLASS DOCUMENTATION
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the rotational portion of the Equations of Motion.
    Note: The order of rotations used in this class corresponds to a 3-2-1 sequence,
    or Y-P-R, or Z-Y-X, if you prefer.
    @see Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
    Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
    School, January 1994
    @see D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
    JSC 12960, July 1977
    @see Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
    NASA-Ames", NASA CR-2497, January 1975
    @see Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
    Wiley & Sons, 1979 ISBN 0-471-03032-5
    @see Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
    1982 ISBN 0-471-08936-2
*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  CLASS DECLARATION
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Quaternion representation of rotations.
 *
 * FGQuaternion is a representation of an arbitrary rotation through a
 * quaternion. It has vector properties as it is a 4-vector. This
 * class also contains access functions to the euler angle
 * representation of rotations and access to transformation matrices
 * for 3D vectors. Transformations and euler angles are therefore
 * computed once they are requested for the first time. Then they are
 * cached for later usage as long as the class is not accessed trough
 * a nonconst member function.
 */
class FGQuaternion : private FGJSBBase {
public:
  /** Default initializer.
   * Default initializer, initializes the class with the identity rotation.
   */
  FGQuaternion() : mQ0(1), mQ1(0), mQ2(0), mQ3(0), mCacheValid(false) {}

  /** Copy constructor.
   * Copy constructor, initializes the quaternion.
   * @param q  a constant reference to another FGQuaternion instance
   */
  FGQuaternion(const FGQuaternion& q);

  /** Inizializer by euler angles.
   * Initialize the quaternion with the euler angles.
   * @param phi The euler X axis (roll) angle in radians
   * @param tht The euler Y axis (attitude) angle in radians
   * @param psi The euler Z axis (heading) angle in radians
   */
  FGQuaternion(double phi, double tht, double psi);

  /// Destructor.
  ~FGQuaternion() {}

  /** Quaternion 'velocity' for given angular rates.
   *  Computes the quaternion derivative which results from the given
   *  angular velocities
   *  @param PQR a constant reference to the body rate vector
   */
  FGQuaternion GetQDot(const FGColumnVector3& PQR) const;

  /** Transformation matrix.
   *  Returns a reference to the transformation/rotation matrix
   *  corresponding to this quaternion rotation.
   */
  const FGMatrix33& GetT() const { ComputeDerived(); return mT; }

  /** Backward transformation matrix.
   *  Returns a reference to the backward transformation/rotation matrix
   *  corresponding to this quaternion rotation.
   */
  const FGMatrix33& GetTInv() const { ComputeDerived(); return mTInv; }

  /** Euler angles.
   *  Returns a reference to the 3-vector of euler angles corresponding
   *  to this quaternion rotation.
   */
  const FGColumnVector3& GetEuler() const {
    ComputeDerived();
    return mEulerAngles;
  }

  /** Euler angle theta.
   *  @return the euler angle theta (pitch attitude) in radians corresponding to this
   *  quaternion rotation.
   */
  double GetEulerTheta() const {
    ComputeDerived();
    return mEulerAngles(eTht);
  }

  /** Euler angle theta.
   *  @return the euler angle theta (pitch attitude) in degrees corresponding to
   *  this quaternion rotation.
   */
  double GetEulerThetaDeg() const {
    ComputeDerived();
    return radtodeg*mEulerAngles(eTht);
  }

  /** Euler angle psi.
   *  Returns the euler angle psi corresponding to this quaternion
   *  rotation.
   */
  double GetEulerPsi() const {
    ComputeDerived();
    return mEulerAngles(ePsi);
  }

  /** Euler angle psi.
   *  Returns the euler angle psi corresponding to this quaternion
   *  rotation.
   */
  double GetEulerPsiDeg() const {
    ComputeDerived();
    return radtodeg*mEulerAngles(ePsi);
  }

  /** Euler angle phi.
   *  Returns the euler angle phi corresponding to this quaternion
   *  rotation.
   */
  double GetEulerPhi() const {
    ComputeDerived();
    return mEulerAngles(ePhi);
  }

  /** Euler angle phi.
   *  Returns the euler angle phi corresponding to this quaternion
   *  rotation.
   */
  double GetEulerPhiDeg() const {
    ComputeDerived();
    return radtodeg*mEulerAngles(ePhi);
  }

  /** Sine of euler angle theta.
   *  Returns the sine of the euler angle theta corresponding to this
   *  quaternion rotation.
   */
  double GetSinEulerTheta() const {
    ComputeDerived();
    return mEulerSines(eTht);
  }

  /** Sine of euler angle psi.
   *  Returns the sine of the euler angle psi corresponding to this
   *  quaternion rotation.
   */
  double GetSinEulerPsi() const {
    ComputeDerived();
    return mEulerSines(ePsi);
  }

  /** Sine of euler angle phi.
   *  Returns the sine of the euler angle phi corresponding to this
   *  quaternion rotation.
   */
  double GetSinEulerPhi() const {
    ComputeDerived();
    return mEulerSines(ePhi);
  }

  /** Cosine of euler angle theta.
   *  Returns the cosine of the euler angle theta corresponding to this
   *  quaternion rotation.
   */
  double GetCosEulerTheta() const {
    ComputeDerived();
    return mEulerCosines(eTht);
  }

  /** Cosine of euler angle psi.
   *  Returns the cosine of the euler angle psi corresponding to this
   *  quaternion rotation.
   */
  double GetCosEulerPsi() const {
    ComputeDerived();
    return mEulerCosines(ePsi);
  }

  /** Cosine of euler angle phi.
   *  Returns the cosine of the euler angle phi corresponding to this
   *  quaternion rotation.
   */
  double GetCosEulerPhi() const {
    ComputeDerived();
    return mEulerCosines(ePhi);
  }

  /** Assignment operator.
   * Assign the value of q to the current object. Cached values are
   * conserved.
   */
  const FGQuaternion& operator=(const FGQuaternion& q) {
    // Copy the master values ...
    mQ0 = q.mQ0;
    mQ1 = q.mQ1;
    mQ2 = q.mQ2;
    mQ3 = q.mQ3;
    // .. and copy the derived values if they are valid
    mCacheValid = q.mCacheValid;
    if (mCacheValid) {
        mT = q.mT;
        mTInv = q.mTInv;
        mEulerAngles = q.mEulerAngles;
        mEulerSines = q.mEulerSines;
        mEulerCosines = q.mEulerCosines;
    }
    return *this;
  }

  /** Comparison operator.
   * Returns true if both quaternions represent the same rotation.
   */
  bool operator==(const FGQuaternion& q) const {
    return mQ0 == q.mQ0 && mQ1 == q.mQ1 && mQ2 == q.mQ2 && mQ3 == q.mQ3;
  }

  /** Comparison operator.
   * Returns true if both quaternions do not represent the same rotation.
   */
  bool operator!=(const FGQuaternion& q) const { return ! operator==(q); }
  const FGQuaternion& operator+=(const FGQuaternion& q) {
    // Copy the master values ...
    mQ0 += q.mQ0;
    mQ1 += q.mQ1;
    mQ2 += q.mQ2;
    mQ3 += q.mQ3;
    mCacheValid = false;
    return *this;
  }

  const FGQuaternion& operator-=(const FGQuaternion& q) {
    // Copy the master values ...
    mQ0 -= q.mQ0;
    mQ1 -= q.mQ1;
    mQ2 -= q.mQ2;
    mQ3 -= q.mQ3;
    mCacheValid = false;
    return *this;
  }

  const FGQuaternion& operator*=(double scalar) {
    mQ0 *= scalar;
    mQ1 *= scalar;
    mQ2 *= scalar;
    mQ3 *= scalar;
    mCacheValid = false;
    return *this;
  }

  const FGQuaternion& operator/=(double scalar) {
    return operator*=(1.0/scalar);
  }

  FGQuaternion operator+(const FGQuaternion& q) const {
    return FGQuaternion(mQ0+q.mQ0, mQ1+q.mQ1, mQ2+q.mQ2, mQ3+q.mQ3, false);
  }

  FGQuaternion operator-(const FGQuaternion& q) const {
    return FGQuaternion(mQ0-q.mQ0, mQ1-q.mQ1, mQ2-q.mQ2, mQ3-q.mQ3, false);
  }

  /** Multiplication of two quaternions.
   * For quaternions there is a multiplication defined. Multiplication
   * of two quaternions is like successive rotation.
   */
  FGQuaternion operator*(const FGQuaternion& q) const {
    return FGQuaternion(mQ0*q.mQ0-mQ1*q.mQ1-mQ2*q.mQ2-mQ3*q.mQ3,
                        mQ0*q.mQ1+mQ1*q.mQ0+mQ2*q.mQ3-mQ3*q.mQ2,
                        mQ0*q.mQ2-mQ1*q.mQ3+mQ2*q.mQ0+mQ3*q.mQ1,
                        mQ0*q.mQ3+mQ1*q.mQ2-mQ2*q.mQ1+mQ3*q.mQ0,
                        false);
  }

  /** Multiplication of two quaternions.
   * For quaternions there is a multiplication defined. Multiplication
   * of two quaternions is like successive rotation.
   */
  const FGQuaternion& operator*=(const FGQuaternion& q) {
    double q0 = mQ0*q.mQ0-mQ1*q.mQ1-mQ2*q.mQ2-mQ3*q.mQ3;
    double q1 = mQ0*q.mQ1+mQ1*q.mQ0+mQ2*q.mQ3-mQ3*q.mQ2;
    double q2 = mQ0*q.mQ2-mQ1*q.mQ3+mQ2*q.mQ0+mQ3*q.mQ1;
    double q3 = mQ0*q.mQ3+mQ1*q.mQ2-mQ2*q.mQ1+mQ3*q.mQ0;
    mQ0 = q0;
    mQ1 = q1;
    mQ2 = q2;
    mQ3 = q3;
    mCacheValid = false;
    return *this;
  }

  friend FGQuaternion operator*(double, const FGQuaternion&);

  /** Ties into the property tree.
   * Ties the variables represented by this class into the property tree.
   */
  void bind(FGPropertyManager*, const string&) const;

  /** Remove from property tree.
   * Unties the variables represented by this class into the property tree.
   */
  void unbind(FGPropertyManager*, const string&) const;

private:
  /** Copying by assigning the vector valued components.
   */
  FGQuaternion(double q0, double q1, double q2, double q3, bool valid)
    : mQ0(q0), mQ1(q1), mQ2(q2), mQ3(q3), mCacheValid(valid) {}

  /** Computation of derived values.
   * This function checks if the derived values like euler angles and
   * transformation matrices are already computed. If so, it
   * returns. If they need to be computed this is done here.
   */
  void ComputeDerived(void) const;

  /** The quaternion values itself. This is the master copy.
  *  The C++ keyword "mutable" tells the compiler that the data member is
  *  allowed to change during a const member function.
  */
  mutable double mQ0;
  mutable double mQ1;
  mutable double mQ2;
  mutable double mQ3;

  /** A data validity flag.
   * This class implements caching of the derived values like the
   * orthogonal rotation matrices or the Euler angles. For caching we
   * carry a flag which signals if the values are valid or not.
   */
  mutable bool mCacheValid;

  /** This carries the transformation matrices.
   */
  mutable FGMatrix33 mT;
  mutable FGMatrix33 mTInv;

  /** The cached euler angles.
   */
  mutable FGColumnVector3 mEulerAngles;
  /** The cached sines and cosines of the euler angles.
   */
  mutable FGColumnVector3 mEulerSines;
  mutable FGColumnVector3 mEulerCosines;
};

inline FGQuaternion operator*(double scalar, const FGQuaternion& q) {
  return FGQuaternion(scalar*q.mQ0, scalar*q.mQ1, scalar*q.mQ2, scalar*q.mQ3,
                      false);
}

} // namespace JSBSim

#endif
