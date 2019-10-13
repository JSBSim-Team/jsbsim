/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGMatrix33.h
Author: Tony Peden, Jon Berndt, Mathias Frolich
Date started: Unknown

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

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
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing
03/06/2004 MF   Rework of the code to make it a bit compiler friendlier

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMATRIX33_H
#define FGMATRIX33_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <iosfwd>

#include "FGColumnVector3.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGQuaternion;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Exception convenience class.
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: MatrixException
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class MatrixException //: public FGJSBBase
{
public:
  std::string Message;
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

  /** Handles matrix math operations.
      @author Tony Peden, Jon Berndt, Mathias Froelich
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGMatrix33
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMatrix33
{
public:

  enum {
    eRows = 3,
    eColumns = 3
  };

  /** Default initializer.

      Create a zero matrix.
   */
  FGMatrix33(void);

  /** Copy constructor.

      @param M Matrix which is used for initialization.

      Create copy of the matrix given in the argument.
   */
  FGMatrix33(const FGMatrix33& M)
  {
    data[0] = M.data[0];
    data[1] = M.data[1];
    data[2] = M.data[2];
    data[3] = M.data[3];
    data[4] = M.data[4];
    data[5] = M.data[5];
    data[6] = M.data[6];
    data[7] = M.data[7];
    data[8] = M.data[8];
  }

  /** Initialization by given values.

      @param m11 value of the 1,1 Matrix element.
      @param m12 value of the 1,2 Matrix element.
      @param m13 value of the 1,3 Matrix element.
      @param m21 value of the 2,1 Matrix element.
      @param m22 value of the 2,2 Matrix element.
      @param m23 value of the 2,3 Matrix element.
      @param m31 value of the 3,1 Matrix element.
      @param m32 value of the 3,2 Matrix element.
      @param m33 value of the 3,3 Matrix element.

      Create a matrix from the doubles given in the arguments.
   */
  FGMatrix33(const double m11, const double m12, const double m13,
             const double m21, const double m22, const double m23,
             const double m31, const double m32, const double m33)
  {
    data[0] = m11;
    data[1] = m21;
    data[2] = m31;
    data[3] = m12;
    data[4] = m22;
    data[5] = m32;
    data[6] = m13;
    data[7] = m23;
    data[8] = m33;
  }

  /** Destructor.
   */
  ~FGMatrix33(void) {}

  /** Prints the contents of the matrix.
      @param delimeter the item separator (tab or comma)
      @return a string with the delimeter-separated contents of the matrix  */
  std::string Dump(const std::string& delimeter) const;

  /** Prints the contents of the matrix.
      @param delimeter the item separator (tab or comma, etc.)
      @param prefix an additional prefix that is used to indent the 3X3 matrix
             printout
      @return a string with the delimeter-separated contents of the matrix  */
  std::string Dump(const std::string& delimiter, const std::string& prefix) const;

  /** Read access the entries of the matrix.
      @param row Row index.
      @param col Column index.

      @return the value of the matrix entry at the given row and
      column indices. Indices are counted starting with 1.
   */
  double operator()(unsigned int row, unsigned int col) const {
    return data[(col-1)*eRows+row-1];
  }

  /** Write access the entries of the matrix.
      Note that the indices given in the arguments are unchecked.

      @param row Row index.
      @param col Column index.

      @return a reference to the matrix entry at the given row and
      column indices. Indices are counted starting with 1.
   */
  double& operator()(unsigned int row, unsigned int col) {
    return data[(col-1)*eRows+row-1];
  }

  /** Read access the entries of the matrix.
      This function is just a shortcut for the <tt>double&
      operator()(unsigned int row, unsigned int col)</tt> function. It is
      used internally to access the elements in a more convenient way.

      Note that the indices given in the arguments are unchecked.

      @param row Row index.
      @param col Column index.

      @return the value of the matrix entry at the given row and
      column indices. Indices are counted starting with 1.
   */
  double Entry(unsigned int row, unsigned int col) const {
    return data[(col-1)*eRows+row-1];
  }

  /** Write access the entries of the matrix.
      This function is just a shortcut for the <tt>double&
      operator()(unsigned int row, unsigned int col)</tt> function. It is
      used internally to access the elements in a more convenient way.

      Note that the indices given in the arguments are unchecked.

      @param row Row index.
      @param col Column index.

      @return a reference to the matrix entry at the given row and
      column indices. Indices are counted starting with 1.
   */
   double& Entry(unsigned int row, unsigned int col) {
     return data[(col-1)*eRows+row-1];
   }

  /** Number of rows in the matrix.
      @return the number of rows in the matrix.
   */
   unsigned int Rows(void) const { return eRows; }

  /** Number of cloumns in the matrix.
      @return the number of columns in the matrix.
   */
   unsigned int Cols(void) const { return eColumns; }

  /** Transposed matrix.
      This function only returns the transpose of this matrix. This matrix
      itself remains unchanged.
      @return the transposed matrix.
   */
  FGMatrix33 Transposed(void) const {
    return FGMatrix33( data[0], data[1], data[2],
                       data[3], data[4], data[5],
                       data[6], data[7], data[8] );
  }

  /** Transposes this matrix.
      This function only transposes this matrix. Nothing is returned.
   */
  void T(void);

/** Initialize the matrix.
    This function initializes a matrix to all 0.0.
 */
  void InitMatrix(void);

/** Initialize the matrix.
    This function initializes a matrix to user specified values.
 */
  void InitMatrix(const double m11, const double m12, const double m13,
                  const double m21, const double m22, const double m23,
                  const double m31, const double m32, const double m33)
  {
    data[0] = m11;
    data[1] = m21;
    data[2] = m31;
    data[3] = m12;
    data[4] = m22;
    data[5] = m32;
    data[6] = m13;
    data[7] = m23;
    data[8] = m33;
  }

  /** Returns the quaternion associated with this direction cosine (rotation) matrix.
  */
  FGQuaternion GetQuaternion(void) const;

  /** Returns the Euler angle column vector associated with this matrix.
  */
  FGColumnVector3 GetEuler() const;

  /** Determinant of the matrix.
      @return the determinant of the matrix.
   */
  double Determinant(void) const;

  /** Return if the matrix is invertible.
      Checks and returns if the matrix is nonsingular and thus
      invertible. This is done by simply computing the determinant and
      check if it is zero. Note that this test does not cover any
      instabilities caused by nearly singular matirces using finite
      arithmetics. It only checks exact singularity.
   */
  bool Invertible(void) const { return 0.0 != Determinant(); }

  /** Return the inverse of the matrix.
      Computes and returns if the inverse of the matrix. It is computed
      by Cramers Rule. Also there are no checks performed if the matrix
      is invertible. If you are not sure that it really is check this
      with the @ref Invertible() call before.
   */
  FGMatrix33 Inverse(void) const;

  /** Assignment operator.

      @param A source matrix.

      Copy the content of the matrix given in the argument into *this.
   */
  FGMatrix33& operator=(const FGMatrix33& A)
  {
    data[0] = A.data[0];
    data[1] = A.data[1];
    data[2] = A.data[2];
    data[3] = A.data[3];
    data[4] = A.data[4];
    data[5] = A.data[5];
    data[6] = A.data[6];
    data[7] = A.data[7];
    data[8] = A.data[8];
    return *this;
  }

  /** Assignment operator.

      @param lv initializer list of at most 9 values.

      Copy the content of the list into *this. */
  FGMatrix33& operator=(std::initializer_list<double> lv)
  {
    double *v = data;
    for(auto& x: lv) {
      *v = x;
      v += 3;
      if (v-data > 8)
        v -= 8;
    }

    return *this;
  }

  /** Matrix vector multiplication.

      @param v vector to multiply with.
      @return matric vector product.

      Compute and return the product of the current matrix with the
      vector given in the argument.
   */
  FGColumnVector3 operator*(const FGColumnVector3& v) const;

  /** Matrix subtraction.

      @param B matrix to add to.
      @return difference of the matrices.

      Compute and return the sum of the current matrix and the matrix
      B given in the argument.
  */
  FGMatrix33 operator-(const FGMatrix33& B) const;

  /** Matrix addition.

      @param B matrix to add to.
      @return sum of the matrices.

      Compute and return the sum of the current matrix and the matrix
      B given in the argument.
  */
  FGMatrix33 operator+(const FGMatrix33& B) const;

  /** Matrix product.

      @param B matrix to add to.
      @return product of the matrices.

      Compute and return the product of the current matrix and the matrix
      B given in the argument.
  */
  FGMatrix33 operator*(const FGMatrix33& B) const;

  /** Multiply the matrix with a scalar.

      @param scalar scalar factor to multiply with.
      @return scaled matrix.

      Compute and return the product of the current matrix with the
      scalar value scalar given in the argument.
  */
  FGMatrix33 operator*(const double scalar) const;

  /** Multiply the matrix with 1.0/scalar.

      @param scalar scalar factor to divide through.
      @return scaled matrix.

      Compute and return the product of the current matrix with the
      scalar value 1.0/scalar, where scalar is given in the argument.
  */
  FGMatrix33 operator/(const double scalar) const;

  /** In place matrix subtraction.

      @param B matrix to subtract.
      @return reference to the current matrix.

      Compute the diffence from the current matrix and the matrix B
      given in the argument.
  */
  FGMatrix33& operator-=(const FGMatrix33 &B);

  /** In place matrix addition.

      @param B matrix to add.
      @return reference to the current matrix.

      Compute the sum of the current matrix and the matrix B
      given in the argument.
  */
  FGMatrix33& operator+=(const FGMatrix33 &B);

  /** In place matrix multiplication.

      @param B matrix to multiply with.
      @return reference to the current matrix.

      Compute the product of the current matrix and the matrix B
      given in the argument.
  */
  FGMatrix33& operator*=(const FGMatrix33 &B);

  /** In place matrix scale.

      @param scalar scalar value to multiply with.
      @return reference to the current matrix.

      Compute the product of the current matrix and the scalar value scalar
      given in the argument.
  */
  FGMatrix33& operator*=(const double scalar);

  /** In place matrix scale.

      @param scalar scalar value to divide through.
      @return reference to the current matrix.

      Compute the product of the current matrix and the scalar value
      1.0/scalar, where scalar is given in the argument.
  */
  FGMatrix33& operator/=(const double scalar);

private:
  double data[eRows*eColumns];
};

/** Scalar multiplication.

    @param scalar scalar value to multiply with.
    @param A Matrix to multiply.

    Multiply the Matrix with a scalar value.
*/
inline FGMatrix33 operator*(double scalar, const FGMatrix33& A) {
  // use already defined operation.
  return A*scalar;
}

/** Write matrix to a stream.

    @param os Stream to write to.
    @param M Matrix to write.

    Write the matrix to a stream.
*/
std::ostream& operator<<(std::ostream& os, const FGMatrix33& M);

/** Read matrix from a stream.

    @param os Stream to read from.
    @param M Matrix to initialize with the values from the stream.

    Read matrix from a stream.
*/
std::istream& operator>>(std::istream& is, FGMatrix33& M);

} // namespace JSBSim
#endif
