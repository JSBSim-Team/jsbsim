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

#if !defined ( sgi ) || defined( __GNUC__ )
using std::cerr;
using std::endl;
using std::cout;
#endif

namespace JSBSim {

static const char *IdSrc = "$Id: FGTank.cpp,v 1.28 2003/06/03 09:53:50 ehofman Exp $";
static const char *IdHdr = ID_TANK;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTank::FGTank(FGConfigFile* AC_cfg)
{
  string token;
  
  type = AC_cfg->GetValue("TYPE");

  if      (type == "FUEL")     Type = ttFUEL;
  else if (type == "OXIDIZER") Type = ttOXIDIZER;
  else                         Type = ttUNKNOWN;
  
  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != string("/AC_TANK")) {
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

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTank::~FGTank()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTank::Reduce(double used)
{
  double shortage = Contents - used;

  if (shortage >= 0) {
    Contents -= used;
    PctFull = 100.0*Contents/Capacity;
  } else {
    Contents = 0.0;
    PctFull = 0.0;
    Selected = false;
  }
  return shortage;
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

void FGTank::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      " << type << " tank holds " << Capacity << " lbs. " << type << endl;
      cout << "      currently at " << PctFull << "% of maximum capacity" << endl;
      cout << "      Tank location (X, Y, Z): " << X << ", " << Y << ", " << Z << endl;
      cout << "      Effective radius: " << Radius << " inches" << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTank" << endl;
    if (from == 1) cout << "Destroyed:    FGTank" << endl;
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
