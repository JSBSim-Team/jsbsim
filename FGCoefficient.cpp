/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGCoefficient.cpp
 Author:       Jon S. Berndt
 Date started: 12/28/98
 Purpose:      Encapsulates the stability derivative class FGCoefficient;
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
This class models the stability derivative coefficient lookup tables or
equations. Note that the coefficients need not be calculated each delta-t.

Note that the values in a row which index into the table must be the same value
for each column of data, so the first column of numbers for each altitude are
seen to be equal, and there are the same number of values for each altitude.

See the header file FGCoefficient.h for the values of the identifiers.

HISTORY
--------------------------------------------------------------------------------
12/28/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGCoefficient.h"
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

static const char *IdSrc = "$Id: FGCoefficient.cpp,v 1.50 2002/02/14 19:16:38 jberndt Exp $";
static const char *IdHdr = ID_COEFFICIENT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGCoefficient::FGCoefficient( FGFDMExec* fdex )
{

  FDMExec = fdex;
  State   = FDMExec->GetState();
  Table   = 0;
  
  bias=0;
  gain=1;

  if (debug_lvl & 2) cout << "Instantiated: FGCoefficient" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGCoefficient::~FGCoefficient()
{
  if (Table) delete Table;
  if (debug_lvl & 2) cout << "Destroyed:    FGCoefficient" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGCoefficient::Load(FGConfigFile *AC_cfg)
{
  int start, end, n;
  string mult;

  if (AC_cfg) {
    name = AC_cfg->GetValue("NAME");
    method = AC_cfg->GetValue("TYPE");
    AC_cfg->GetNextConfigLine();
    *AC_cfg >> description;

    if      (method == "EQUATION") type = EQUATION;
    else if (method == "TABLE")    type = TABLE;
    else if (method == "VECTOR")   type = VECTOR;
    else if (method == "VALUE")    type = VALUE;
    else                           type = UNKNOWN;

    if (type == VECTOR || type == TABLE) {
      *AC_cfg >> rows;
      if (type == TABLE) {
        *AC_cfg >> columns;
        Table = new FGTable(rows, columns);
      } else {
        Table = new FGTable(rows);
      }

      *AC_cfg >> multparmsRow;
      LookupR = State->GetParameterIndex(multparmsRow);
    }

    if (type == TABLE) {
      *AC_cfg >> multparmsCol;
      LookupC = State->GetParameterIndex(multparmsCol);
    }

    // Here, read in the line of the form (e.g.) FG_MACH|FG_QBAR|FG_ALPHA
    // where each non-dimensionalizing parameter for this coefficient is
    // separated by a | character

    *AC_cfg >> multparms;

    end   = multparms.length();
    n     = multparms.find("|");
    start = 0;

    if (multparms != string("FG_NONE")) {
      while (n < end && n >= 0) {
        n -= start;
        mult = multparms.substr(start,n);
        multipliers.push_back( State->GetParameterIndex(mult) );
        start += n+1;
        n = multparms.find("|",start);
      }
      multipliers.push_back(State->GetParameterIndex(multparms.substr(start,n)));
      // End of non-dimensionalizing parameter read-in
    }

    if (type == VALUE) {
      *AC_cfg >> StaticValue;
    } else if (type == VECTOR || type == TABLE) {
      *Table << *AC_cfg;
    } else {
      cerr << "Unimplemented coefficient type: " << type << endl;
    }

    AC_cfg->GetNextConfigLine();
    FGCoefficient::Debug(2);

    return true;
  } else {
    return false;
  }  
}



//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::Value(double rVal, double cVal)
{
  double Value;
  unsigned int midx;

  SD = Value = gain*Table->GetValue(rVal, cVal) + bias;
  

  for (midx=0; midx < multipliers.size(); midx++) {
      Value *= State->GetParameter(multipliers[midx]);
  }
  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::Value(double Val)
{
  double Value;

  SD = Value = gain*Table->GetValue(Val) + bias;
  
  for (unsigned int midx=0; midx < multipliers.size(); midx++) 
      Value *= State->GetParameter(multipliers[midx]);
  
  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::Value(void)
{
	double Value;

  SD = Value = gain*StaticValue + bias;

  for (unsigned int midx=0; midx < multipliers.size(); midx++)
    Value *= State->GetParameter(multipliers[midx]);

  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGCoefficient::TotalValue()
{
  switch(type) {
  case 0:
    return -1;
  case 1:
    return (Value());
  case 2:
    return (Value(State->GetParameter(LookupR)));
  case 3:
    return (Value(State->GetParameter(LookupR),State->GetParameter(LookupC)));
  case 4:
    return 0.0;
  }
  return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCoefficient::DumpSD(void)
{
  cout << "   " << name << ": " << SD << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGCoefficient::DisplayCoeffFactors(void)
{
  unsigned int i;

  cout << "   Non-Dimensionalized by: ";

  if (multipliers.size() == 0) {
    cout << "none" << endl;
  } else {
    for (i=0; i<multipliers.size(); i++) 
      cout << State->GetParameterName(multipliers[i]);
  }
  cout << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGCoefficient::GetCoefficientValues(void)
{
  char buffer[10];
  string value;

  sprintf(buffer,"%9.6f",SD);
  value = string(buffer);
  return value;
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

void FGCoefficient::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    
    if (from == 2) { // Loading
      cout << "\n   " << highint << underon << name << underoff << normint << endl;
      cout << "   " << description << endl;
      cout << "   " << method << endl;

      if (type == VECTOR || type == TABLE) {
        cout << "   Rows: " << rows << " ";
        if (type == TABLE) {
          cout << "Cols: " << columns;
        }
        cout << endl << "   Row indexing parameter: " << multparmsRow << endl;
      }

      if (type == TABLE) {
        cout << "   Column indexing parameter: " << multparmsCol << endl;
      }

      if (type == VALUE) {
        cout << "      Value = " << StaticValue << endl;
      } else if (type == VECTOR || type == TABLE) {
        Table->Print();
      }

      DisplayCoeffFactors();
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGCoefficient" << endl;
    if (from == 1) cout << "Destroyed:    FGCoefficient" << endl;
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

