/*******************************************************************************

Header: FGStateMatrix.h
Author: Jon Berndt
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------
04/06/2000 JSB  Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGSTATEMATRIX_H
#define FGSTATEMATRIX_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGMatrix.h"

/*******************************************************************************
FORWARD DECLARATIONS
*******************************************************************************/

class FGColumnVector;

/*******************************************************************************
DECLARATION: StateMatrixException
*******************************************************************************/

class StateMatrixException : public exception
{
public:
  string Message;
};

/*******************************************************************************
DECLARATION: FGStateMatrix
*******************************************************************************/

class FGStateMatrix : public FGMatrix
{
protected:
  double **data;

private:
  unsigned rows,cols;
  char delim;
  int width,prec,origin;
  void TransposeSquare(void);
  void TransposeNonSquare(void);

public:
  FGStateMatrix(void);
  FGStateMatrix(const FGStateMatrix& A);
  ~FGStateMatrix(void);

  FGStateMatrix& operator=(const FGStateMatrix& A);
  double& operator()(unsigned row, unsigned col) const;
  FGColumnVector operator*(const FGColumnVector& Col);

  unsigned FGStateMatrix::Rows(void) const;
  unsigned FGStateMatrix::Cols(void) const;

  void FGStateMatrix::T(void);
  void InitStateMatrix(void);
  void InitStateMatrix(double value);

  FGStateMatrix operator-(const FGStateMatrix& B);
  FGStateMatrix operator+(const FGStateMatrix& B);
  FGStateMatrix operator*(const FGStateMatrix& Right);
  FGStateMatrix operator/(const double scalar);

  void operator-=(const FGStateMatrix &B);
  void operator+=(const FGStateMatrix &B);
  void operator*=(const FGStateMatrix &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);

  friend FGStateMatrix operator*(double scalar,FGStateMatrix& A);

  void SetOParams(char delim,int width,int prec, int origin=0);
};

#endif
