/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGMars.cpp
 Author:       Jon Berndt
 Date started: 1/4/04
 Purpose:      Models the Martian atmosphere very simply
 Called by:    FGFDMExec

 ------------- Copyright (C) 2004  Jon S. Berndt (jon@jsbsim.org) -------------

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
Models the Martian atmosphere.

HISTORY
--------------------------------------------------------------------------------
1/04/2004   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>

#include "FGMars.h"
#include "FGFDMExec.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMars::FGMars(FGFDMExec* fdmex) : FGAtmosphere(fdmex)
{
  Name = "FGMars";
  Reng = 53.5 * 44.01;

  bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMars::Calculate(double altitude)
{
  //Calculate reftemp, refpress, and density

  // LIMIT the temperatures so they do not descend below absolute zero.

  if (altitude < 22960.0) {
    Temperature = -25.68 - 0.000548*altitude; // Deg Fahrenheit
  } else {
    Temperature = -10.34 - 0.001217*altitude; // Deg Fahrenheit
  }
  Pressure = 14.62*exp(-0.00003*altitude); // psf - 14.62 psf =~ 7 millibars
  Density = Pressure/(Reng*Temperature); // slugs/ft^3 (needs deg R. as input

  //FGLogging log(FDMExec->GetLogger(), LogLevel::INFO);
  //log << "Atmosphere:  h=" << altitude << " rho= " << intDensity << "\n";
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

void FGMars::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(FDMExec->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGMars\n";
    if (from == 1) log << "Destroyed:    FGMars\n";
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 32) { // Turbulence
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}

} // namespace JSBSim
