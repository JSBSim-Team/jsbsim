/*******************************************************************************

Module: FGStateMatrix.cpp
Author: Jon Berndt
Purpose: FGStateMatrix class
Called by: Various

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
A 3 X 3 matrix used to convert from Local frame to Body and back. This matrix
also contains a quaternion representation. The "state" - i.e. both matrix and
quaternion vector - are updated every iteration by supplying body rates, P, Q, and
R.

HISTORY
--------------------------------------------------------------------------------
04/06/2000 JSB  Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGStateMatrix.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

double** FGalloc(void)
{
  double **A;

  A = new double *[4];
  if (!A)  return NULL;

  for (int i=0; i <= 3; i++){
    A[i] = new double [4];
    if (!A[i]) return NULL;
  }
  return A;
}

/******************************************************************************/

void dealloc(double **A)
{
  for(int i=0; i<=3; i++) delete[] A[i];
  delete[] A;
}

/******************************************************************************/

FGStateMatrix::FGStateMatrix(void) : FGMatrix(3, 3)
{
  this->rows = 3;
  this->cols = 3;
  data = FGalloc();
}

/******************************************************************************/

FGStateMatrix::FGStateMatrix(const FGStateMatrix& M) : FGMatrix(M)
{
  data  = NULL;
  *this = M;
}

/******************************************************************************/

FGStateMatrix::~FGStateMatrix(void)
{
  dealloc(data);
  rows=cols=0;
}

/******************************************************************************/

FGStateMatrix& FGStateMatrix::operator=(const FGStateMatrix& M)
{
  if (&M != this) {
    if (data != NULL) dealloc(data);

    width  = M.width;
    prec   = M.prec;
    delim  = M.delim;
    origin = M.origin;
    rows   = M.rows;
    cols   = M.cols;

    data = FGalloc();
    for (unsigned int i=0; i<=rows; i++) {
      for (unsigned int j=0; j<=cols; j++) {
        data[i][j] = M.data[i][j];
      }
    }
  }
  return *this;
}

/******************************************************************************/

double& FGStateMatrix::operator()(unsigned row, unsigned col) const
{
  return data[row][col];
}

/******************************************************************************/

unsigned FGStateMatrix::Rows(void) const
{
  return rows;
}

/******************************************************************************/

unsigned FGStateMatrix::Cols(void) const
{
  return cols;
}

/******************************************************************************/

void FGStateMatrix::SetOParams(char delim,int width,int prec,int origin)
{
  FGStateMatrix::delim  = delim;
  FGStateMatrix::width  = width;
  FGStateMatrix::prec   = prec;
  FGStateMatrix::origin = origin;
}

/******************************************************************************/

void FGStateMatrix::InitStateMatrix(double value)
{
  if (data) {
    for (unsigned int i=0;i<=rows;i++) {
      for (unsigned int j=0;j<=cols;j++) {
        operator()(i,j) = value;
      }
    }
  }
}

/******************************************************************************/

void FGStateMatrix::InitStateMatrix(void)
{
  this->InitStateMatrix(0);
}

// *****************************************************************************
// binary operators ************************************************************
// *****************************************************************************

FGStateMatrix FGStateMatrix::operator-(const FGStateMatrix& M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    StateMatrixException mE;
    mE.Message = "Invalid row/column match in StateMatrix operator -";
    throw mE;
  }

  FGStateMatrix Diff;

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      Diff(i,j) = (*this)(i,j) - M(i,j);
    }
  }
  return Diff;
}

/******************************************************************************/

void FGStateMatrix::operator-=(const FGStateMatrix &M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    StateMatrixException mE;
    mE.Message = "Invalid row/column match in StateMatrix operator -=";
    throw mE;
  }

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j) -= M(i,j);
    }
  }
}

/******************************************************************************/

