/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGQuaternion.cpp
 Author:       Jon Berndt, Mathias Froehlich
 Date started: 12/02/98

 ------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) ------------------
 -------           (C) 2004  Mathias Froehlich (Mathias.Froehlich@web.de) ----

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

HISTORY
-------------------------------------------------------------------------------
12/02/98   JSB   Created
15/01/04   Mathias Froehlich implemented a quaternion class from many places
           in JSBSim.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  INCLUDES
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>
#include <iostream>
#include <cmath>
using std::cerr;
using std::cout;
using std::endl;

#include "FGMatrix33.h"
#include "FGColumnVector3.h"

#include "FGQuaternion.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  DEFINITIONS
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {
  
static const char *IdSrc = "$Id: FGQuaternion.cpp,v 1.6 2008/07/22 02:42:17 jberndt Exp $";
static const char *IdHdr = ID_QUATERNION;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Initialize from q
FGQuaternion::FGQuaternion(const FGQuaternion& q)
  : mCacheValid(q.mCacheValid) {
  Entry(1) = q(1);
  Entry(2) = q(2);
  Entry(3) = q(3);
  Entry(4) = q(4);
  if (mCacheValid) {
    mT = q.mT;
    mTInv = q.mTInv;
    mEulerAngles = q.mEulerAngles;
    mEulerSines = q.mEulerSines;
    mEulerCosines = q.mEulerCosines;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Initialize with the three euler angles
FGQuaternion::FGQuaternion(double phi, double tht, double psi)
  : mCacheValid(false) {
  double thtd2 = 0.5*tht;
  double psid2 = 0.5*psi;
  double phid2 = 0.5*phi;
  
  double Sthtd2 = sin(thtd2);
  double Spsid2 = sin(psid2);
  double Sphid2 = sin(phid2);
  
  double Cthtd2 = cos(thtd2);
  double Cpsid2 = cos(psid2);
  double Cphid2 = cos(phid2);
  
  double Cphid2Cthtd2 = Cphid2*Cthtd2;
  double Cphid2Sthtd2 = Cphid2*Sthtd2;
  double Sphid2Sthtd2 = Sphid2*Sthtd2;
  double Sphid2Cthtd2 = Sphid2*Cthtd2;
  
  Entry(1) = Cphid2Cthtd2*Cpsid2 + Sphid2Sthtd2*Spsid2;
  Entry(2) = Sphid2Cthtd2*Cpsid2 - Cphid2Sthtd2*Spsid2;
  Entry(3) = Cphid2Sthtd2*Cpsid2 + Sphid2Cthtd2*Spsid2;
  Entry(4) = Cphid2Cthtd2*Spsid2 - Sphid2Sthtd2*Cpsid2;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/** Returns the derivative of the quaternion corresponding to the
    angular velocities PQR.
    See Stevens and Lewis, "Aircraft Control and Simulation", Second Edition,
    Equation 1.3-36. 
*/
FGQuaternion FGQuaternion::GetQDot(const FGColumnVector3& PQR) const {
  double norm = Magnitude();
  if (norm == 0.0)
    return FGQuaternion::zero();
  double rnorm = 1.0/norm;

  FGQuaternion QDot;
  QDot(1) = -0.5*(Entry(2)*PQR(eP) + Entry(3)*PQR(eQ) + Entry(4)*PQR(eR));
  QDot(2) =  0.5*(Entry(1)*PQR(eP) + Entry(3)*PQR(eR) - Entry(4)*PQR(eQ));
  QDot(3) =  0.5*(Entry(1)*PQR(eQ) + Entry(4)*PQR(eP) - Entry(2)*PQR(eR));
  QDot(4) =  0.5*(Entry(1)*PQR(eR) + Entry(2)*PQR(eQ) - Entry(3)*PQR(eP));
  return rnorm*QDot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGQuaternion::Normalize()
{
  // Note: this does not touch the cache
  // since it does not change the orientation ...
  
  double norm = Magnitude();
  if (norm == 0.0)
    return;
  
  double rnorm = 1.0/norm;
  Entry(1) *= rnorm;
  Entry(2) *= rnorm;
  Entry(3) *= rnorm;
  Entry(4) *= rnorm;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Compute the derived values if required ...
void FGQuaternion::ComputeDerivedUnconditional(void) const
{
  mCacheValid = true;
  
  // First normalize the 4-vector
  double norm = Magnitude();
  if (norm == 0.0)
    return;

  double rnorm = 1.0/norm;
  double q1 = rnorm*Entry(1);
  double q2 = rnorm*Entry(2);
  double q3 = rnorm*Entry(3);
  double q4 = rnorm*Entry(4);

  // Now compute the transformation matrix.
  double q1q1 = q1*q1;
  double q2q2 = q2*q2;
  double q3q3 = q3*q3;
  double q4q4 = q4*q4;
  double q1q2 = q1*q2;
  double q1q3 = q1*q3;
  double q1q4 = q1*q4;
  double q2q3 = q2*q3;
  double q2q4 = q2*q4;
  double q3q4 = q3*q4;
  
  mT(1,1) = q1q1 + q2q2 - q3q3 - q4q4;
  mT(1,2) = 2.0*(q2q3 + q1q4);
  mT(1,3) = 2.0*(q2q4 - q1q3);
  mT(2,1) = 2.0*(q2q3 - q1q4);
  mT(2,2) = q1q1 - q2q2 + q3q3 - q4q4;
  mT(2,3) = 2.0*(q3q4 + q1q2);
  mT(3,1) = 2.0*(q2q4 + q1q3);
  mT(3,2) = 2.0*(q3q4 - q1q2);
  mT(3,3) = q1q1 - q2q2 - q3q3 + q4q4;
  // Since this is an orthogonal matrix, the inverse is simply
  // the transpose.
  mTInv = mT;
  mTInv.T();
  
  // Compute the Euler-angles
  if (mT(3,3) == 0.0)
    mEulerAngles(ePhi) = 0.5*M_PI;
  else
    mEulerAngles(ePhi) = atan2(mT(2,3), mT(3,3));
  
  if (mT(1,3) < -1.0)
    mEulerAngles(eTht) = 0.5*M_PI;
  else if (1.0 < mT(1,3))
    mEulerAngles(eTht) = -0.5*M_PI;
  else
    mEulerAngles(eTht) = asin(-mT(1,3));
  
  if (mT(1,1) == 0.0)
    mEulerAngles(ePsi) = 0.5*M_PI;
  else {
    double psi = atan2(mT(1,2), mT(1,1));
    if (psi < 0.0)
      psi += 2*M_PI;
    mEulerAngles(ePsi) = psi;
  }
  
  // FIXME: may be one can compute those values easier ???
  mEulerSines(ePhi) = sin(mEulerAngles(ePhi));
  // mEulerSines(eTht) = sin(mEulerAngles(eTht));
  mEulerSines(eTht) = -mT(1,3);
  mEulerSines(ePsi) = sin(mEulerAngles(ePsi));
  mEulerCosines(ePhi) = cos(mEulerAngles(ePhi));
  mEulerCosines(eTht) = cos(mEulerAngles(eTht));
  mEulerCosines(ePsi) = cos(mEulerAngles(ePsi));
}

} // namespace JSBSim
