/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTank.cpp
 Author:       Jon Berndt
 Date started: 01/21/99
 Called by:    FGAircraft

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
See header file.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGTank.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGTank.cpp,v 1.10 2001/02/02 01:17:01 jsb Exp $";
static const char *IdHdr = ID_TANK;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

using std::cerr;
using std::endl;
using std::cout;

FGTank::FGTank(FGConfigFile* AC_cfg)
{
  string type = AC_cfg->GetValue("TYPE");
  string token;
  
  if      (type == "FUEL")     Type = ttFUEL;
  else if (type == "OXIDIZER") Type = ttOXIDIZER;
  else                         Type = ttUNKNOWN;
  
  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != "/AC_TANK") {
    if (token == "XLOC") *AC_cfg >> X;
    else if (token == "YLOC") *AC_cfg >> Y;
    else if (token == "ZLOC") *AC_cfg >> Z;
    else if (token == "RADIUS") *AC_cfg >> Radius;
    else if (token == "CAPACITY") *AC_cfg >> Capacity;
    else if (token == "CONTENTS") *AC_cfg >> Contents;
    else cerr << "Unknown identifier: " << token << " in tank definition." << endl;
  }
  
  Selected = true;

  if (Capacity != 0) {
    PctFull = 100.0*Contents/Capacity;            // percent full; 0 to 100.0
  } else {
    Contents = 0;
    PctFull  = 0;
  }     

  cout << "      " << type << " tank holds " << Capacity << " lbs. " << type << endl;
  cout << "      currently at " << PctFull << "% of maximum capacity" << endl;
  cout << "      Tank location (X, Y, Z): " << X << ", " << Y << ", " << Z << endl;
  cout << "      Effective radius: " << Radius << " inches" << endl;
}


FGTank::~FGTank(void)
{
}


float FGTank::Reduce(float used)
{
  float shortage;

  if (used < Contents) {
    Contents -= used;
    PctFull = 100.0*Contents/Capacity;
    return 0.0;
  } else {
    shortage = Contents - used;
    Contents = 0.0;
    PctFull = 0.0;
    Selected = false;
    return shortage;
  }
}

