/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGColumnVector3.cpp
Author: Originally by Tony Peden [formatted here (and broken??) by JSB]
Date started: 1998
Purpose: FGColumnVector3 class
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

namespace JSBSim {

static const char *IdSrc = "$Id: FGColumnVector3.cpp,v 1.17 2004/03/06 14:16:46 jberndt Exp $";
static const char *IdHdr = ID_COLUMNVECTOR3;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGColumnVector3::FGColumnVector3(void)
{
  rowCtr = 1;
  data[0]=0; data[1]=0; data[2]=0; data[3]=0;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3::FGColumnVector3(double X, double Y, double Z)
{
  rowCtr = 1;
  data[0] = 0; data[eX] = X; data[eY] = Y; data[eZ] = Z;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3::~FGColumnVector3(void)
{
  Debug(1);
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3::FGColumnVector3(const FGColumnVector3& b) 
{
  data[1] = b.data[1];
  data[2] = b.data[2];
  data[3] = b.data[3];
  rowCtr = 1;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator=(const FGColumnVector3& b) 
{
  data[1] = b.data[1];
  data[2] = b.data[2];
  data[3] = b.data[3];
  rowCtr = 1;

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator+(const FGColumnVector3& C) const
{
  FGColumnVector3 Sum; 
  Sum(1) = C(1) + data[1];
  Sum(2) = C(2) + data[2];
  Sum(3) = C(3) + data[3];

  return Sum;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector3::operator+=(const FGColumnVector3& C)
{
   data[1] += C(1);
   data[2] += C(2);
   data[3] += C(3);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator*(const double scalar) const
{
  FGColumnVector3 Product;

  Product(1) = scalar * data[1];
  Product(2) = scalar * data[2];
  Product(3) = scalar * data[3];

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector3::operator*=(const double scalar)
{
  data[1] *= scalar;
  data[2] *= scalar;
  data[3] *= scalar;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator-(const FGColumnVector3& V) const
{
  
  FGColumnVector3 Diff; 
  
  Diff(1) = data[1] - V(1);
  Diff(2) = data[2] - V(2);
  Diff(3) = data[3] - V(3);

  return Diff;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector3::operator-=(const FGColumnVector3& V)
{
  data[1] -= V(1);
  data[2] -= V(2);
  data[3] -= V(3);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator/(const double scalar) const
{
  FGColumnVector3 Quotient;

  if (scalar != 0) {
	  double tmp = 1.0/scalar;
    Quotient(1) = data[1] * tmp;
    Quotient(2) = data[2] * tmp;
    Quotient(3) = data[3] * tmp;
  } else {
    cerr << "Attempt to divide by zero in method FGColumnVector3::operator/(const double scalar), object " << this << endl; 
  }
  return Quotient;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector3::operator/=(const double scalar)
{
  FGColumnVector3 Quotient;

  if (scalar != 0) {
	  double tmp = 1.0/scalar;
    data[1] *= tmp;
    data[2] *= tmp;
    data[3] *= tmp;
  } else {
    cerr << "Attempt to divide by zero in method FGColumnVector3::operator/=(const double scalar), object " << this << endl; 
  }
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

double FGColumnVector3::Magnitude(void) const
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
     Mag = 1.0/Mag;
     data[1] *= Mag;
     data[2] *= Mag;
     data[3] *= Mag;
  }    

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator*(const FGColumnVector3& V) const
{
  FGColumnVector3 Product;
  
  Product(1) = data[2] * V(3) - data[3] * V(2);
  Product(2) = data[3] * V(1) - data[1] * V(3);
  Product(3) = data[1] * V(2) - data[2] * V(1);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGColumnVector3::operator*=(const FGColumnVector3& V)
{
  double a,b,c;
  a = data[1]; b=data[2]; c=data[3];
  
  data[1] = b * V(3) - c * V(2);
  data[2] = c * V(1) - a * V(3);
  data[3] = a * V(2) - b * V(1);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::multElementWise(const FGColumnVector3& V) const
{
  FGColumnVector3 Product;

  Product(1) = data[1] * V(1);
  Product(2) = data[2] * V(2);
  Product(3) = data[3] * V(3);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ostream& operator<<(ostream& os, const FGColumnVector3& col)
{
  os << col(1) << " , " << col(2) << " , " << col(3);
  return os;
}  

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGColumnVector3::operator<<(const double ff)
{
  data[rowCtr] = ff;
  if (++rowCtr > 3 )
      rowCtr = 1;
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

void FGColumnVector3::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGColumnVector3" << endl;
    if (from == 1) cout << "Destroyed:    FGColumnVector3" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

} // namespace JSBSim
