/*******************************************************************************

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

********************************************************************************
INCLUDES
*******************************************************************************/
#include "FGTank.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGTank::FGTank(FGConfigFile* AC_cfg)
{
  string type;

  *AC_cfg >> type;                              // Type = 0: fuel, 1: oxidizer

  if (type == "FUEL") Type = ttFUEL;
  else if (type == "OXIDIZER") Type = ttOXIDIZER;
  else Type = ttUNKNOWN;
  *AC_cfg >> X;                                 // inches
  *AC_cfg >> Y;                                 // "
  *AC_cfg >> Z;                                 // "
  *AC_cfg >> Radius;                            // "
  *AC_cfg >> Capacity;                          // pounds (amount it can hold)
  *AC_cfg >> Contents;                          // pounds  (amount it is holding)
  Selected = true;
  PctFull = 100.0*Contents/Capacity;            // percent full; 0 to 100.0
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
    return Contents;
  } else {
    shortage = Contents - used;
    Contents = 0.0;
    PctFull = 0.0;
    Selected = false;
    return shortage;
  }
}

