/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGMatrix33.h
Author: Tony Peden, Jon Berndt, Mathias Frolich
Date started: Unknown

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

#include <stdlib.h>
#ifdef FGFS
#  include <math.h>
#  include <simgear/compiler.h>
#  include STL_STRING
#  include STL_FSTREAM
#  include STL_IOSTREAM
   SG_USING_STD(string);
   SG_USING_STD(ostream);
   SG_USING_STD(istream);
   SG_USING_STD(cerr);
   SG_USING_STD(cout);
   SG_USING_STD(endl);
#else
#  include <string>
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
     include <fstream.h>
     include <iostream.h>
#    include <math.h>
#  else
#    include <fstream>
#    include <iostream>
#    if defined(sgi) && !defined(__GNUC__)
#      include <math.h>
#    else
#      include <cmath>
#    endif
     using std::ostream;
     using std::istream;
     using std::cerr;
     using std::cout;
     using std::endl;
#  endif
   using std::string;
#endif

#include "FGColumnVector3.h"
#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MATRIX33 "$Id: FGMatrix33.h,v 1.22 2004/03/06 23:47:16 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGColumnVector3;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Exception convenience class.
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: MatrixException
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class MatrixException : public FGJSBBase
{
public:
  string Message;
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

  /** Handles matrix math operations.
      @author Tony Peden, Jon Berndt, Mathias Frolich
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGMatrix33
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMatrix33 : public FGJSBBase
{
public:

  enum {
    eRows = 3,
    eColumns = 3
  };

  /** Default initializer.
   *
   * Create a zero matrix.
   */
  FGMatrix33(void);

  /** Copy constructor.
   *
   * \param M Matrix which is used for initialization.
   *
   * Create copy of the matrix given in the argument.
   */
  FGMatrix33(const FGMatrix33& M) {
    Entry(1,1) = M.Entry(1,1);
    Entry(2,1) = M.Entry(2,1);
    Entry(3,1) = M.Entry(3,1);
    Entry(1,2) = M.Entry(1,2);
    Entry(2,2) = M.Entry(2,2);
    Entry(3,2) = M.Entry(3,2);
    Entry(1,3) = M.Entry(1,3);
    Entry(2,3) = M.Entry(2,3);
    Entry(3,3) = M.Entry(3,3);

    Debug(0);
  }

  /** Initialization by given values.
   *
   * \param m11 value of the 1,1 Matrix element.
   * \param m12 value of the 1,2 Matrix element.
   * \param m13 value of the 1,3 Matrix element.
   * \param m21 value of the 2,1 Matrix element.
   * \param m22 value of the 2,2 Matrix element.
   * \param m23 value of the 2,3 Matrix element.
   * \param m31 value of the 3,1 Matrix element.
   * \param m32 value of the 3,2 Matrix element.
   * \param m33 value of the 3,3 Matrix element.
   *
   * Create a matrix from the doubles given in the arguments.
   */
  FGMatrix33(double m11, double m12, double m13,
             double m21, double m22, double m23,
             double m31, double m32, double m33) {
    Entry(1,1) = m11;
    Entry(2,1) = m21;
    Entry(3,1) = m31;
    Entry(1,2) = m12;
    Entry(2,2) = m22;
    Entry(3,2) = m32;
    Entry(1,3) = m13;
    Entry(2,3) = m23;
    Entry(3,3) = m33;

    Debug(0);
  }

  /** Destructor.
   */
  ~FGMatrix33(void) { Debug(1); }

  /** Read access the entries of the matrix.
   \param row Row index.
   \param col Column index.

   \return the value of the matrix entry at the given row and
   column indices. Indices are counted starting with 1.
   */
  double operator()(unsigned int row, unsigned int col) const {
    return Entry(row, col);
  }

  /** Write access the entries of the matrix.
   * Note that the indices given in the arguments are unchecked.
   *
   * \param row Row index.
   * \param col Column index.
   *
   * \return a reference to the matrix entry at the given row and
   * column indices. Indices are counted starting with 1.
   */
  double& operator()(unsigned int row, unsigned int col) {
    return Entry(row, col);
  }

  /** Read access the entries of the matrix.
   * This function is just a shortcut for the \ref double&
   * operator()(unsigned int row, unsigned int col) function. It is
   * used internally to access the elements in a more convenient way.
   *
   * Note that the indices given in the arguments are unchecked.
   *
   * \param row Row index.
   * \param col Column index.
   *
   * \return the value of the matrix entry at the given row and
   * column indices. Indices are counted starting with 1.
   */
  double Entry(unsigned int row, unsigned int col) const {
    return data[(col-1)*eRows+row-1];
  }

  /** Write access the entries of the matrix.
   * This function is just a shortcut for the \ref double&
   * operator()(unsigned int row, unsigned int col) function. It is
   * used internally to access the elements in a more convenient way.
   *
   * Note that the indices given in the arguments are unchecked.
   *
   * \param row Row index.
   * \param col Column index.
   *
   * \return a reference to the matrix entry at the given row and
   * column indices. Indices are counted starting with 1.
   */
   double& Entry(unsigned int row, unsigned int col) {
     return data[(col-1)*eRows+row-1];
   }

   /** Number of rows in the matrix.
    *
    * Return the number of rows in the matrix.
    */
   unsigned int Rows(void) const { return eRows; }

   /** Number of cloumns in the matrix.
    *
    * Return the number of columns in the matrix.
    */
   unsigned int Cols(void) const { return eColumns; }

  /** Transposed matrix.
   *
   * Return the transposed matrix.
   */
  FGMatrix33 Transposed(void) const {
    return FGMatrix33( Entry(1,1), Entry(2,1), Entry(3,1),
                       Entry(1,2), Entry(2,2), Entry(3,2),
                       Entry(1,3), Entry(2,3), Entry(3,3) );
  }

  // Not shure of these. Provided for compatibility for now.
  void T(void);
  void InitMatrix(void);
  void InitMatrix(double m11, double m12, double m13,
                  double m21, double m22, double m23,
                  double m31, double m32, double m33) {
    Entry(1,1) = m11;
    Entry(2,1) = m21;
    Entry(3,1) = m31;
    Entry(1,2) = m12;
    Entry(2,2) = m22;
    Entry(3,2) = m32;
    Entry(1,3) = m13;
    Entry(2,3) = m23;
    Entry(3,3) = m33;
  }

  /** Determinant of the matrix.
   *
   * Compute and return the determinant of the matrix.
   */
  double Determinant(void) const;

  /** Return if the matrix is invertible.
   * Checks and returns if the matrix is nonsingular and thus
   * invertible. This is done by simply computing the determinant and
   * check if it is zero. Note that this test does not cover any
   * instabilities caused by nearly singular matirces using finite
   * arithmetics. It only checks exact singularity.
   */
  bool Invertible(void) const { return 0.0 != Determinant(); }

  /** Return the inverse of the matrix.
   *
   * Computes and returns if the inverse of the matrix. It is computed
   * by Cramers Rule. Also there are no checks performed if the matrix
   * is invertible. If you are not sure that it really is check this
   * with the \ref Invertible() call before.
   */
  FGMatrix33 Inverse(void) const;

  /** Assignment operator.
   *
   * \param A source matrix.
   *
   * Copy the content of the matrix given in the argument into *this.
   */
  FGMatrix33& operator=(const FGMatrix33& A) {
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

  /** Matrix vector multiplication.
   *
   * \param v vector to multiply with.
   *
   * Compute and return the matrix vector ptoduct (*this)*v.
   */
  FGColumnVector3 operator*(const FGColumnVector3& v) const;

  // FIXME: write documentation.
  FGMatrix33 operator-(const FGMatrix33& B) const;
  FGMatrix33 operator+(const FGMatrix33& B) const;
  FGMatrix33 operator*(const FGMatrix33& B) const;
  FGMatrix33 operator*(const double scalar) const;
  FGMatrix33 operator/(const double scalar) const;

  FGMatrix33& operator-=(const FGMatrix33 &B);
  FGMatrix33& operator+=(const FGMatrix33 &B);
  FGMatrix33& operator*=(const FGMatrix33 &B);
  FGMatrix33& operator*=(const double scalar);
  FGMatrix33& operator/=(const double scalar);

private:
  double data[eRows*eColumns];

  void Debug(int from);
};

/** Write matrix to a stream.
 *
 * \param os Stream to write to.
 * \param M Matrix to write.
 *
 * Write the matrix to a stream.
 */
ostream& operator<<(ostream& os, const FGMatrix33& M);

/** Read matrix from a stream.
 *
 * \param os Stream to read from.
 * \param M Matrix to initialize with the values from the stream.
 *
 * Read matrix from a stream.
 */
istream& operator>>(istream& is, FGMatrix33& M);

}
#endif
