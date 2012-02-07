/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Module: FGColumnVector3.cpp
Author: Originally by Tony Peden [formatted here by JSB]
Date started: 1998
Purpose: FGColumnVector3 class
Called by: Various

 ------------- Copyright (C) 1998 Tony Peden and Jon S. Berndt (jon@jsbsim.org) -

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

#include "FGColumnVector3.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGColumnVector3.cpp,v 1.15 2012/02/07 00:27:51 jentron Exp $";
static const char *IdHdr = ID_COLUMNVECTOR3;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGColumnVector3::FGColumnVector3(void)
{
  data[0] = data[1] = data[2] = 0.0;
  // Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGColumnVector3::Dump(const string& delimiter) const
{
  ostringstream buffer;
  buffer << std::setprecision(16) << data[0] << delimiter;
  buffer << std::setprecision(16) << data[1] << delimiter;
  buffer << std::setprecision(16) << data[2];
  return buffer.str();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ostream& operator<<(ostream& os, const FGColumnVector3& col)
{
  os << col(1) << " , " << col(2) << " , " << col(3);
  return os;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3 FGColumnVector3::operator/(const double scalar) const
{
  if (scalar != 0.0)
    return operator*( 1.0/scalar );

  cerr << "Attempt to divide by zero in method \
    FGColumnVector3::operator/(const double scalar), \
    object " << data[0] << " , " << data[1] << " , " << data[2] << endl;
  return FGColumnVector3();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGColumnVector3::operator/=(const double scalar)
{
  if (scalar != 0.0)
    operator*=( 1.0/scalar );
  else
    cerr << "Attempt to divide by zero in method \
      FGColumnVector3::operator/=(const double scalar), \
      object " << data[0] << " , " << data[1] << " , " << data[2] << endl;

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGColumnVector3::Magnitude(void) const
{
  return sqrt( data[0]*data[0] +  data[1]*data[1] +  data[2]*data[2] );
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGColumnVector3::Normalize(void)
{
  double Mag = Magnitude();

  if (Mag != 0.0)
    operator*=( 1.0/Mag );

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGColumnVector3::Magnitude(const int idx1, const int idx2) const {
  return sqrt( data[idx1-1]*data[idx1-1] +  data[idx2-1]*data[idx2-1] );
}


} // namespace JSBSim
