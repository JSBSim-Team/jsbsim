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
#  include <iomanip>
#else
#  include STL_IOMANIP
#endif

static const char *IdSrc = "$Id: FGCoefficient.cpp,v 1.31 2001/04/10 13:07:53 jberndt Exp $";
static const char *IdHdr = "ID_COEFFICIENT";

extern char highint[5];
extern char halfint[5];
extern char normint[6];
extern char reset[5];
extern char underon[5];
extern char underoff[6];
extern char fgblue[6];
extern char fgcyan[6];
extern char fgred[6];
extern char fggreen[6];
extern char fgdef[6];

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGCoefficient::FGCoefficient(FGFDMExec* fdex, FGConfigFile* AC_cfg)
{
  int start, end, n;
  string multparms;

  FDMExec = fdex;
  State   = FDMExec->GetState();
  Table   = 0;

  if (AC_cfg) {
    name = AC_cfg->GetValue("NAME");
    method = AC_cfg->GetValue("TYPE");

    AC_cfg->GetNextConfigLine();
    *AC_cfg >> description;

    cout << "\n   " << highint << underon << name << underoff << normint << endl;
    cout << "   " << description << endl;
    cout << "   " << method << endl;

    if      (method == "EQUATION") type = EQUATION;
    else if (method == "TABLE")    type = TABLE;
    else if (method == "VECTOR")   type = VECTOR;
    else if (method == "VALUE")    type = VALUE;
    else                           type = UNKNOWN;

    if (type == VECTOR || type == TABLE) {
      *AC_cfg >> rows;
      cout << "   Rows: " << rows << " ";
      if (type == TABLE) {
        *AC_cfg >> columns;
        cout << "Cols: " << columns;
        Table = new FGTable(rows, columns);
      } else {
        Table = new FGTable(rows);
      }

      cout << endl;

      *AC_cfg >> multparms;
      LookupR = State->GetParameterIndex(multparms);
      cout << "   Row indexing parameter: " << multparms << endl;
    }

    if (type == TABLE) {
      *AC_cfg >> multparms;
      LookupC = State->GetParameterIndex(multparms);
      cout << "   Column indexing parameter: " << multparms << endl;
    }

    // Here, read in the line of the form (e.g.) FG_MACH|FG_QBAR|FG_ALPHA
    // where each non-dimensionalizing parameter for this coefficient is
    // separated by a | character

    *AC_cfg >> multparms;

    end   = multparms.length();
    n     = multparms.find("|");
    start = 0;

    while (n < end && n >= 0) {
      n -= start;
      multipliers.push_back(State->GetParameterIndex(multparms.substr(start,n)));
      start += n+1;
      n = multparms.find("|",start);
    }

    multipliers.push_back(State->GetParameterIndex(multparms.substr(start,n)));

    // End of non-dimensionalizing parameter read-in

    switch(type) {
    case VALUE:
      *AC_cfg >> StaticValue;
      cout << "      Value = " << StaticValue << endl;
      break;
    case VECTOR:
    case TABLE:
      *Table << *AC_cfg;
      Table->Print();

      break;
    case EQUATION:
    case UNKNOWN:
      cerr << "Unimplemented coefficient type: " << type << endl;
      break;
    }
    AC_cfg->GetNextConfigLine();
  }
  if (debug_lvl & 2) cout << "Instantiated: FGCoefficient" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGCoefficient::~FGCoefficient()
{
  if (Table) delete Table;
  if (debug_lvl & 2) cout << "Destroyed:    FGCoefficient" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGCoefficient::Value(float rVal, float cVal)
{
  float Value;
  unsigned int midx;

  SD = Value = Table->GetValue(rVal, cVal);

  for (midx=0; midx < multipliers.size(); midx++) {
      Value *= State->GetParameter(multipliers[midx]);
  }
  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGCoefficient::Value(float Val)
{
  float Value;

  SD = Value = Table->GetValue(Val);
  
  for (unsigned int midx=0; midx < multipliers.size(); midx++) 
      Value *= State->GetParameter(multipliers[midx]);
  
  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGCoefficient::Value(void)
{
	float Value;

  SD = Value = StaticValue;

  for (unsigned int midx=0; midx < multipliers.size(); midx++)
    Value *= State->GetParameter(multipliers[midx]);

  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGCoefficient::TotalValue()
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

void FGCoefficient::Debug(void)
{
    //TODO: Add your source code here
}

