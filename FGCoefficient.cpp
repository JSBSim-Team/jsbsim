/*******************************************************************************

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

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGCoefficient.h"
#include "FGState.h"
#include "FGFDMExec.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGCoefficient.cpp,v 1.16 2000/10/13 19:21:02 jsb Exp $";
static const char *IdHdr = "ID_COEFFICIENT";

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGCoefficient::FGCoefficient(FGFDMExec* fdex, FGConfigFile* AC_cfg)
{
  int r, c, start, end, n;
  float ftrashcan;
  string multparms;

  FDMExec     = fdex;
  State       = FDMExec->GetState();
  Table = 0;

  if (AC_cfg) {
    name = AC_cfg->GetValue("NAME");
    method = AC_cfg->GetValue("TYPE");

    AC_cfg->GetNextConfigLine();
    *AC_cfg >> description;

    cout << "   " << name << endl;
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
      Allocate(rows,2);

      for (r=1;r<=rows;r++) {
        *AC_cfg >> Table[r][0];
        *AC_cfg >> Table[r][1];
      }

      for (r=1;r<=rows;r++) {
        cout << "	";
        for (c=0;c<columns;c++) {
          cout << Table[r][c] << "	";
        }
        cout << endl;
      }

      break;
    case TABLE:
      Allocate(rows, columns);

      Table[0][0] = 0.0;
      for (c=1;c<=columns;c++) {
        *AC_cfg >> Table[0][c];
        for (r=1;r<=rows;r++) {
          if ( c==1 ) *AC_cfg >> Table[r][0];
          else *AC_cfg >> ftrashcan;
          *AC_cfg >> Table[r][c];
        }
      }

      for (r=0;r<=rows;r++) {
        cout << "	";
        for (c=0;c<=columns;c++) {
          cout << Table[r][c] << "	";
        }
        cout << endl;
      }

      break;
    case EQUATION:
    case UNKNOWN:
      cerr << "Unimplemented coefficient type: " << type << endl;
      break;  
    }
    AC_cfg->GetNextConfigLine();
  }
}

/******************************************************************************/

FGCoefficient::~FGCoefficient(void) {
  DeAllocate();
}

/******************************************************************************/

bool FGCoefficient::DeAllocate(void)
{
  if (Table != NULL ) {
    for (unsigned int i=0; i<=rows; i++) delete[] Table[i];
    
    delete[] Table;
  } 
  
  return true;
}

/******************************************************************************/

bool FGCoefficient::Allocate(int r, int c)
{
  rows = r;
  columns = c;
  Table = new float*[r+1];
  for (int i=0;i<=r;i++) Table[i] = new float[c+1];
  return true;
}

/******************************************************************************/

float FGCoefficient::Value(float rVal, float cVal)
{
  float rFactor, cFactor, col1temp, col2temp, Value;
  int r, c;
  unsigned midx;

  if (rows < 2 || columns < 2) return 0.0;

  for (r=1;r<=rows;r++) if (Table[r][0] >= rVal) break;
  for (c=1;c<=columns;c++) if (Table[0][c] >= cVal) break;

  c = c < 2 ? 2 : (c > columns ? columns : c);
  r = r < 2 ? 2 : (r > rows    ? rows    : r);

  rFactor = (rVal - Table[r-1][0]) / (Table[r][0] - Table[r-1][0]);
  cFactor = (cVal - Table[0][c-1]) / (Table[0][c] - Table[0][c-1]);

  col1temp = rFactor*(Table[r][c-1] - Table[r-1][c-1]) + Table[r-1][c-1];
  col2temp = rFactor*(Table[r][c] - Table[r-1][c]) + Table[r-1][c];

  SD = Value = col1temp + cFactor*(col2temp - col1temp);

  for (midx=0; midx < multipliers.size(); midx++) {
    Value *= State->GetParameter(multipliers[midx]);
  }

  return Value;
}

/******************************************************************************/

float FGCoefficient::Value(float Val)
{
  
  
  float Factor, Value;
  int r;
  unsigned midx;

  if (rows < 2) return 0.0;

  for (r=1;r<=rows;r++) if (Table[r][0] >= Val) break;
  r = r < 2 ? 2 : (r > rows    ? rows    : r);

  // make sure denominator below does not go to zero.
  if (Table[r][0] != Table[r-1][0]) {
    Factor = (Val - Table[r-1][0]) / (Table[r][0] - Table[r-1][0]);
  } else {
    Factor = 1.0;
  }

  SD = Value = Factor*(Table[r][1] - Table[r-1][1]) + Table[r-1][1];
  for (midx=0; midx < multipliers.size(); midx++) {
    Value *= State->GetParameter(multipliers[midx]);

  }

  return Value;
}

/******************************************************************************/

float FGCoefficient::Value(void)
{
	float Value;
	unsigned midx;

	SD = Value = StaticValue;

  for (midx=0; midx < multipliers.size(); midx++) {
    Value *= State->GetParameter(multipliers[midx]);
  }

  return Value;
}

/******************************************************************************/

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

/******************************************************************************/

void FGCoefficient::DumpSD(void)
{
  cout << "   " << name << ": " << SD << endl;
}

/******************************************************************************/

