/*******************************************************************************

Header: FGMatrix.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGMATRIX_H
#define FGMATRIX_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include <stdlib.h>
#ifdef FGFS
#  include <Include/compiler.h>
#  include STL_STRING
#  ifdef FG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
   FG_USING_STD(string);
#else
#  include <string>
#  include <fstream>
#endif

/*******************************************************************************
FORWARD DECLARATIONS
*******************************************************************************/

class FGColumnVector;

/*******************************************************************************
DECLARATION: MatrixException
*******************************************************************************/

using namespace std;

class MatrixException /* :  public exception */  
{
public:
  string Message;
};

/*******************************************************************************
DECLARATION: FGMatrix
*******************************************************************************/

class FGMatrix
{
protected:
  double **data;

private:
  unsigned int rows,cols;
  char delim;
  int width,prec,origin;
  void TransposeSquare(void);
  void TransposeNonSquare(void);
  unsigned int rowCtr, colCtr;

public:
  FGMatrix(unsigned int r, unsigned int c);
  FGMatrix(const FGMatrix& A);
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
};

/*******************************************************************************
DECLARATION: FGColumnVector
*******************************************************************************/

class FGColumnVector : public FGMatrix
{
public:
  FGColumnVector(void);
  FGColumnVector(int m);
  FGColumnVector(const FGColumnVector& b);
  ~FGColumnVector();

  FGColumnVector operator*(const double scalar);
  FGColumnVector operator/(const double scalar);
  FGColumnVector operator+(const FGColumnVector& B);
  float Magnitude(void); 
  FGColumnVector Normalize(void);

  friend FGColumnVector operator*(const double scalar, const FGColumnVector& A);

  double& operator()(int m) const;
};

/******************************************************************************/
#endif
