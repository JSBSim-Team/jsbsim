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

static const char *IdSrc = "$Id: FGColumnVector4.cpp,v 1.10 2001/12/23 21:49:01 jberndt Exp $";
static const char *IdHdr = ID_COLUMNVECTOR4;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGColumnVector4::FGColumnVector4(void)
{
  rowCtr = 1;
  data[1]=0;data[2]=0;data[3]=0;data[4]=0;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4::FGColumnVector4(double A, double B, double C, double D)
{
  rowCtr = 1;
  data[1]=0;data[2]=0;data[3]=0;data[4]=0;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4::~FGColumnVector4(void)
{
  Debug(1);
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4::FGColumnVector4(const FGColumnVector4& b) 
{
  data[1] = b.data[1];
  data[2] = b.data[2];
  data[3] = b.data[3];
  data[4] = b.data[4];

  rowCtr = 1;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector4 FGColumnVector4::operator=(const FGColumnVector4& b) 
{
  data[1] = b.data[1];
  data[2] = b.data[2];
  data[3] = b.data[3];
  data[4] = b.data[4];
  rowCtr = 1;
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
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGColumnVector4::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGColumnVector4" << endl;
    if (from == 1) cout << "Destroyed:    FGColumnVector4" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) { // Sanity checking
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}


