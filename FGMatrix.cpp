/*******************************************************************************

Module: FGMatrix.cpp
Author: Originally by Tony Peden [formatted here (and broken??) by JSB]
Date started: 1998
Purpose: FGMatrix class
Called by: Various

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGMatrix.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

double** FGalloc(int rows, int cols)
{
  double **A;

  A = new double *[rows+1];
  if (!A)  return NULL;

  for (int i=0; i <= rows; i++){
    A[i] = new double [cols+1];
    if (!A[i]) return NULL;
  }
  return A;
}

/******************************************************************************/

void dealloc(double **A, int rows)
{
  for(int i=0;i<= rows;i++) delete[] A[i];
  delete[] A;
}

/******************************************************************************/

FGMatrix::FGMatrix(unsigned int rows, unsigned int cols)
{
  this->rows = rows;
  this->cols = cols;
  data = FGalloc(rows,cols);
}

/******************************************************************************/

FGMatrix::FGMatrix(const FGMatrix& M)
{
  data  = NULL;
  *this = M;
}

/******************************************************************************/

FGMatrix::~FGMatrix(void)
{
  dealloc(data,rows);
  rows=cols=0;
}

/******************************************************************************/

FGMatrix& FGMatrix::operator=(const FGMatrix& M)
{
  if (&M != this) {
    if (data != NULL) dealloc(data,rows);

    width  = M.width;
    prec   = M.prec;
    delim  = M.delim;
    origin = M.origin;
    rows   = M.rows;
    cols   = M.cols;

    data = FGalloc(rows,cols);
    for (unsigned int i=0; i<=rows; i++) {
      for (unsigned int j=0; j<=cols; j++) {
        data[i][j] = M.data[i][j];
      }
    }
  }
  return *this;
}

/******************************************************************************/

double& FGMatrix::operator()(unsigned int row, unsigned int col) const
{
  return data[row][col];
}

/******************************************************************************/

unsigned int FGMatrix::Rows(void) const
{
  return rows;
}

/******************************************************************************/

unsigned int FGMatrix::Cols(void) const
{
  return cols;
}

/******************************************************************************/

void FGMatrix::SetOParams(char delim,int width,int prec,int origin)
{
  FGMatrix::delim  = delim;
  FGMatrix::width  = width;
  FGMatrix::prec   = prec;
  FGMatrix::origin = origin;
}

/******************************************************************************/

void FGMatrix::InitMatrix(double value)
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

void FGMatrix::InitMatrix(void)
{
  this->InitMatrix(0);
}

// *****************************************************************************
// binary operators ************************************************************
// *****************************************************************************

FGMatrix FGMatrix::operator-(const FGMatrix& M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Matrix operator -";
    throw mE;
  }

  FGMatrix Diff(Rows(), Cols());

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      Diff(i,j) = (*this)(i,j) - M(i,j);
    }
  }
  return Diff;
}

/******************************************************************************/

void FGMatrix::operator-=(const FGMatrix &M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Matrix operator -=";
    throw mE;
  }

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j) -= M(i,j);
    }
  }
}

/******************************************************************************/

FGMatrix FGMatrix::operator+(const FGMatrix& M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Matrix operator +";
    throw mE;
  }

  FGMatrix Sum(Rows(), Cols());

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      Sum(i,j) = (*this)(i,j) + M(i,j);
    }
  }
  return Sum;
}

/******************************************************************************/

void FGMatrix::operator+=(const FGMatrix &M)
{
  if ((Rows() != M.Rows()) || (Cols() != M.Cols())) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Matrix operator +=";
    throw mE;
  }

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j)+=M(i,j);
    }
  }
}

/******************************************************************************/

FGMatrix operator*(double scalar, FGMatrix &M)
{
  FGMatrix Product(M.Rows(), M.Cols());

  for (unsigned int i=1; i<=M.Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      Product(i,j) = scalar*M(i,j);
    }
  }
  return Product;
}

/******************************************************************************/

void FGMatrix::operator*=(const double scalar)
{
  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j) *= scalar;
    }
  }
}

/******************************************************************************/

