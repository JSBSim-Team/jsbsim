/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGColumnVector3.h"
#include "FGMatrix.h"


static const char *IdSrc = "$Id: FGColumnVector3.cpp,v 1.1 2001/07/14 12:48:05 apeden Exp $";
static const char *IdHdr = ID_MATRIX;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGColumnVector3::FGColumnVector3(void)
{
  data = new double[4];
  if (debug_lvl & 2) cout << "Instantiated: FGColumnVector3" << endl;
}

FGColumnVector3::~FGColumnVector3(void)
{
  delete[] data;
  data = NULL;
  if (debug_lvl & 2) cout << "Destroyed:    FGColumnVector3" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double& FGColumnVector3::operator()(int m) const
{
  return data[m];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 operator*(const FGMatrix& Mat, const FGColumnVector3& Col)
{
  FGColumnVector3 Product;

  Product(1) = Col(1)*Mat(1,1) + Col(2)*Mat(1,2) + Col(3)*Mat(1,3);
  Product(2) = Col(1)*Mat(2,1) + Col(2)*Mat(2,2) + Col(3)*Mat(2,3);
  Product(3) = Col(1)*Mat(3,1) + Col(2)*Mat(3,2) + Col(3)*Mat(3,3);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator+(const FGColumnVector3& C)
{
  FGColumnVector3 Sum; // This must be created dynamically
                                // because we don't know the size of "C",
                                // it could be 3 or 4 or ...
  Sum(1) = C(1) + data[1];
  Sum(2) = C(2) + data[2];
  Sum(3) = C(3) + data[3];

  return Sum;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator*(const double scalar)
{
  FGColumnVector3 Product;

  Product(1) = scalar * data[1];
  Product(2) = scalar * data[2];
  Product(3) = scalar * data[3];

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator-(const FGColumnVector3& V)
{
  
  FGColumnVector3 Diff; 
  
  Diff(1) = V(1) - data[1];
  Diff(2) = V(2) - data[2];
  Diff(3) = V(3) - data[3];

  return Diff;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator/(const double scalar)
{
  FGColumnVector3 Quotient;

  if (scalar != 0) {
    Quotient(1) = data[1] / scalar;
    Quotient(2) = data[2] / scalar;
    Quotient(3) = data[3] / scalar;
  } else {
    cerr << "Attempt to divide by zero in method FGColumnVector3::operator/(const double scalar), object " << this << endl; 
  }
  return Quotient;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 operator*(const double scalar, const FGColumnVector3& C)
{
  FGColumnVector3 Product;

  Product(1) = scalar * C(1);
  Product(2) = scalar * C(2);
  Product(3) = scalar * C(3);
  
  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGColumnVector3::Magnitude(void)
{
  double num;

  if ((data[1] == 0.00) &&
      (data[2] == 0.00) &&
      (data[3] == 0.00))
  {
    return 0.00;
  } else {
    num  = data[1]*data[1];
    num += data[2]*data[2];
    num += data[3]*data[3];
    return sqrt(num);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::Normalize(void)
{
  double Mag = Magnitude();

  if (Mag != 0) {
     data[1] /= Mag;
     data[2] /= Mag;
     data[3] /= Mag;
  }    

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator*(const FGColumnVector3& V)
{
  FGColumnVector3 Product;
  
  Product(1) = data[2] * V(3) - data[3] * V(2);
  Product(2) = data[3] * V(1) - data[1] * V(3);
  Product(3) = data[1] * V(2) - data[2] * V(1);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::multElementWise(const FGColumnVector3& V)
{
  FGColumnVector3 Product;

  Product(1) = data[1] * V(1);
  Product(2) = data[2] * V(2);
  Product(3) = data[3] * V(3);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector3::Debug(void)
{
    //TODO: Add your source code here
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