FGStateMatrix FGStateMatrix::operator+(const FGStateMatrix& M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    StateMatrixException mE;
    mE.Message = "Invalid row/column match in StateMatrix operator +";
    throw mE;
  }

  FGStateMatrix Sum;

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      Sum(i,j) = (*this)(i,j) + M(i,j);
    }
  }
  return Sum;
}

/******************************************************************************/

void FGStateMatrix::operator+=(const FGStateMatrix &M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    StateMatrixException mE;
    mE.Message = "Invalid row/column match in StateMatrix operator +=";
    throw mE;
  }

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j)+=M(i,j);
    }
  }
}

/******************************************************************************/

FGStateMatrix operator*(double scalar, FGStateMatrix &M)
{
  FGStateMatrix Product;

  for (unsigned int i=1; i<=M.Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      Product(i,j) = scalar*M(i,j);
    }
  }
  return Product;
}

/******************************************************************************/

void FGStateMatrix::operator*=(const double scalar)
{
  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j) *= scalar;
    }
  }
}

/******************************************************************************/

FGStateMatrix FGStateMatrix::operator*(const FGStateMatrix& M)
{
  if (Cols() != M.Rows()) {
    StateMatrixException mE;
    mE.Message = "Invalid row/column match in StateMatrix operator *";
    throw mE;
  }

  FGStateMatrix Product;

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++)  {
      Product(i,j) = 0;
      for (unsigned int k=1; k<=Cols(); k++) {
         Product(i,j) += (*this)(i,k) * M(k,j);
      }
    }
  }
  return Product;
}

/******************************************************************************/

void FGStateMatrix::operator*=(const FGStateMatrix& M)
{
  if (Cols() != M.Rows()) {
    StateMatrixException mE;
    mE.Message = "Invalid row/column match in StateMatrix operator *=";
    throw mE;
  }

  double **prod;

  prod = FGalloc();
  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      prod[i][j] = 0;
      for (unsigned int k=1; k<=Cols(); k++) {
        prod[i][j] += (*this)(i,k) * M(k,j);
      }
    }
  }
  dealloc(data);
  data = prod;
  cols = M.cols;
}

/******************************************************************************/

FGStateMatrix FGStateMatrix::operator/(const double scalar)
{
  FGStateMatrix Quot;

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++)  {
       Quot(i,j) = (*this)(i,j)/scalar;
    }
  }
  return Quot;
}

/******************************************************************************/

void FGStateMatrix::operator/=(const double scalar)
{
  for (unsigned int i=1; i<=Rows(); i++)  {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j)/=scalar;
    }
  }
}

/******************************************************************************/

void FGStateMatrix::T(void)
{
  if (rows==cols)
    TransposeSquare();
  else
    TransposeNonSquare();
}

/******************************************************************************/

FGColumnVector FGStateMatrix::operator*(const FGColumnVector& Col)
{
  FGColumnVector Product(Col.Rows());

  if (Cols() != Col.Rows()) {
    StateMatrixException mE;
    mE.Message = "Invalid row/column match in Column Vector operator *";
    throw mE;
  }

  for (unsigned int i=1;i<=Rows();i++) {
    Product(i) = 0.00;
    for (unsigned int j=1;j<=Cols();j++) {
      Product(i) += Col(j)*data[i][j];
    }
  }
  return Product;
}

/******************************************************************************/

void FGStateMatrix::TransposeSquare(void)
{
  for (unsigned int i=1; i<=rows; i++) {
    for (unsigned int j=i+1; j<=cols; j++) {
      double tmp = data[i][j];
      data[i][j] = data[j][i];
      data[j][i] = tmp;
    }
  }
}

/******************************************************************************/

void FGStateMatrix::TransposeNonSquare(void)
{
  double **tran;

  tran = FGalloc();

  for (unsigned int i=1; i<=rows; i++) {
    for (unsigned int j=1; j<=cols; j++) {
      tran[j][i] = data[i][j];
    }
  }

  dealloc(data);

  data       = tran;
  unsigned m = rows;
  rows       = cols;
  cols       = m;
}

/******************************************************************************/

