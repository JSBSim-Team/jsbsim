/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGQuaternion.cpp
 Author:       Jon Berndt, Mathias Froehlich
 Date started: 12/02/98

 ------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) ------------------
 -------           (C) 2004  Mathias Froehlich (Mathias.Froehlich@web.de) ----

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
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

#ifdef FGFS
#  include <math.h>
#  include <simgear/compiler.h>
#  include STL_IOSTREAM
   SG_USING_STD(cerr);
   SG_USING_STD(cout);
   SG_USING_STD(endl);
#else
#  include <string>
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <iostream.h>
#    include <math.h>
#  else
#    include <iostream>
#    if defined(sgi) && !defined(__GNUC__)
#      include <math.h>
#    else
#      include <cmath>
#    endif
     using std::cerr;
     using std::cout;
     using std::endl;
#  endif
#endif

#include "FGMatrix33.h"
#include "FGColumnVector3.h"

#include "FGQuaternion.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  DEFINITIONS
  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

static const char *IdSrc = "$Id: FGQuaternion.cpp,v 1.4 2004/04/17 21:16:19 jberndt Exp $";
static const char *IdHdr = ID_QUATERNION;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Initialize from q
FGQuaternion::FGQuaternion(const FGQuaternion& q)
  : mQ0(q.mQ0), mQ1(q.mQ1), mQ2(q.mQ2), mQ3(q.mQ3),
    mCacheValid(q.mCacheValid)
{
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

  mQ0 = Cphid2Cthtd2*Cpsid2 + Sphid2Sthtd2*Spsid2;
  mQ1 = Sphid2Cthtd2*Cpsid2 - Cphid2Sthtd2*Spsid2;
  mQ2 = Cphid2Sthtd2*Cpsid2 + Sphid2Cthtd2*Spsid2;
  mQ3 = Cphid2Cthtd2*Spsid2 - Sphid2Sthtd2*Cpsid2;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/**
   Returns the derivative of the quaternion coresponding to the
   angular velocities PQR.
*/
FGQuaternion FGQuaternion::GetQDot(const FGColumnVector3& PQR) const {
  FGQuaternion QDot;
  QDot.mQ0 = -0.5*(mQ1*PQR(eP) + mQ2*PQR(eQ) + mQ3*PQR(eR));
  QDot.mQ1 =  0.5*(mQ0*PQR(eP) + mQ2*PQR(eR) - mQ3*PQR(eQ));
  QDot.mQ2 =  0.5*(mQ0*PQR(eQ) + mQ3*PQR(eP) - mQ1*PQR(eR));
  QDot.mQ3 =  0.5*(mQ0*PQR(eR) + mQ1*PQR(eQ) - mQ2*PQR(eP));
  return QDot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Compute the derived values if required ...
void FGQuaternion::ComputeDerived(void) const {
  if (mCacheValid)
    return;

  mCacheValid = true;

  // First normalize the 4-vector
  double norm = sqrt(mQ0*mQ0+mQ1*mQ1+mQ2*mQ2+mQ3*mQ3);
  if (1e-2 < fabs(norm-1.0))
    std::cerr << "Renormalizing quaternion too much: error = "
              << fabs(norm-1.0) << std::endl;
  double rnorm = 1.0/norm;
  mQ0 *= rnorm;
  mQ1 *= rnorm;
  mQ2 *= rnorm;
  mQ3 *= rnorm;

  // Now compute the transformation matrix.
  double Q0Q0 = mQ0*mQ0;
  double Q1Q1 = mQ1*mQ1;
  double Q2Q2 = mQ2*mQ2;
  double Q3Q3 = mQ3*mQ3;
  double Q0Q1 = mQ0*mQ1;
  double Q0Q2 = mQ0*mQ2;
  double Q0Q3 = mQ0*mQ3;
  double Q1Q2 = mQ1*mQ2;
  double Q1Q3 = mQ1*mQ3;
  double Q2Q3 = mQ2*mQ3;

  mT(1,1) = Q0Q0 + Q1Q1 - Q2Q2 - Q3Q3;
  mT(1,2) = 2.0*(Q1Q2 + Q0Q3);
  mT(1,3) = 2.0*(Q1Q3 - Q0Q2);
  mT(2,1) = 2.0*(Q1Q2 - Q0Q3);
  mT(2,2) = Q0Q0 - Q1Q1 + Q2Q2 - Q3Q3;
  mT(2,3) = 2.0*(Q2Q3 + Q0Q1);
  mT(3,1) = 2.0*(Q1Q3 + Q0Q2);
  mT(3,2) = 2.0*(Q2Q3 - Q0Q1);
  mT(3,3) = Q0Q0 - Q1Q1 - Q2Q2 + Q3Q3;
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
