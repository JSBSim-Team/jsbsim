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
#  include <iomanip>
#else
#  include STL_IOMANIP
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFactorGroup::FGFactorGroup( FGFDMExec* fdmex ) : FGCoefficient( fdmex) {
 FDMExec=fdmex;

}  

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
                          
FGFactorGroup::~FGFactorGroup() {
    int i;
    for (i=0; i<sum.size(); i++) {
      delete sum[i];
    }
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
    while ( token != "/GROUP" ) {
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

float FGFactorGroup::TotalValue(void) {
  int i;
  float totalsum=0;
  SDtotal=0.0;
  for(i=0;i<sum.size();i++) {
     totalsum+=sum[i]->TotalValue();
     SDtotal += sum[i]->GetSD();
  }
  totalsum *= FGCoefficient::TotalValue();
  SDtotal *= FGCoefficient::GetSD();
  cout << "FGCoefficient::GetSD(): " << FGCoefficient::GetSD() << endl;
  cout << "FGFactorGroup::SDtotal: " << SDtotal << endl;
  return totalsum;
}        

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

string FGFactorGroup::GetCoefficientStrings(void) {
  int i;
  string CoeffStrings;
  
  CoeffStrings += name;
  CoeffStrings += ", ";
  CoeffStrings += FGCoefficient::Getname();
  for(i=0;i<sum.size();i++) {
    CoeffStrings += ", ";
    CoeffStrings += sum[i]->Getname();
  }
  return CoeffStrings;    
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

string FGFactorGroup::GetCoefficientValues(void) {
    int i;
    char buffer[10];
    string values;
    
    snprintf(buffer,10,"%9.6f",SDtotal);
    values += string(buffer);
    values += ", ";
    snprintf(buffer,10,"%9.6f",FGCoefficient::GetSD() );
    values += string(buffer);
    values += ", ";
    for(i=0;i<sum.size();i++) {
       values += sum[i]->GetCoefficientValues();
    }
    return values;
}       
       
