/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGColumnVector3.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

 ------------- Copyright (C) 2001 by Tony Peden and Jon S. Berndt (jsb@hal-pc.org)

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
??/??/???? ??   Initial version and more.
03/06/2004 MF   Rework, document and do much inlineing.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGCOLUMNVECTOR3_H
#define FGCOLUMNVECTOR3_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>
#include <cmath>

using std::ostream;
using std::istream;
using std::cerr;
using std::cout;
using std::endl;
using std::sqrt;
using std::string;

#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_COLUMNVECTOR3 "$Id: FGColumnVector3.h,v 1.9 2008/07/22 02:42:17 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** This class implements a 3 element column vector.
    @author Jon S. Berndt, Tony Peden, et. al.
    @version $Id: FGColumnVector3.h,v 1.9 2008/07/22 02:42:17 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector3 : public FGJSBBase
{
public:
  /** Default initializer.
      Create a zero vector.   */
  FGColumnVector3(void);

  /** Initialization by given values.
      @param X value of the x-conponent.
      @param Y value of the y-conponent.
      @param Z value of the z-conponent.
      Create a vector from the doubles given in the arguments.   */
  FGColumnVector3(double X, double Y, double Z) {
    data[0] = X;
    data[1] = Y;
    data[2] = Z;
    Debug(0);
  }

  /** Copy constructor.
      @param v Vector which is used for initialization.
      Create copy of the vector given in the argument.   */
  FGColumnVector3(const FGColumnVector3& v) {
    data[0] = v.data[0];
    data[1] = v.data[1];
    data[2] = v.data[2];
    Debug(0);
  }

  /// Destructor.
  ~FGColumnVector3(void) { Debug(1); }

  /** Read access the entries of the vector.
      @param idx the component index.
      Return the value of the matrix entry at the given index.
      Indices are counted starting with 1.
      Note that the index given in the argument is unchecked.   */
  double operator()(unsigned int idx) const { return Entry(idx); }

  /** Write access the entries of the vector.
      @param idx the component index.
      Return a reference to the vector entry at the given index.
      Indices are counted starting with 1.
      Note that the index given in the argument is unchecked.   */
  double& operator()(unsigned int idx) { return Entry(idx); }

  /** Read access the entries of the vector.
      @param idx the component index.
      Return the value of the matrix entry at the given index.
      Indices are counted starting with 1.
      This function is just a shortcut for the <tt>double
      operator()(unsigned int idx) const</tt> function. It is
      used internally to access the elements in a more convenient way.
      Note that the index given in the argument is unchecked.   */
  double Entry(unsigned int idx) const { return data[idx-1]; }

  /** Write access the entries of the vector.
      @param idx the component index.
      Return a reference to the vector entry at the given index.
      Indices are counted starting with 1.
      This function is just a shortcut for the <tt>double&
      operator()(unsigned int idx)</tt> function. It is
      used internally to access the elements in a more convenient way.
      Note that the index given in the argument is unchecked.   */
  double& Entry(unsigned int idx) { return data[idx-1]; }

  /** Prints the contents of the vector
      @param delimeter the item separator (tab or comma)
      @return a string with the delimeter-separated contents of the vector  */
  string Dump(string delimeter) const;

  /** Assignment operator.
      @param b source vector.
      Copy the content of the vector given in the argument into *this.   */
  FGColumnVector3& operator=(const FGColumnVector3& b) {
    data[0] = b.data[0];
    data[1] = b.data[1];
    data[2] = b.data[2];
    return *this;
  }

  /**  Comparison operator.
      @param b other vector.
      Returns true if both vectors are exactly the same.   */
  bool operator==(const FGColumnVector3& b) const {
    return data[0] == b.data[0] && data[1] == b.data[1] && data[2] == b.data[2];
  }

  /** Comparison operator.
      @param b other vector.
      Returns false if both vectors are exactly the same.   */
  bool operator!=(const FGColumnVector3& b) const { return ! operator==(b); }

  /** Multiplication by a scalar.
      @param scalar scalar value to multiply the vector with.
      @return The resulting vector from the multiplication with that scalar.
      Multiply the vector with the scalar given in the argument.   */
  FGColumnVector3 operator*(const double scalar) const {
    return FGColumnVector3(scalar*Entry(1), scalar*Entry(2), scalar*Entry(3));
  }

  /** Multiply by 1/scalar.
      @param scalar scalar value to devide the vector through.
      @return The resulting vector from the division through that scalar.
      Multiply the vector with the 1/scalar given in the argument.   */
  FGColumnVector3 operator/(const double scalar) const;

  /** Cross product multiplication.
      @param V vector to multiply with.
      @return The resulting vector from the cross product multiplication.
      Compute and return the cross product of the current vector with
      the given argument.   */
  FGColumnVector3 operator*(const FGColumnVector3& V) const {
    return FGColumnVector3( Entry(2) * V(3) - Entry(3) * V(2),
                            Entry(3) * V(1) - Entry(1) * V(3),
                            Entry(1) * V(2) - Entry(2) * V(1) );
  }

  /// Addition operator.
  FGColumnVector3 operator+(const FGColumnVector3& B) const {
    return FGColumnVector3( Entry(1) + B(1), Entry(2) + B(2), Entry(3) + B(3) );
  }

  /// Subtraction operator.
  FGColumnVector3 operator-(const FGColumnVector3& B) const {
    return FGColumnVector3( Entry(1) - B(1), Entry(2) - B(2), Entry(3) - B(3) );
  }

  /// Subtract an other vector.
  FGColumnVector3& operator-=(const FGColumnVector3 &B) {
    Entry(1) -= B(1);
    Entry(2) -= B(2);
    Entry(3) -= B(3);
    return *this;
  }

  /// Add an other vector.
  FGColumnVector3& operator+=(const FGColumnVector3 &B) {
    Entry(1) += B(1);
    Entry(2) += B(2);
    Entry(3) += B(3);
    return *this;
  }

  /// Scale by a scalar.
  FGColumnVector3& operator*=(const double scalar) {
    Entry(1) *= scalar;
    Entry(2) *= scalar;
    Entry(3) *= scalar;
    return *this;
  }

  /// Scale by a 1/scalar.
  FGColumnVector3& operator/=(const double scalar);

  void InitMatrix(void) { data[0] = data[1] = data[2] = 0.0; }
  void InitMatrix(double a) { data[0] = data[1] = data[2] = a; }
  void InitMatrix(double a, double b, double c) {
    data[0]=a; data[1]=b; data[2]=c;
  }

  /** Length of the vector.
      Compute and return the euclidean norm of this vector.   */
  double Magnitude(void) const;

  /** Length of the vector in a coordinate axis plane.
      Compute and return the euclidean norm of this vector projected into
      the coordinate axis plane idx1-idx2.   */
  double Magnitude(int idx1, int idx2) const {
    return sqrt( Entry(idx1)*Entry(idx1) +  Entry(idx2)*Entry(idx2) );
  }

  /** Normalize.
      Normalize the vector to have the Magnitude() == 1.0. If the vector
      is equal to zero it is left untouched.   */
  FGColumnVector3& Normalize(void);

  // little trick here.
  struct AssignRef {
    AssignRef(FGColumnVector3& r, int i) : Ref(r), idx(i) {}
    AssignRef operator<<(const double ff) {
      Ref.Entry(idx) = ff;
      return AssignRef(Ref, idx+1);
    }
    FGColumnVector3& Ref;
    int idx;
  };
  AssignRef operator<<(const double ff) {
    Entry(1) = ff;
    return AssignRef(*this, 2);
  }

private:
  double data[3];

  void Debug(int from);
};

/** Scalar multiplication.
    @param scalar scalar value to multiply with.
    @param A Vector to multiply.
    Multiply the Vector with a scalar value.*/
inline FGColumnVector3 operator*(double scalar, const FGColumnVector3& A) {
  // use already defined operation.
  return A*scalar;
}

/** Write vector to a stream.
    @param os Stream to write to.
    @param M Matrix to write.
    Write the matrix to a stream.*/
ostream& operator<<(ostream& os, const FGColumnVector3& col);

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
