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

#include "FGColumnVector4.h"

static const char *IdSrc = "$Id: FGColumnVector4.cpp,v 1.8 2001/12/06 20:56:53 jberndt Exp $";
static const char *IdHdr = ID_COLUMNVECTOR4;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGColumnVector4::FGColumnVector4(void)
{
  rowCtr = 1;
  data[1]=0;data[2]=0;data[3]=0;data[4]=0;

  if (debug_lvl & 2) cout << "Instantiated: FGColumnVector4" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4::FGColumnVector4(double A, double B, double C, double D)
{
  rowCtr = 1;
  data[1]=0;data[2]=0;data[3]=0;data[4]=0;

  if (debug_lvl & 2) cout << "Instantiated: FGColumnVector4" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4::~FGColumnVector4(void)
{
  if (debug_lvl & 2) cout << "Destroyed:    FGColumnVector4" << endl;
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4::FGColumnVector4(const FGColumnVector4& b) 
{
  data[1] = b.data[1];
  data[2] = b.data[2];
  data[3] = b.data[3];
  data[4] = b.data[4];

  rowCtr = 1;

  if (debug_lvl & 2) cout << "Instantiated: FGColumnVector4" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::operator=(const FGColumnVector4& b) 
{
  data[1] = b.data[1];
  data[2] = b.data[2];
  data[3] = b.data[3];
  data[4] = b.data[4];
  rowCtr = 1;

  if (debug_lvl & 2) cout << "Instantiated: FGColumnVector4" << endl;
  
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::operator+(const FGColumnVector4& C)
{
  FGColumnVector4 Sum;

  Sum(1) = C(1) + data[1];
  Sum(2) = C(2) + data[2];
  Sum(3) = C(3) + data[3];
  Sum(4) = C(4) + data[4];
  return Sum;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector4::operator+=(const FGColumnVector4& C)
{
   data[1] += C(1);
   data[2] += C(2);
   data[3] += C(3);
   data[4] += C(4);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::operator*(const double scalar)
{
  FGColumnVector4 Product;

  Product(1) = scalar * data[1];
  Product(2) = scalar * data[2];
  Product(3) = scalar * data[3];
  Product(4) = scalar * data[4];

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector4::operator*=(const double scalar)
{
  data[1] *= scalar;
  data[2] *= scalar;
  data[3] *= scalar;
  data[4] *= scalar;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::operator-(const FGColumnVector4& V)
{
  
  FGColumnVector4 Diff; 
  
  Diff(1) = data[1] - V(1);
  Diff(2) = data[2] - V(2);
  Diff(3) = data[3] - V(3);
  Diff(4) = data[4] - V(4);
  
  return Diff;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector4::operator-=(const FGColumnVector4& V)
{
  data[1] -= V(1);
  data[2] -= V(2);
  data[3] -= V(3);
  data[4] -= V(4);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::operator/(const double scalar)
{
  FGColumnVector4 Quotient;

  if (scalar != 0) {
	  double tmp = 1.0/scalar;
    Quotient(1) = data[1] * tmp;
    Quotient(2) = data[2] * tmp;
    Quotient(3) = data[3] * tmp;
    Quotient(4) = data[4] * tmp;
  } else {
    cerr << "Attempt to divide by zero in method FGColumnVector4::operator/(const double scalar), object " << this << endl; 
  }
  return Quotient;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector4::operator/=(const double scalar)
{
  FGColumnVector4 Quotient;

  if (scalar != 0) {
	  double tmp = 1.0/scalar;
    data[1] *= tmp;
    data[2] *= tmp;
    data[3] *= tmp;
    data[4] *= tmp;
  } else {
    cerr << "Attempt to divide by zero in method FGColumnVector4::operator/=(const double scalar), object " << this << endl; 
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 operator*(const double scalar, const FGColumnVector4& C)
{
  FGColumnVector4 Product;

  Product(1) = scalar * C(1);
  Product(2) = scalar * C(2);
  Product(3) = scalar * C(3);
  Product(4) = scalar * C(4);
  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGColumnVector4::Magnitude(void)
{
  double num;

  if ((data[1] == 0.00) &&
      (data[2] == 0.00) &&
      (data[3] == 0.00) &&
      (data[4] == 0.00))
  {
    return 0.00;
  } else {
    num  = data[1]*data[1];
    num += data[2]*data[2];
    num += data[3]*data[3];
    num += data[4]*data[4];
    return sqrt(num);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::Normalize(void)
{
  double Mag = Magnitude();

  if (Mag != 0) {
	  Mag = 1.0/Mag;
     data[1] *= Mag;
     data[2] *= Mag;
     data[3] *= Mag;
     data[4] *= Mag;  
  }    

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::multElementWise(const FGColumnVector4& V)
{
  FGColumnVector4 Product;

  Product(1) = data[1] * V(1);
  Product(2) = data[2] * V(2);
  Product(3) = data[3] * V(3);
  Product(4) = data[4] * V(4);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ostream& operator<<(ostream& os, FGColumnVector4& col)
{
  os << col(1) << " , " << col(2) << " , " << col(3) << " , " << col(4);
  return os;
}  

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4& FGColumnVector4::operator<<(const double ff)
{
  data[rowCtr] = ff;
  if (++rowCtr > 4) rowCtr = 1;
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector4::Debug(void)
{
    //TODO: Add your source code here
}

