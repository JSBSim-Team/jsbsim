/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGMatrix.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMATRIX_H
#define FGMATRIX_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <stdlib.h>
#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <fstream>
#    include <cmath>
#    include <iostream>
#  else
#    include <fstream.h>
#    include <math.h>
#    include <iostream.h>
#  endif
#else
#  include <fstream>
#  include <cmath>
#  include <iostream>
#endif

#include <string>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MATRIX "$Id: FGMatrix.h,v 1.22 2001/03/29 00:06:11 jberndt Exp $"

using std::string;
using std::ostream;
using std::istream;
using std::cerr;
using std::endl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: MatrixException
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class MatrixException /* :  public exception */
{
public:
  string Message;
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGMatrix
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMatrix
{
public:
  FGMatrix(unsigned int r, unsigned int c);
  FGMatrix(const FGMatrix& A);
  FGMatrix(void) {};
  ~FGMatrix(void);

  FGMatrix& operator=(const FGMatrix& A);
  inline double& operator()(unsigned int row, unsigned int col) const {return data[row][col];}

  FGColumnVector operator*(const FGColumnVector& Col);

  unsigned int Rows(void) const;
  unsigned int Cols(void) const;

  void T(void);
  void InitMatrix(void);
  void InitMatrix(double value);

  FGMatrix operator-(const FGMatrix& B);
  FGMatrix operator+(const FGMatrix& B);
  FGMatrix operator*(const FGMatrix& B);
  FGMatrix operator/(const double scalar);
  FGMatrix& operator<<(const float ff);

  friend ostream& operator<<(ostream& os, const FGMatrix& M);
  friend istream& operator>>(istream& is, FGMatrix& M);

  void operator-=(const FGMatrix &B);
  void operator+=(const FGMatrix &B);
  void operator*=(const FGMatrix &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);

  friend FGMatrix operator*(double scalar,FGMatrix& A);

  void SetOParams(char delim,int width,int prec, int origin=0);

protected:
  double **data;
  unsigned int rows,cols;

private:
  char delim;
  int width,prec,origin;
  void TransposeSquare(void);
  void TransposeNonSquare(void);
  unsigned int rowCtr, colCtr;
  void Debug(void);
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGColumnVector
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector : public FGMatrix
{
public:
  FGColumnVector(void);
  FGColumnVector(int m);
  FGColumnVector(const FGColumnVector& b);
  ~FGColumnVector(void);

  FGColumnVector operator*(const double scalar);
  FGColumnVector& operator*(const FGColumnVector& V);   // Cross product operator
  FGColumnVector operator/(const double scalar);
  FGColumnVector operator+(const FGColumnVector& B); // must not return reference
  FGColumnVector operator-(const FGColumnVector& B);
  float Magnitude(void);
  FGColumnVector Normalize(void);

  friend FGColumnVector operator*(const double scalar, const FGColumnVector& A);
  friend FGColumnVector operator*(const FGMatrix& M, const FGColumnVector& V);

  double& operator()(int m) const;
  
  FGColumnVector& multElementWise(const FGColumnVector& V);

private:
  void Debug(void);
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGMatrix3x3
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMatrix3x3 : public FGMatrix
{
public:
  FGMatrix3x3(void) {FGMatrix(3,3);}
//  ~FGMatrix3x3(void) {~FGMatrix();}
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGColumnVector4
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector4 : public FGColumnVector
{
public:
  FGColumnVector4(void) {FGColumnVector(4);}
//  ~FGColumnVector4(void) {~FGColumnVector();}
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGColumnVector3
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector3 : public FGColumnVector
{
public:
  FGColumnVector3(void) {FGColumnVector(3);}
//  ~FGColumnVector3(void) {~FGColumnVector();}
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
