/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGMatrix33.cpp
Author: Tony Peden, Jon Berndt, Mathias Frolich
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

namespace JSBSim {

static const char *IdSrc = "$Id: FGMatrix33.cpp,v 1.19 2004/03/06 17:05:30 jberndt Exp $";
static const char *IdHdr = ID_MATRIX33;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(void)
{
  InitMatrix();
  rowCtr = colCtr = 1;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(int r, int c)
{
  InitMatrix();
  rowCtr = colCtr = 1;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(const FGMatrix33& M)
{
  rowCtr = colCtr = 1;

  Entry(1,1) = M.Entry(1,1);
  Entry(1,2) = M.Entry(1,2);
  Entry(1,3) = M.Entry(1,3);
  Entry(2,1) = M.Entry(2,1);
  Entry(2,2) = M.Entry(2,2);
  Entry(2,3) = M.Entry(2,3);
  Entry(3,1) = M.Entry(3,1);
  Entry(3,2) = M.Entry(3,2);
  Entry(3,3) = M.Entry(3,3);

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(double x1, double x2, double x3,
                       double y1, double y2, double y3,
                       double z1, double z2, double z3)
{
  rowCtr = colCtr = 1;

  Entry(1,1) = x1;
  Entry(1,2) = x2;
  Entry(1,3) = x3;
  Entry(2,1) = y1;
  Entry(2,2) = y2;
  Entry(2,3) = y3;
  Entry(3,1) = z1;
  Entry(3,2) = z2;
  Entry(3,3) = z3;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::~FGMatrix33(void)
{
  rowCtr = colCtr = 1;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ostream& operator<<(ostream& os, const FGMatrix33& M)
{
  for (unsigned int i=1; i<=M.Rows(); i++) {
    for (unsigned int j=1; j<=M.Cols(); j++) {
      if (i == M.Rows() && j == M.Cols())
        os << M(i,j);
      else
        os << M(i,j) << ", ";
    }
  }
  return os;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator<<(const double ff)
{
  Entry(rowCtr,colCtr) = ff;
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
      is >> M(i,j);
    }
  }
  return is;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator=(const FGMatrix33& M)
{
  if (&M != this) {
    Entry(1,1) = M.Entry(1,1);
    Entry(1,2) = M.Entry(1,2);
    Entry(1,3) = M.Entry(1,3);
    Entry(2,1) = M.Entry(2,1);
    Entry(2,2) = M.Entry(2,2);
    Entry(2,3) = M.Entry(2,3);
    Entry(3,1) = M.Entry(3,1);
    Entry(3,2) = M.Entry(3,2);
    Entry(3,3) = M.Entry(3,3);
  }
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::InitMatrix(double value)
{
  Entry(1,1) = value;
  Entry(1,2) = value;
  Entry(1,3) = value;
  Entry(2,1) = value;
  Entry(2,2) = value;
  Entry(2,3) = value;
  Entry(3,1) = value;
  Entry(3,2) = value;
  Entry(3,3) = value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::InitMatrix(double x1, double x2, double x3,
                            double y1, double y2, double y3,
                            double z1, double z2, double z3)
{
  Entry(1,1) = x1;
  Entry(1,2) = x2;
  Entry(1,3) = x3;
  Entry(2,1) = y1;
  Entry(2,2) = y2;
  Entry(2,3) = y3;
  Entry(3,1) = z1;
  Entry(3,2) = z2;
  Entry(3,3) = z3;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGMatrix33::Determinant(void) const {
  return Entry(1,1)*Entry(2,2)*Entry(3,3) + Entry(1,2)*Entry(2,3)*Entry(3,1)
    + Entry(1,3)*Entry(2,1)*Entry(3,2) - Entry(1,3)*Entry(2,2)*Entry(3,1)
    - Entry(1,2)*Entry(2,1)*Entry(3,3) - Entry(2,3)*Entry(3,2)*Entry(1,1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::Inverse(void) const {
  // Compute the inverse of a general matrix using Cramers rule.
  // I guess googling for cramers rule gives tons of references
  // for this. :)
  double rdet = 1.0/Determinant();

  double i11 = rdet*(Entry(2,2)*Entry(3,3)-Entry(2,3)*Entry(3,2));
  double i21 = rdet*(Entry(2,3)*Entry(3,1)-Entry(2,1)*Entry(3,3));
  double i31 = rdet*(Entry(2,1)*Entry(3,2)-Entry(2,2)*Entry(3,1));
  double i12 = rdet*(Entry(1,3)*Entry(3,2)-Entry(1,2)*Entry(3,3));
  double i22 = rdet*(Entry(1,1)*Entry(3,3)-Entry(1,3)*Entry(3,1));
  double i32 = rdet*(Entry(1,2)*Entry(3,1)-Entry(1,1)*Entry(3,2));
  double i13 = rdet*(Entry(1,2)*Entry(2,3)-Entry(1,3)*Entry(2,2));
  double i23 = rdet*(Entry(1,3)*Entry(2,1)-Entry(1,1)*Entry(2,3));
  double i33 = rdet*(Entry(1,1)*Entry(2,2)-Entry(1,2)*Entry(2,1));

  return FGMatrix33( i11, i12, i13,
                     i21, i22, i23,
                     i31, i32, i33 );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::InitMatrix(void)
{
  this->InitMatrix(0);
}

// *****************************************************************************
// binary operators ************************************************************
// *****************************************************************************

FGMatrix33 FGMatrix33::operator-(const FGMatrix33& M) const
{
  FGMatrix33 Diff;

  Diff(1,1) = Entry(1,1) - M(1,1);
  Diff(1,2) = Entry(1,2) - M(1,2);
  Diff(1,3) = Entry(1,3) - M(1,3);
  Diff(2,1) = Entry(2,1) - M(2,1);
  Diff(2,2) = Entry(2,2) - M(2,2);
  Diff(2,3) = Entry(2,3) - M(2,3);
  Diff(3,1) = Entry(3,1) - M(3,1);
  Diff(3,2) = Entry(3,2) - M(3,2);
  Diff(3,3) = Entry(3,3) - M(3,3);

  return Diff;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator-=(const FGMatrix33 &M)
{
  Entry(1,1) -= M(1,1);
  Entry(1,2) -= M(1,2);
  Entry(1,3) -= M(1,3);
  Entry(2,1) -= M(2,1);
  Entry(2,2) -= M(2,2);
  Entry(2,3) -= M(2,3);
  Entry(3,1) -= M(3,1);
  Entry(3,2) -= M(3,2);
  Entry(3,3) -= M(3,3);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator+(const FGMatrix33& M) const
{
  FGMatrix33 Sum;

  Sum(1,1) = Entry(1,1) + M(1,1);
  Sum(1,2) = Entry(1,2) + M(1,2);
  Sum(1,3) = Entry(1,3) + M(1,3);
  Sum(2,1) = Entry(2,1) + M(2,1);
  Sum(2,2) = Entry(2,2) + M(2,2);
  Sum(2,3) = Entry(2,3) + M(2,3);
  Sum(3,1) = Entry(3,1) + M(3,1);
  Sum(3,2) = Entry(3,2) + M(3,2);
  Sum(3,3) = Entry(3,3) + M(3,3);

  return Sum;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator+=(const FGMatrix33 &M)
{
  Entry(1,1) += M(1,1);
  Entry(1,2) += M(1,2);
  Entry(1,3) += M(1,3);
  Entry(2,1) += M(2,1);
  Entry(2,2) += M(2,2);
  Entry(2,3) += M(2,3);
  Entry(3,1) += M(3,1);
  Entry(3,2) += M(3,2);
  Entry(3,3) += M(3,3);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator*(const double scalar) const
{
  FGMatrix33 Product;

  Product(1,1) = Entry(1,1) * scalar;
  Product(1,2) = Entry(1,2) * scalar;
  Product(1,3) = Entry(1,3) * scalar;
  Product(2,1) = Entry(2,1) * scalar;
  Product(2,2) = Entry(2,2) * scalar;
  Product(2,3) = Entry(2,3) * scalar;
  Product(3,1) = Entry(3,1) * scalar;
  Product(3,2) = Entry(3,2) * scalar;
  Product(3,3) = Entry(3,3) * scalar;

  return Product;
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
  Entry(1,1) *= scalar;
  Entry(1,2) *= scalar;
  Entry(1,3) *= scalar;
  Entry(2,1) *= scalar;
  Entry(2,2) *= scalar;
  Entry(2,3) *= scalar;
  Entry(3,1) *= scalar;
  Entry(3,2) *= scalar;
  Entry(3,3) *= scalar;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator*(const FGMatrix33& M) const
{
  FGMatrix33 Product;

  Product(1,1) = Entry(1,1)*M(1,1) + Entry(1,2)*M(2,1) + Entry(1,3)*M(3,1);
  Product(1,2) = Entry(1,1)*M(1,2) + Entry(1,2)*M(2,2) + Entry(1,3)*M(3,2);
  Product(1,3) = Entry(1,1)*M(1,3) + Entry(1,2)*M(2,3) + Entry(1,3)*M(3,3);
  Product(2,1) = Entry(2,1)*M(1,1) + Entry(2,2)*M(2,1) + Entry(2,3)*M(3,1);
  Product(2,2) = Entry(2,1)*M(1,2) + Entry(2,2)*M(2,2) + Entry(2,3)*M(3,2);
  Product(2,3) = Entry(2,1)*M(1,3) + Entry(2,2)*M(2,3) + Entry(2,3)*M(3,3);
  Product(3,1) = Entry(3,1)*M(1,1) + Entry(3,2)*M(2,1) + Entry(3,3)*M(3,1);
  Product(3,2) = Entry(3,1)*M(1,2) + Entry(3,2)*M(2,2) + Entry(3,3)*M(3,2);
  Product(3,3) = Entry(3,1)*M(1,3) + Entry(3,2)*M(2,3) + Entry(3,3)*M(3,3);

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::operator*=(const FGMatrix33& M)
{
  double a,b,c;

  a = Entry(1,1); b=Entry(1,2); c=Entry(1,3);
  Entry(1,1) = a*M(1,1) + b*M(2,1) + c*M(3,1);
  Entry(1,2) = a*M(1,2) + b*M(2,2) + c*M(3,2);
  Entry(1,3) = a*M(1,3) + b*M(2,3) + c*M(3,3);

  a = Entry(2,1); b=Entry(2,2); c=Entry(2,3);
  Entry(2,1) = a*M(1,1) + b*M(2,1) + c*M(3,1);
  Entry(2,2) = a*M(1,2) + b*M(2,2) + c*M(3,2);
  Entry(2,3) = a*M(1,3) + b*M(2,3) + c*M(3,3);

  a = Entry(3,1); b=Entry(3,2); c=Entry(3,3);
  Entry(3,1) = a*M(1,1) + b*M(2,1) + c*M(3,1);
  Entry(3,2) = a*M(1,2) + b*M(2,2) + c*M(3,2);
  Entry(3,3) = a*M(1,3) + b*M(2,3) + c*M(3,3);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator/(const double scalar) const
{
  FGMatrix33 Quot;

  if ( scalar != 0 ) {
    double tmp = 1.0/scalar;
    Quot(1,1) = Entry(1,1) * tmp;
    Quot(1,2) = Entry(1,2) * tmp;
    Quot(1,3) = Entry(1,3) * tmp;
    Quot(2,1) = Entry(2,1) * tmp;
    Quot(2,2) = Entry(2,2) * tmp;
    Quot(2,3) = Entry(2,3) * tmp;
    Quot(3,1) = Entry(3,1) * tmp;
    Quot(3,2) = Entry(3,2) * tmp;
    Quot(3,3) = Entry(3,3) * tmp;
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
  if ( scalar != 0 ) {
    double tmp = 1.0/scalar;
    Entry(1,1) *= tmp;
    Entry(1,2) *= tmp;
    Entry(1,3) *= tmp;
    Entry(2,1) *= tmp;
    Entry(2,2) *= tmp;
    Entry(2,3) *= tmp;
    Entry(3,1) *= tmp;
    Entry(3,2) *= tmp;
    Entry(3,3) *= tmp;
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
      double tmp = Entry(i,j);
      Entry(i,j) = Entry(j,i);
      Entry(j,i) = tmp;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGMatrix33::operator*(const FGColumnVector3& Col) const
{
  return ( FGColumnVector3(
    Entry(1,1)*Col(1) + Entry(1,2)*Col(2) + Entry(1,3)*Col(3),
    Entry(2,1)*Col(1) + Entry(2,2)*Col(2) + Entry(2,3)*Col(3),
    Entry(3,1)*Col(1) + Entry(3,2)*Col(2) + Entry(3,3)*Col(3))
  );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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

void FGMatrix33::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGMatrix33" << endl;
    if (from == 1) cout << "Destroyed:    FGMatrix33" << endl;
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
}