FGMatrix FGMatrix::operator*(const FGMatrix& M)
{
  if (Cols() != M.Rows()) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Matrix operator *";
    throw mE;
  }

  FGMatrix Product(Rows(), M.Cols());

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

void FGMatrix::operator*=(const FGMatrix& M)
{
  if (Cols() != M.Rows()) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Matrix operator *=";
    throw mE;
  }

  double **prod;

  prod = FGalloc(Rows(), M.Cols());
  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      prod[i][j] = 0;
      for (unsigned int k=1; k<=Cols(); k++) {
        prod[i][j] += (*this)(i,k) * M(k,j);
      }
    }
  }
  dealloc(data, Rows());
  data = prod;
  cols = M.cols;
}

/******************************************************************************/

FGMatrix FGMatrix::operator/(const double scalar)
{
  FGMatrix Quot(Rows(), Cols());

  for (unsigned int i=1; i<=Rows(); i++) {
    for (unsigned int j=1; j<=Cols(); j++)  {
       Quot(i,j) = (*this)(i,j)/scalar;
    }
  }
  return Quot;
}

/******************************************************************************/

void FGMatrix::operator/=(const double scalar)
{
  for (unsigned int i=1; i<=Rows(); i++)  {
    for (unsigned int j=1; j<=Cols(); j++) {
      (*this)(i,j)/=scalar;
    }
  }
}

/******************************************************************************/

void FGMatrix::T(void)
{
  if (rows==cols)
    TransposeSquare();
  else
    TransposeNonSquare();
}

/******************************************************************************/

FGColumnVector FGMatrix::operator*(const FGColumnVector& Col)
{
  FGColumnVector Product(Col.Rows());

  if (Cols() != Col.Rows()) {
    MatrixException mE;
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

void FGMatrix::TransposeSquare(void)
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

void FGMatrix::TransposeNonSquare(void)
{
  double **tran;

  tran = FGalloc(rows,cols);

  for (unsigned int i=1; i<=rows; i++) {
    for (unsigned int j=1; j<=cols; j++) {
      tran[j][i] = data[i][j];
    }
  }

  dealloc(data,rows);

  data       = tran;
  unsigned int m = rows;
  rows       = cols;
  cols       = m;
}

/******************************************************************************/

FGColumnVector::FGColumnVector(void):FGMatrix(3,1) { }
FGColumnVector::FGColumnVector(int m):FGMatrix(m,1) { }
FGColumnVector::FGColumnVector(const FGColumnVector& b):FGMatrix(b) { }
FGColumnVector::~FGColumnVector() { }

/******************************************************************************/

double& FGColumnVector::operator()(int m) const
{
  return FGMatrix::operator()(m,1);
}

/******************************************************************************/

FGColumnVector operator*(const FGMatrix& Mat, const FGColumnVector& Col)
{
  FGColumnVector Product(Col.Rows());

  if (Mat.Cols() != Col.Rows()) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Column Vector operator *";
    throw mE;
  }

  for (unsigned int i=1;i<=Mat.Rows();i++) {
    Product(i) = 0.00;
    for (unsigned int j=1;j<=Mat.Cols();j++) {
      Product(i) += Col(j)*Mat(i,j);
    }
  }

  return Product;
}

/******************************************************************************/

FGColumnVector FGColumnVector::operator+(const FGColumnVector& C)
{
  if (Rows() != C.Rows()) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Column Vector operator *";
    throw mE;
  }

  FGColumnVector Sum(C.Rows());

  for (unsigned int i=1; i<=C.Rows(); i++) {
    Sum(i) = C(i) + data[i][1];
  }

  return Sum;
}

/******************************************************************************/

FGColumnVector FGColumnVector::operator*(const double scalar)
{
  FGColumnVector Product(Rows());

  for (unsigned int i=1; i<=Rows(); i++) {
     Product(i) = scalar * data[i][1];
  }

  return Product;
}

/******************************************************************************/

FGColumnVector operator*(const double scalar, const FGColumnVector& C)
{
  FGColumnVector Product(C.Rows());

  for (unsigned int i=1; i<=C.Rows(); i++) {
     Product(i) = scalar * C(i);
  }

  return Product;
}

