/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGMatrix33.cpp
Author: Originally by Tony Peden [formatted here (and broken??) by JSB]
Date started: 1998
Purpose: FGMatrix33 class
Called by: Various

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGMatrix33.h"
#include "FGColumnVector3.h"

static const char *IdSrc = "$Id: FGMatrix33.cpp,v 1.1 2001/07/23 12:46:03 apeden Exp $";
static const char *IdHdr = ID_MATRIX;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

double** FGalloc(void)
{
  double **A;

  A = new double *[4];
  if (!A)  return NULL;

  A[0] = new double [4];
  if (!A[0]) return NULL;
  A[1] = new double [4];
  if (!A[1]) return NULL;
  A[2] = new double [4];
  if (!A[2]) return NULL;
  A[3] = new double [4];
  if (!A[3]) return NULL;
  
  return A;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void dealloc(double **A)
{
  delete[] A[0];
  delete[] A[1];
  delete[] A[2];
  delete[] A[3];
 
  delete[] A;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(void)
{
  data=FGalloc();
  InitMatrix();
  rowCtr = colCtr = 1;
  
  if (debug_lvl & 2) cout << "Instantiated: FGMatrix33" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(const FGMatrix33& M)
{
  rowCtr = colCtr = 1;
  *this = M;

  if (debug_lvl & 2) cout << "Instantiated: FGMatrix33" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::~FGMatrix33(void)
{
  dealloc(data);
  rowCtr = colCtr = 1;

  if (debug_lvl & 2) cout << "Destroyed:    FGMatrix33" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ostream& operator<<(ostream& os, const FGMatrix33& M)
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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator<<(const float ff)
{
  data[rowCtr][colCtr] = ff;
  if (++colCtr > Cols()) {
    colCtr = 1;
    if (++rowCtr > Rows())
      rowCtr = 1;
  }
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

istream& operator>>(istream& is, FGMatrix33& M)
{
  for (unsigned int i=1; i<=M.Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      is >> M.data[i][j];
    }
  }
  return is;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator=(const FGMatrix33& M)
{
  if (&M != this) {
    if (data != NULL) dealloc(data);

    width  = M.width;
    prec   = M.prec;
    delim  = M.delim;
    origin = M.origin;
    
    data=FGalloc();
    
    data[1][1] = M.data[1][1];
    data[1][2] = M.data[1][2];
    data[1][3] = M.data[1][3];
    data[2][1] = M.data[2][1];
    data[2][2] = M.data[2][2];
    data[2][3] = M.data[2][3];
    data[3][1] = M.data[3][1];
    data[3][2] = M.data[3][2];
    data[3][3] = M.data[3][3];

  }
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::SetOParams(char delim,int width,int prec,int origin)
{
  FGMatrix33::delim  = delim;
  FGMatrix33::width  = width;
  FGMatrix33::prec   = prec;
  FGMatrix33::origin = origin;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::InitMatrix(double value)
{
  if (data) {
    data[1][1] = value;
    data[1][2] = value;
    data[1][3] = value;
    data[2][1] = value;
    data[2][2] = value;
    data[2][3] = value;
    data[3][1] = value;
    data[3][2] = value;
    data[3][3] = value;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::InitMatrix(void)
{
  this->InitMatrix(0);
}

// *****************************************************************************
// binary operators ************************************************************
// *****************************************************************************

FGMatrix33 FGMatrix33::operator-(const FGMatrix33& M)
{
  FGMatrix33 Diff;

  Diff(1,1) = data[1][1] - M(1,1);
  Diff(1,2) = data[1][2] - M(1,2);
  Diff(1,3) = data[1][3] - M(1,3);
  Diff(2,1) = data[2][1] - M(2,1);
  Diff(2,2) = data[2][2] - M(2,2);
  Diff(2,3) = data[2][3] - M(2,3);
  Diff(3,1) = data[3][1] - M(3,1);
  Diff(3,2) = data[3][2] - M(3,2);
  Diff(3,3) = data[3][3] - M(3,3);

  
  return Diff;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator-=(const FGMatrix33 &M)
{
  data[1][1] -= M(1,1);
  data[1][2] -= M(1,2);
  data[1][3] -= M(1,3);
  data[2][1] -= M(2,1);
  data[2][2] -= M(2,2);
  data[2][3] -= M(2,3);
  data[3][1] -= M(3,1);
  data[3][2] -= M(3,2);
  data[3][3] -= M(3,3);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator+(const FGMatrix33& M)
{
  FGMatrix33 Sum;

  Sum(1,1) = data[1][1] + M(1,1);
  Sum(1,2) = data[1][2] + M(1,2);
  Sum(1,3) = data[1][3] + M(1,3);
  Sum(2,1) = data[2][1] + M(2,1);
  Sum(2,2) = data[2][2] + M(2,2);
  Sum(2,3) = data[2][3] + M(2,3);
  Sum(3,1) = data[3][1] + M(3,1);
  Sum(3,2) = data[3][2] + M(3,2);
  Sum(3,3) = data[3][3] + M(3,3);

  return Sum;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator+=(const FGMatrix33 &M)
{
  data[1][1] += M(1,1);
  data[1][2] += M(1,2);
  data[1][3] += M(1,3);
  data[2][1] += M(2,1);
  data[2][2] += M(2,2);
  data[2][3] += M(2,3);
  data[3][1] += M(3,1);
  data[3][2] += M(3,2);
  data[3][3] += M(3,3);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 operator*(double scalar, FGMatrix33 &M)
{
  FGMatrix33 Product;
  
  Product(1,1) = M(1,1) * scalar;
  Product(1,2) = M(1,2) * scalar;
  Product(1,3) = M(1,3) * scalar;
  Product(2,1) = M(2,1) * scalar;
  Product(2,2) = M(2,2) * scalar;
  Product(2,3) = M(2,3) * scalar;
  Product(3,1) = M(3,1) * scalar;
  Product(3,2) = M(3,2) * scalar;
  Product(3,3) = M(3,3) * scalar;

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator*=(const double scalar)
{

  data[1][1] *= scalar;
  data[1][2] *= scalar;
  data[1][3] *= scalar;
  data[2][1] *= scalar;
  data[2][2] *= scalar;
  data[2][3] *= scalar;
  data[3][1] *= scalar;
  data[3][2] *= scalar;
  data[3][3] *= scalar;

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator*(const FGMatrix33& M)
{
  FGMatrix33 Product;
  
  Product(1,1) = data[1][1]*M(1,1) + data[1][2]*M(2,1) + data[1][3]*M(3,1);
  Product(1,2) = data[1][1]*M(1,2) + data[1][2]*M(2,2) + data[1][3]*M(3,2);
  Product(1,3) = data[1][1]*M(1,3) + data[1][2]*M(2,3) + data[1][3]*M(3,3);
  Product(2,1) = data[2][1]*M(1,1) + data[2][2]*M(2,1) + data[2][3]*M(3,1);
  Product(2,2) = data[2][1]*M(1,2) + data[2][2]*M(2,2) + data[2][3]*M(3,2);
  Product(2,3) = data[2][1]*M(1,3) + data[2][2]*M(2,3) + data[2][3]*M(3,3);
  Product(3,1) = data[3][1]*M(1,1) + data[3][2]*M(2,1) + data[3][3]*M(3,1);
  Product(3,2) = data[3][1]*M(1,2) + data[3][2]*M(2,2) + data[3][3]*M(3,2);
  Product(3,3) = data[3][1]*M(1,3) + data[3][2]*M(2,3) + data[3][3]*M(3,3);
  
  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator*=(const FGMatrix33& M)
{

  data[1][1] = data[1][1]*M(1,1) + data[1][2]*M(2,1) + data[1][3]*M(3,1);
  data[1][2] = data[1][1]*M(1,2) + data[1][2]*M(2,2) + data[1][3]*M(3,2);
  data[1][3] = data[1][1]*M(1,3) + data[1][2]*M(2,3) + data[1][3]*M(3,3);
  data[2][1] = data[2][1]*M(1,1) + data[2][2]*M(2,1) + data[2][3]*M(3,1);
  data[2][2] = data[2][1]*M(1,2) + data[2][2]*M(2,2) + data[2][3]*M(3,2);
  data[2][3] = data[2][1]*M(1,3) + data[2][2]*M(2,3) + data[2][3]*M(3,3);
  data[3][1] = data[3][1]*M(1,1) + data[3][2]*M(2,1) + data[3][3]*M(3,1);
  data[3][2] = data[3][1]*M(1,2) + data[3][2]*M(2,2) + data[3][3]*M(3,2);
  data[3][3] = data[3][1]*M(1,3) + data[3][2]*M(2,3) + data[3][3]*M(3,3);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator/(const double scalar)
{
  FGMatrix33 Quot;
  
  if( scalar != 0 ) {
    Quot(1,1) = data[1][1] / scalar;
    Quot(1,2) = data[1][2] / scalar;
    Quot(1,3) = data[1][3] / scalar;
    Quot(2,1) = data[2][1] / scalar;
    Quot(2,2) = data[2][2] / scalar;
    Quot(2,3) = data[2][3] / scalar;
    Quot(3,1) = data[3][1] / scalar;
    Quot(3,2) = data[3][2] / scalar;
    Quot(3,3) = data[3][3] / scalar;
  } else {
    MatrixException mE;
    mE.Message = "Attempt to divide by zero in method FGMatrix33::operator/(const double scalar)";
    throw mE;
  }
  return Quot;  
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator/=(const double scalar)
{
  if( scalar != 0 ) {
    data[1][1] /= scalar;
    data[1][2] /= scalar;
    data[1][3] /= scalar;
    data[2][1] /= scalar;
    data[2][2] /= scalar;
    data[2][3] /= scalar;
    data[3][1] /= scalar;
    data[3][2] /= scalar;
    data[3][3] /= scalar;
  } else {
    MatrixException mE;
    mE.Message = "Attempt to divide by zero in method FGMatrix33::operator/=(const double scalar)";
    throw mE;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::T(void)
{
  for (unsigned int i=1; i<=3; i++) {
    for (unsigned int j=i+1; j<=3; j++) {
      double tmp = data[i][j];
      data[i][j] = data[j][i];
      data[j][i] = tmp;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGMatrix33::operator*(const FGColumnVector3& Col)
{
  FGColumnVector3 Product;

  Product(1) = data[1][1]*Col(1) + data[1][2]*Col(2) + data[1][3]*Col(3);
  Product(2) = data[2][1]*Col(1) + data[2][2]*Col(2) + data[2][3]*Col(3);
  Product(3) = data[3][1]*Col(1) + data[3][2]*Col(2) + data[3][3]*Col(3);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::Debug(void)
{
    //TODO: Add your source code here
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

