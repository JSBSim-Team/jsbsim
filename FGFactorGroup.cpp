/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFactorGroup.cpp
 Author:       Tony Peden
 Date started: 7/21/01
 Purpose:      Encapsulates coefficients in the mathematical construct:
               factor*(coeff1 + coeff2 + coeff3 + ... + coeffn)
 Called by:    FGAerodynamics

 ------------- Copyright (C) 2001 Tony Peden (apeden@earthlink.net) -----------

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
This class models the stability derivative coefficient lookup tables or
equations. Note that the coefficients need not be calculated each delta-t.

Note that the values in a row which index into the table must be the same value
for each column of data, so the first column of numbers for each altitude are
seen to be equal, and there are the same number of values for each altitude.

See the header file FGFactorGroup.h for the values of the identifiers.

HISTORY
--------------------------------------------------------------------------------
7/21/01   TP  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGCoefficient.h"
#include "FGFactorGroup.h"
#include "FGState.h"
#include "FGFDMExec.h"

#ifndef FGFS
#  if defined(sgi) && !defined(__GNUC__)
#    include <iomanip.h>
#  else
#    include <iomanip>
#  endif
#else
#  include STL_IOMANIP
#endif

static const char *IdSrc = "$Id: FGFactorGroup.cpp,v 1.17 2002/03/18 12:12:47 apeden Exp $";
static const char *IdHdr = ID_FACTORGROUP;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFactorGroup::FGFactorGroup( FGFDMExec* fdmex ) : FGCoefficient( fdmex)
{
  FDMExec = fdmex;
  totalValue = 0;
  Debug(0);
}  

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
                          
FGFactorGroup::~FGFactorGroup()
{
  for (unsigned int i=0; i<sum.size(); i++) delete sum[i];

  Debug(1);
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

bool FGFactorGroup::Load(FGConfigFile *AC_cfg)
{
  string token;
  
  if (AC_cfg) {
    name = AC_cfg->GetValue("NAME");
    AC_cfg->GetNextConfigLine();
    *AC_cfg >> description;
    token = AC_cfg->GetValue();
    if (token == "FACTOR") {
      FGCoefficient::Load(AC_cfg);
    } 
    token = AC_cfg->GetValue();  
    while (token != string("/GROUP") ) {
      sum.push_back( new FGCoefficient(FDMExec) );
      sum.back()->Load(AC_cfg);
      token = AC_cfg->GetValue(); 
    }
    AC_cfg->GetNextConfigLine();
    return true;
  } else {
    return false;
  }  
}  

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

double FGFactorGroup::TotalValue(void)
{
  unsigned int i;
  SDtotal = 0.0;
  totalValue = 0.0;
  for (i=0; i<sum.size(); i++) {
     totalValue += sum[i]->TotalValue();
     SDtotal += sum[i]->GetSD();
  }
  //cout << totalValue << "  " << FGCoefficient::TotalValue() << endl;
  totalValue *= FGCoefficient::TotalValue();
  SDtotal *= FGCoefficient::GetSD();
  Debug(2);
  return totalValue;
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
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

void FGFactorGroup::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFactorGroup" << endl;
    if (from == 1) cout << "Destroyed:    FGFactorGroup" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
    if (from == 2) {
      cout << "FGCoefficient::GetSD(): " << FGCoefficient::GetSD() << endl;
      cout << "FGFactorGroup::SDtotal: " << SDtotal << endl;
    }
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

