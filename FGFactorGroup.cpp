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

static const char *IdSrc = "$Id: FGFactorGroup.cpp,v 1.11 2001/12/01 17:58:41 apeden Exp $";
static const char *IdHdr = ID_FACTORGROUP;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFactorGroup::FGFactorGroup( FGFDMExec* fdmex ) : FGCoefficient( fdmex)
{
  FDMExec=fdmex;
  if (debug_lvl & 2) cout << "Instantiated: FGFactorGroup" << endl;
}  

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
                          
FGFactorGroup::~FGFactorGroup()
{
  unsigned i;
  for (i=0; i<sum.size(); i++) {
    delete sum[i];
  }
  if (debug_lvl & 2) cout << "Destroyed:    FGFactorGroup" << endl;
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

bool FGFactorGroup::Load(FGConfigFile *AC_cfg) {
  string token;
  
  if(AC_cfg) {
    name = AC_cfg->GetValue("NAME");
    AC_cfg->GetNextConfigLine();
    *AC_cfg >> description;
    token = AC_cfg->GetValue();
    if( token == "FACTOR") {
      FGCoefficient::Load(AC_cfg);
      //if (debug_lvl > 0) DisplayCoeffFactors(ca.back()->Getmultipliers());
    } 
    token = AC_cfg->GetValue();  
    while ( token != string("/GROUP") ) {
          sum.push_back( new FGCoefficient(FDMExec) );
          sum.back()->Load(AC_cfg);
          //if (debug_lvl > 0) DisplayCoeffFactors(ca.back()->Getmultipliers());
          token = AC_cfg->GetValue(); 
    }
    AC_cfg->GetNextConfigLine();
    return true;
  } else {
    return false;
  }  
}  

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

double FGFactorGroup::TotalValue(void) {
  unsigned i;
  double totalsum=0;
  SDtotal=0.0;
  for(i=0;i<sum.size();i++) {
     totalsum+=sum[i]->TotalValue();
     SDtotal += sum[i]->GetSD();
  }
  totalsum *= FGCoefficient::TotalValue();
  SDtotal *= FGCoefficient::GetSD();
  if (debug_lvl & 8) Debug();
  return totalsum;
}        

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

void FGFactorGroup::Debug(void)
{
  cout << "FGCoefficient::GetSD(): " << FGCoefficient::GetSD() << endl;
  cout << "FGFactorGroup::SDtotal: " << SDtotal << endl;
}

