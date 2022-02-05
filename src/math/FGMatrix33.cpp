/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGMatrix33.cpp
Author: Tony Peden, Jon Berndt, Mathias Frolich
Date started: 1998
Purpose: FGMatrix33 class
Called by: Various

 ------------- Copyright (C) 1998 by the authors above -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

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
#include "FGQuaternion.h"
#include <sstream>
#include <iomanip>

#include <iostream>

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33::FGMatrix33(void)
{
  data[0] = data[1] = data[2] = data[3] = data[4] = data[5] =
    data[6] = data[7] = data[8] = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGMatrix33::Dump(const string& delimiter) const
{
  ostringstream buffer;
  buffer << setw(12) << setprecision(10) << data[0] << delimiter;
  buffer << setw(12) << setprecision(10) << data[3] << delimiter;
  buffer << setw(12) << setprecision(10) << data[6] << delimiter;
  buffer << setw(12) << setprecision(10) << data[1] << delimiter;
  buffer << setw(12) << setprecision(10) << data[4] << delimiter;
  buffer << setw(12) << setprecision(10) << data[7] << delimiter;
  buffer << setw(12) << setprecision(10) << data[2] << delimiter;
  buffer << setw(12) << setprecision(10) << data[5] << delimiter;
  buffer << setw(12) << setprecision(10) << data[8];
  return buffer.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGMatrix33::Dump(const string& delimiter, const string& prefix) const
{
  ostringstream buffer;

  buffer << prefix << right << fixed << setw(9) << setprecision(6) << data[0] << delimiter;
  buffer << right << fixed << setw(9) << setprecision(6) << data[3] << delimiter;
  buffer << right << fixed << setw(9) << setprecision(6) << data[6] << endl;

  buffer << prefix << right << fixed << setw(9) << setprecision(6) << data[1] << delimiter;
  buffer << right << fixed << setw(9) << setprecision(6) << data[4] << delimiter;
  buffer << right << fixed << setw(9) << setprecision(6) << data[7] << endl;

  buffer << prefix << right << fixed << setw(9) << setprecision(6) << data[2] << delimiter;
  buffer << right << fixed << setw(9) << setprecision(6) << data[5] << delimiter;
  buffer << right << fixed << setw(9) << setprecision(6) << data[8];

  buffer << setw(0) << left;

  return buffer.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGQuaternion FGMatrix33::GetQuaternion(void) const
{
  FGQuaternion Q;

  double tempQ[4];
  int idx;

  tempQ[0] = 1.0 + data[0] + data[4] + data[8];
  tempQ[1] = 1.0 + data[0] - data[4] - data[8];
  tempQ[2] = 1.0 - data[0] + data[4] - data[8];
  tempQ[3] = 1.0 - data[0] - data[4] + data[8];

  // Find largest of the above
  idx = 0;
  for (int i=1; i<4; i++) if (tempQ[i] > tempQ[idx]) idx = i;

  switch(idx) {
    case 0:
      Q(1) = 0.50*sqrt(tempQ[0]);
      Q(2) = 0.25*(data[7] - data[5])/Q(1);
      Q(3) = 0.25*(data[2] - data[6])/Q(1);
      Q(4) = 0.25*(data[3] - data[1])/Q(1);
      break;
    case 1:
      Q(2) = 0.50*sqrt(tempQ[1]);
      Q(1) = 0.25*(data[7] - data[5])/Q(2);
      Q(3) = 0.25*(data[3] + data[1])/Q(2);
      Q(4) = 0.25*(data[2] + data[6])/Q(2);
      break;
    case 2:
      Q(3) = 0.50*sqrt(tempQ[2]);
      Q(1) = 0.25*(data[2] - data[6])/Q(3);
      Q(2) = 0.25*(data[3] + data[1])/Q(3);
      Q(4) = 0.25*(data[7] + data[5])/Q(3);
      break;
    case 3:
      Q(4) = 0.50*sqrt(tempQ[3]);
      Q(1) = 0.25*(data[3] - data[1])/Q(4);
      Q(2) = 0.25*(data[6] + data[2])/Q(4);
      Q(3) = 0.25*(data[7] + data[5])/Q(4);
      break;
    default:
      //error
      break;
  }

  return (Q);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Compute the Euler-angles
// Also see Jack Kuipers, "Quaternions and Rotation Sequences", section 7.8..

FGColumnVector3 FGMatrix33::GetEuler(void) const
{
  FGColumnVector3 mEulerAngles;
  bool GimbalLock = false;

  if (data[6] <= -1.0) {
    mEulerAngles(2) = 0.5*M_PI;
    GimbalLock = true;
  }
  else if (1.0 <= data[6]) {
    mEulerAngles(2) = -0.5*M_PI;
    GimbalLock = true;
  }
  else
    mEulerAngles(2) = asin(-data[6]);

  if (GimbalLock)
    mEulerAngles(1) = atan2(-data[5], data[4]);
  else
    mEulerAngles(1) = atan2(data[7], data[8]);

  if (GimbalLock)
    mEulerAngles(3) = 0.0;
  else {
    double psi = atan2(data[3], data[0]);
    if (psi < 0.0)
      psi += 2*M_PI;
    mEulerAngles(3) = psi;
  }

  return mEulerAngles;
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
  return data[0]*data[4]*data[8] + data[3]*data[7]*data[2]
       + data[6]*data[1]*data[5] - data[6]*data[4]*data[2]
       - data[3]*data[1]*data[8] - data[7]*data[5]*data[0];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::Inverse(void) const {
  // Compute the inverse of a general matrix using Cramers rule.
  // I guess googling for cramers rule gives tons of references
  // for this. :)

  if (Determinant() != 0.0) {
    double rdet = 1.0/Determinant();

    double i11 = rdet*(data[4]*data[8]-data[7]*data[5]);
    double i21 = rdet*(data[7]*data[2]-data[1]*data[8]);
    double i31 = rdet*(data[1]*data[5]-data[4]*data[2]);
    double i12 = rdet*(data[6]*data[5]-data[3]*data[8]);
    double i22 = rdet*(data[0]*data[8]-data[6]*data[2]);
    double i32 = rdet*(data[3]*data[2]-data[0]*data[5]);
    double i13 = rdet*(data[3]*data[7]-data[6]*data[4]);
    double i23 = rdet*(data[6]*data[1]-data[0]*data[7]);
    double i33 = rdet*(data[0]*data[4]-data[3]*data[1]);

    return FGMatrix33( i11, i12, i13,
                       i21, i22, i23,
                       i31, i32, i33 );
  } else {
    return FGMatrix33( 0, 0, 0,
                       0, 0, 0,
                       0, 0, 0 );
  }
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
  return FGMatrix33( data[0] - M.data[0],
                     data[3] - M.data[3],
                     data[6] - M.data[6],
                     data[1] - M.data[1],
                     data[4] - M.data[4],
                     data[7] - M.data[7],
                     data[2] - M.data[2],
                     data[5] - M.data[5],
                     data[8] - M.data[8] );
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
  return FGMatrix33( data[0] + M.data[0],
                     data[3] + M.data[3],
                     data[6] + M.data[6],
                     data[1] + M.data[1],
                     data[4] + M.data[4],
                     data[7] + M.data[7],
                     data[2] + M.data[2],
                     data[5] + M.data[5],
                     data[8] + M.data[8] );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator+=(const FGMatrix33 &M)
{
  data[0] += M.data[0];
  data[3] += M.data[3];
  data[6] += M.data[6];
  data[1] += M.data[1];
  data[4] += M.data[4];
  data[7] += M.data[7];
  data[2] += M.data[2];
  data[5] += M.data[5];
  data[8] += M.data[8];

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator*(const double scalar) const
{
  return FGMatrix33( scalar * data[0],
                     scalar * data[3],
                     scalar * data[6],
                     scalar * data[1],
                     scalar * data[4],
                     scalar * data[7],
                     scalar * data[2],
                     scalar * data[5],
                     scalar * data[8] );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
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
*/
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator*=(const double scalar)
{
  data[0] *= scalar;
  data[3] *= scalar;
  data[6] *= scalar;
  data[1] *= scalar;
  data[4] *= scalar;
  data[7] *= scalar;
  data[2] *= scalar;
  data[5] *= scalar;
  data[8] *= scalar;

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator*(const FGMatrix33& M) const
{
  FGMatrix33 Product;

  Product.data[0] = data[0]*M.data[0] + data[3]*M.data[1] + data[6]*M.data[2];
  Product.data[3] = data[0]*M.data[3] + data[3]*M.data[4] + data[6]*M.data[5];
  Product.data[6] = data[0]*M.data[6] + data[3]*M.data[7] + data[6]*M.data[8];
  Product.data[1] = data[1]*M.data[0] + data[4]*M.data[1] + data[7]*M.data[2];
  Product.data[4] = data[1]*M.data[3] + data[4]*M.data[4] + data[7]*M.data[5];
  Product.data[7] = data[1]*M.data[6] + data[4]*M.data[7] + data[7]*M.data[8];
  Product.data[2] = data[2]*M.data[0] + data[5]*M.data[1] + data[8]*M.data[2];
  Product.data[5] = data[2]*M.data[3] + data[5]*M.data[4] + data[8]*M.data[5];
  Product.data[8] = data[2]*M.data[6] + data[5]*M.data[7] + data[8]*M.data[8];

  return Product;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator*=(const FGMatrix33& M)
{
  // FIXME: Make compiler friendlier
  double a,b,c;

  a = data[0]; b=data[3]; c=data[6];
  data[0] = a*M.data[0] + b*M.data[1] + c*M.data[2];
  data[3] = a*M.data[3] + b*M.data[4] + c*M.data[5];
  data[6] = a*M.data[6] + b*M.data[7] + c*M.data[8];

  a = data[1]; b=data[4]; c=data[7];
  data[1] = a*M.data[0] + b*M.data[1] + c*M.data[2];
  data[4] = a*M.data[3] + b*M.data[4] + c*M.data[5];
  data[7] = a*M.data[6] + b*M.data[7] + c*M.data[8];

  a = data[2]; b=data[5]; c=data[8];
  data[2] = a*M.data[0] + b*M.data[1] + c*M.data[2];
  data[5] = a*M.data[3] + b*M.data[4] + c*M.data[5];
  data[8] = a*M.data[6] + b*M.data[7] + c*M.data[8];

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33 FGMatrix33::operator/(const double scalar) const
{
  FGMatrix33 Quot;

  double tmp = 1.0/scalar;
  Quot.data[0] = data[0] * tmp;
  Quot.data[3] = data[3] * tmp;
  Quot.data[6] = data[6] * tmp;
  Quot.data[1] = data[1] * tmp;
  Quot.data[4] = data[4] * tmp;
  Quot.data[7] = data[7] * tmp;
  Quot.data[2] = data[2] * tmp;
  Quot.data[5] = data[5] * tmp;
  Quot.data[8] = data[8] * tmp;

  return Quot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGMatrix33::operator/=(const double scalar)
{
  double tmp = 1.0/scalar;
  data[0] *= tmp;
  data[3] *= tmp;
  data[6] *= tmp;
  data[1] *= tmp;
  data[4] *= tmp;
  data[7] *= tmp;
  data[2] *= tmp;
  data[5] *= tmp;
  data[8] *= tmp;

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMatrix33::T(void)
{
  double tmp;

  tmp = data[3];
  data[3] = data[1];
  data[1] = tmp;

  tmp = data[6];
  data[6] = data[2];
  data[2] = tmp;

  tmp = data[7];
  data[7] = data[5];
  data[5] = tmp;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGMatrix33::operator*(const FGColumnVector3& v) const
{
  double v1 = v(1);
  double v2 = v(2);
  double v3 = v(3);

  double tmp1 = v1*data[0];  //[(col-1)*eRows+row-1]
  double tmp2 = v1*data[1];
  double tmp3 = v1*data[2];

  tmp1 += v2*data[3];
  tmp2 += v2*data[4];
  tmp3 += v2*data[5];

  tmp1 += v3*data[6];
  tmp2 += v3*data[7];
  tmp3 += v3*data[8];

  return FGColumnVector3( tmp1, tmp2, tmp3 );
}

}
