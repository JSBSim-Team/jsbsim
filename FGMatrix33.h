/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGMatrix33.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing

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

#define ID_MATRIX33 "$Id: FGMatrix33.h,v 1.21 2004/03/06 18:49:01 jberndt Exp $"

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
   * \param A Matrix which is used for initialization.
   *
   * Creat copy of the matrix given in the argument.
   */
  FGMatrix33(const FGMatrix33& A);

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
   * Creat a matrix from the doubles given in the arguments.
   */
  FGMatrix33(double m11, double m12, double m13,
             double m21, double m22, double m23,
             double m31, double m32, double m33);

  /** Destructor.
   */
  ~FGMatrix33(void);

  // ???
  FGMatrix33(int r, int c);


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

  void T(void);
  void InitMatrix(void);
  void InitMatrix(double value);
  void InitMatrix(double x1, double x2, double x3,
                  double y1, double y2, double y3,
                  double z1, double z2, double z3);

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

  FGMatrix33& operator=(const FGMatrix33& A);

  FGColumnVector3 operator*(const FGColumnVector3& Col) const;

  FGMatrix33 operator-(const FGMatrix33& B) const;
  FGMatrix33 operator+(const FGMatrix33& B) const;
  FGMatrix33 operator*(const FGMatrix33& B) const;
  FGMatrix33 operator*(const double scalar) const;
  FGMatrix33 operator/(const double scalar) const;
  FGMatrix33& operator<<(const double ff);

  void operator-=(const FGMatrix33 &B);
  void operator+=(const FGMatrix33 &B);
  void operator*=(const FGMatrix33 &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);

protected:
  double data[eRows*eColumns];

private:
  unsigned int rowCtr, colCtr;
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
