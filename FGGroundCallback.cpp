/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGroundCallback.cpp
 Author:       Mathias Froehlich
 Date started: 05/21/04

 ------ Copyright (C) 2004 Mathias Froehlich (Mathias.Froehlich@web.de) -------

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
05/21/00   MF   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGColumnVector3.h"
#include "FGLocation.h"
#include "FGGroundCallback.h"

namespace JSBSim {

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGroundCallback::FGGroundCallback()
{
  mReferenceRadius = 2.0902264e7;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGroundCallback::~FGGroundCallback()
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGGroundCallback::GetAltitude(const FGLocation& l) const
{
  return l.GetRadius() - mReferenceRadius;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGGroundCallback::GetAGLevel(double t, const FGLocation& l,
                                    FGLocation& cont, FGColumnVector3& n,
                                    FGColumnVector3& v) const
{
  v = FGColumnVector3(0.0, 0.0, 0.0);
  n = FGColumnVector3(0.0, 0.0, -1.0);
  double agl = GetAltitude(l);
  cont = agl/mReferenceRadius*FGColumnVector3(l);
  return agl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundCallback::CaughtWire(double t, const FGLocation l[4]) const
{
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundCallback::GetWire(double t, FGLocation c[2], FGColumnVector3 v[2]) const
{
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGroundCallback::ReleaseWire(void) const
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGGroundCallback::GetCatapult(double t, const FGLocation& lb,
                                     FGLocation c[2], FGColumnVector3 v[2]) const
{
  return DBL_MAX;
}

}
