/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGQuaternion.cpp
 Author:       Jon Berndt, Mathias Froehlich
 Date started: 12/02/98

 ------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) ------------------
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
#include <sstream>
#include <iomanip>
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
  
static const char *IdSrc = "$Id: FGQuaternion.cpp,v 1.21 2012/09/05 05:00:57 jberndt Exp $";
static const char *IdHdr = ID_QUATERNION;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Initialize from q
FGQuaternion::FGQuaternion(const FGQuaternion& q) : mCacheValid(q.mCacheValid)
{
  data[0] = q(1);
  data[1] = q(2);
  data[2] = q(3);
  data[3] = q(4);
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
FGQuaternion::FGQuaternion(double phi, double tht, double psi): mCacheValid(false)
{
  InitializeFromEulerAngles(phi, tht, psi);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGQuaternion::FGQuaternion(FGColumnVector3 vOrient): mCacheValid(false)
{
  double phi = vOrient(ePhi);
  double tht = vOrient(eTht);
  double psi = vOrient(ePsi);

  InitializeFromEulerAngles(phi, tht, psi);
}
 
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// This function computes the quaternion that describes the relationship of the
// body frame with respect to another frame, such as the ECI or ECEF frames. The
// Euler angles used represent a 3-2-1 (psi, theta, phi) rotation sequence from
// the reference frame to the body frame. See "Quaternions and Rotation
// Sequences", Jack B. Kuipers, sections 9.2 and 7.6. 

void FGQuaternion::InitializeFromEulerAngles(double phi, double tht, double psi)
{
  mEulerAngles(ePhi) = phi;
  mEulerAngles(eTht) = tht;
  mEulerAngles(ePsi) = psi;

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
  
  data[0] = Cphid2Cthtd2*Cpsid2 + Sphid2Sthtd2*Spsid2;
  data[1] = Sphid2Cthtd2*Cpsid2 - Cphid2Sthtd2*Spsid2;
  data[2] = Cphid2Sthtd2*Cpsid2 + Sphid2Cthtd2*Spsid2;
  data[3] = Cphid2Cthtd2*Spsid2 - Sphid2Sthtd2*Cpsid2;

  Normalize();
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Initialize with a direction cosine (rotation) matrix

FGQuaternion::FGQuaternion(const FGMatrix33& m) : mCacheValid(false)
{
  data[0] = 0.50*sqrt(1.0 + m(1,1) + m(2,2) + m(3,3));
  double t = 0.25/data[0];
  data[1] = t*(m(2,3) - m(3,2));
  data[2] = t*(m(3,1) - m(1,3));
  data[3] = t*(m(1,2) - m(2,1));

  Normalize();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/** Returns the derivative of the quaternion corresponding to the
    angular velocities PQR.
    See Stevens and Lewis, "Aircraft Control and Simulation", Second Edition,
    Equation 1.3-36.
    Also see Jack Kuipers, "Quaternions and Rotation Sequences", Equation 11.12.
*/
FGQuaternion FGQuaternion::GetQDot(const FGColumnVector3& PQR) const
{
  return FGQuaternion(
    -0.5*( data[1]*PQR(eP) + data[2]*PQR(eQ) + data[3]*PQR(eR)),
     0.5*( data[0]*PQR(eP) - data[3]*PQR(eQ) + data[2]*PQR(eR)),
     0.5*( data[3]*PQR(eP) + data[0]*PQR(eQ) - data[1]*PQR(eR)),
     0.5*(-data[2]*PQR(eP) + data[1]*PQR(eQ) + data[0]*PQR(eR))
  );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGQuaternion::Normalize()
{
  // Note: this does not touch the cache since it does not change the orientation
  double norm = Magnitude();
  if (norm == 0.0 || fabs(norm - 1.000) < 1e-10) return;

  double rnorm = 1.0/norm;

  data[0] *= rnorm;
  data[1] *= rnorm;
  data[2] *= rnorm;
  data[3] *= rnorm;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Compute the derived values if required ...
void FGQuaternion::ComputeDerivedUnconditional(void) const
{
  mCacheValid = true;

  double q0 = data[0]; // use some aliases/shorthand for the quat elements.
  double q1 = data[1];
  double q2 = data[2];
  double q3 = data[3];

  // Now compute the transformation matrix.
  double q0q0 = q0*q0;
  double q1q1 = q1*q1;
  double q2q2 = q2*q2;
  double q3q3 = q3*q3;
  double q0q1 = q0*q1;
  double q0q2 = q0*q2;
  double q0q3 = q0*q3;
  double q1q2 = q1*q2;
  double q1q3 = q1*q3;
  double q2q3 = q2*q3;
  
  mT(1,1) = q0q0 + q1q1 - q2q2 - q3q3; // This is found from Eqn. 1.3-32 in
  mT(1,2) = 2.0*(q1q2 + q0q3);         // Stevens and Lewis
  mT(1,3) = 2.0*(q1q3 - q0q2);
  mT(2,1) = 2.0*(q1q2 - q0q3);
  mT(2,2) = q0q0 - q1q1 + q2q2 - q3q3;
  mT(2,3) = 2.0*(q2q3 + q0q1);
  mT(3,1) = 2.0*(q1q3 + q0q2);
  mT(3,2) = 2.0*(q2q3 - q0q1);
  mT(3,3) = q0q0 - q1q1 - q2q2 + q3q3;

  // Since this is an orthogonal matrix, the inverse is simply the transpose.

  mTInv = mT;
  mTInv.T();
  
  // Compute the Euler-angles
  // Also see Jack Kuipers, "Quaternions and Rotation Sequences", section 7.8..

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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::string FGQuaternion::Dump(const std::string& delimiter) const
{
  std::ostringstream buffer;
  buffer << std::setprecision(16) << data[0] << delimiter;
  buffer << std::setprecision(16) << data[1] << delimiter;
  buffer << std::setprecision(16) << data[2] << delimiter;
  buffer << std::setprecision(16) << data[3];
  return buffer.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

std::ostream& operator<<(std::ostream& os, const FGQuaternion& q)
{
  os << q(1) << " , " << q(2) << " , " << q(3) << " , " << q(4);
  return os;
}

} // namespace JSBSim
