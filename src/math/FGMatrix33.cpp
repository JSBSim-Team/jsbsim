/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGMatrix33.cpp
Author: Tony Peden, Jon Berndt, Mathias Frolich
Date started: 1998
Purpose: FGMatrix33 class
Called by: Various

 ------------- Copyright (C) 1998 by the authors above -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

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

static const char *IdSrc = "$Id: FGMatrix33.cpp,v 1.4 2008/12/12 05:05:50 jberndt Exp $";
static const char *IdHdr = ID_MATRIX33;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(void)
{
  data[0] = data[1] = data[2] = data[3] = data[4] = data[5] =
    data[6] = data[7] = data[8] = 0.0;

  // Debug(0);
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
  data[0] = data[1] = data[2] = data[3] = data[4] = data[5] =
    data[6] = data[7] = data[8] = 0.0;
}

// *****************************************************************************
// binary operators ************************************************************
// *****************************************************************************

FGMatrix33 FGMatrix33::operator-(const FGMatrix33& M) const
{
  return FGMatrix33( Entry(1,1) - M(1,1),
                     Entry(1,2) - M(1,2),
                     Entry(1,3) - M(1,3),
                     Entry(2,1) - M(2,1),
                     Entry(2,2) - M(2,2),
                     Entry(2,3) - M(2,3),
                     Entry(3,1) - M(3,1),
                     Entry(3,2) - M(3,2),
                     Entry(3,3) - M(3,3) );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator-=(const FGMatrix33 &M)
{
  data[0] -= M.data[0];
  data[1] -= M.data[1];
  data[2] -= M.data[2];
  data[3] -= M.data[3];
  data[4] -= M.data[4];
  data[5] -= M.data[5];
  data[6] -= M.data[6];
  data[7] -= M.data[7];
  data[8] -= M.data[8];

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator+(const FGMatrix33& M) const
{
  return FGMatrix33( Entry(1,1) + M(1,1),
                     Entry(1,2) + M(1,2),
                     Entry(1,3) + M(1,3),
                     Entry(2,1) + M(2,1),
                     Entry(2,2) + M(2,2),
                     Entry(2,3) + M(2,3),
                     Entry(3,1) + M(3,1),
                     Entry(3,2) + M(3,2),
                     Entry(3,3) + M(3,3) );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator+=(const FGMatrix33 &M)
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

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator*(const double scalar) const
{
  return FGMatrix33( scalar * Entry(1,1),
                     scalar * Entry(1,2),
                     scalar * Entry(1,3),
                     scalar * Entry(2,1),
                     scalar * Entry(2,2),
                     scalar * Entry(2,3),
                     scalar * Entry(3,1),
                     scalar * Entry(3,2),
                     scalar * Entry(3,3) );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 operator*(double scalar, FGMatrix33 &M)
{
  return FGMatrix33( scalar * M(1,1),
                     scalar * M(1,2),
                     scalar * M(1,3),
                     scalar * M(2,1),
                     scalar * M(2,2),
                     scalar * M(2,3),
                     scalar * M(3,1),
                     scalar * M(3,2),
                     scalar * M(3,3) );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator*=(const double scalar)
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

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator*(const FGMatrix33& M) const
{
  // FIXME: Make compiler friendlier
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

FGMatrix33& FGMatrix33::operator*=(const FGMatrix33& M)
{
  // FIXME: Make compiler friendlier
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

  return *this;
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

FGMatrix33& FGMatrix33::operator/=(const double scalar)
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
  return *this;
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

FGColumnVector3 FGMatrix33::operator*(const FGColumnVector3& v) const {
  double tmp1 = v(1)*Entry(1,1);
  double tmp2 = v(1)*Entry(2,1);
  double tmp3 = v(1)*Entry(3,1);

  tmp1 += v(2)*Entry(1,2);
  tmp2 += v(2)*Entry(2,2);
  tmp3 += v(2)*Entry(3,2);

  tmp1 += v(3)*Entry(1,3);
  tmp2 += v(3)*Entry(2,3);
  tmp3 += v(3)*Entry(3,3);

  return FGColumnVector3( tmp1, tmp2, tmp3 );
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
