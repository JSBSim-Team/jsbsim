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

FGMatrix::FGMatrix(const unsigned int r, const unsigned int c) : rows(r), cols(c)
{
  data = FGalloc(rows,cols);
  InitMatrix();
  rowCtr = colCtr = 1;
}

/******************************************************************************/

FGMatrix::FGMatrix(const FGMatrix& M)
{
  data  = NULL;
  rowCtr = colCtr = 1;
  *this = M;
}

/******************************************************************************/

FGMatrix::~FGMatrix(void)
{
  dealloc(data,rows);
  rowCtr = colCtr = 1;
  rows = cols = 0;
}

/******************************************************************************/

ostream& operator<<(ostream& os, const FGMatrix& M)
{
  for (unsigned int i=1; i<=M.Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      if (i == M.Rows() && j == M.Cols())
        os << M.data[i][j];
      else
        os << M.data[i][j] << ", ";
    }
  }
  return os;
}

/******************************************************************************/

FGMatrix& FGMatrix::operator<<(const float ff)
{
  data[rowCtr][colCtr] = ff;
  if (++colCtr > Cols()) {
    colCtr = 1;
    if (++rowCtr > Rows())
      rowCtr = 1;
  }
  return *this;
}

/******************************************************************************/

istream& operator>>(istream& is, FGMatrix& M)
{
  for (unsigned int i=1; i<=M.Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      is >> M.data[i][j];
    }
  }
  return is;
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
      Diff(i,j) = data[i][j] - M(i,j);
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
      data[i][j] -= M(i,j);
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
      Sum(i,j) = data[i][j] + M(i,j);
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
      data[i][j]+=M(i,j);
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
      data[i][j] *= scalar;
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
         Product(i,j) += data[i][k] * M(k,j);
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
        prod[i][j] += data[i][k] * M(k,j);
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
       Quot(i,j) = data[i][j]/scalar;
    }
  }
  return Quot;
}

/******************************************************************************/

void FGMatrix::operator/=(const double scalar)
{
  for (unsigned int i=1; i<=Rows(); i++)  {
    for (unsigned int j=1; j<=Cols(); j++) {
      data[i][j]/=scalar;
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

  for (unsigned int i=1; i<=Rows(); i++) Product(i) = scalar * data[i][1];

  return Product;
}

/******************************************************************************/

FGColumnVector FGColumnVector::operator-(const FGColumnVector& V)
{
  if ((Rows() != V.Rows()) || (Cols() != V.Cols())) {
    MatrixException mE;
    mE.Message = "Invalid row/column match in Column Vector operator -";
    throw mE;
  }

  FGColumnVector Diff(Rows());

  for (unsigned int i=1; i<=Rows(); i++) {
    Diff(i) = data[i][1] - V(i);
  }

  return Diff;
}

/******************************************************************************/

FGColumnVector FGColumnVector::operator/(const double scalar)
{
  FGColumnVector Quotient(Rows());

  for (unsigned int i=1; i<=Rows(); i++) Quotient(i) = data[i][1] / scalar;

  return Quotient;
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

/******************************************************************************/

float FGColumnVector::Magnitude(void)
{
  double num=0.0;

  if ((data[1][1] == 0.00) &&
      (data[2][1] == 0.00) &&
      (data[3][1] == 0.00))
  {
    return 0.00;
  } else {
    for (unsigned int i = 1; i<=Rows(); i++) num += data[i][1]*data[i][1];
    return sqrt(num);
  }
}

/******************************************************************************/

FGColumnVector FGColumnVector::Normalize(void)
{
  double Mag = Magnitude();

  for (unsigned int i=1; i<=Rows(); i++)
    for (unsigned int j=1; j<=Cols(); j++)
      data[i][j] = data[i][j]/Mag;

  return *this;
}

/******************************************************************************/

FGColumnVector FGColumnVector::operator*(const FGColumnVector& V)
{
  if (Rows() != 3 || V.Rows() != 3) {
    MatrixException mE;
    mE.Message = "Invalid row count in vector cross product function";
    throw mE;
  }

  FGColumnVector Product(3);

  Product(1) = data[2][1] * V(3) - data[3][1] * V(2);
  Product(2) = data[3][1] * V(1) - data[1][1] * V(3);
  Product(3) = data[1][1] * V(2) - data[2][1] * V(1);

  return Product;
}

/******************************************************************************/

