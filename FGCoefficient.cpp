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
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGCoefficient::CoeffMap FGCoefficient::coeffdef;

FGCoefficient::FGCoefficient(FGFDMExec* fdex, FGConfigFile* AC_cfg)
{
  int r, c, start, end, n;
  float ftrashcan;
  string strashcan;

  static bool FirstTime = true;
  if (FirstTime) {
    FirstTime = false;
    coeffdef["FG_QBAR"]      = 1;
    coeffdef["FG_WINGAREA"]  = 2;
    coeffdef["FG_WINGSPAN"]  = 4;
    coeffdef["FG_CBAR"]      = 8;
    coeffdef["FG_ALPHA"]     = 16;
    coeffdef["FG_ALPHADOT"]  = 32;
    coeffdef["FG_BETA"]      = 64;
    coeffdef["FG_BETADOT"]   = 128;
    coeffdef["FG_PITCHRATE"] = 256;
    coeffdef["FG_ROLLRATE"]  = 512;
    coeffdef["FG_YAWRATE"]   = 1024;
    coeffdef["FG_ELEVATOR"]  = 2048;
    coeffdef["FG_AILERON"]   = 4096;
    coeffdef["FG_RUDDER"]    = 8192;
    coeffdef["FG_MACH"]      = 16384;
    coeffdef["FG_ALTITUDE"]  = 32768L;
    coeffdef["FG_BI2VEL"]    = 65536L;
    coeffdef["FG_CI2VEL"]    = 131072L;
  }

  FDMExec     = fdex;
  State       = FDMExec->GetState();
  Atmosphere  = FDMExec->GetAtmosphere();
  FCS         = FDMExec->GetFCS();
  Aircraft    = FDMExec->GetAircraft();
  Translation = FDMExec->GetTranslation();
  Rotation    = FDMExec->GetRotation();
  Position    = FDMExec->GetPosition();
  Auxiliary   = FDMExec->GetAuxiliary();
  Output      = FDMExec->GetOutput();

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

      *AC_cfg >> strashcan;
      if (strashcan.substr(0,1) == "F") {
        LookupR = coeffdef[strashcan.c_str()];
        cout << "   Row indexing parameter: " << strashcan << endl;
      } else {
        LookupR = atoi(strashcan.c_str());
        cout << "   Row indexing parameter: " << LookupR << endl;
      }

    }

    if (type == TABLE) {
      *AC_cfg >> strashcan;
      if (strashcan.substr(0,1) == "F") {
        LookupC = coeffdef[strashcan.c_str()];
        cout << "   Column indexing parameter: " << strashcan << endl;
      } else {
        LookupC = atoi(strashcan.c_str());
        cout << "   Column indexing parameter: " << LookupC << endl;
      }
    }

    *AC_cfg >> strashcan;

    end   = strashcan.length();
    n     = strashcan.find("|");
    start = 0;
    multipliers = 0;
    if (strashcan.substr(0,1) == "F") {
      while(n < end && n >= 0) {
        n -= start;
        multipliers += coeffdef[strashcan.substr(start,n).c_str()];
        start += n+1;
        n = strashcan.find("|",start);
      }
      multipliers += coeffdef[strashcan.substr(start,end).c_str()];
    } else {
      multipliers = atoi(strashcan.c_str());
    }

    cout << "   Non-Dimensionalized by: ";

    mult_count = 0;
    if (multipliers & FG_QBAR) {
      mult_idx[mult_count] = FG_QBAR;
      mult_count++;
      cout << "qbar ";
    }
    if (multipliers & FG_WINGAREA) {
      mult_idx[mult_count] = FG_WINGAREA;
      mult_count++;
      cout << "S ";
    }
    if (multipliers & FG_WINGSPAN) {
      mult_idx[mult_count] = FG_WINGSPAN;
      mult_count++;
      cout << "b ";
    }
    if (multipliers & FG_CBAR) {
      mult_idx[mult_count] = FG_CBAR;
      mult_count++;
      cout << "c ";
    }
    if (multipliers & FG_ALPHA) {
      mult_idx[mult_count] = FG_ALPHA;
      mult_count++;
      cout << "alpha ";
    }
    if (multipliers & FG_ALPHADOT) {
      mult_idx[mult_count] = FG_ALPHADOT;
      mult_count++;
      cout << "alphadot ";
    }
    if (multipliers & FG_BETA) {
      mult_idx[mult_count] = FG_BETA;
      mult_count++;
      cout << "beta ";
    }
    if (multipliers & FG_BETADOT) {
      mult_idx[mult_count] = FG_BETADOT;
      mult_count++;
      cout << "betadot ";
    }
    if (multipliers & FG_PITCHRATE) {
      mult_idx[mult_count] = FG_PITCHRATE;
      mult_count++;
      cout << "q ";
    }
    if (multipliers & FG_ROLLRATE) {
      mult_idx[mult_count] = FG_ROLLRATE;
      mult_count++;
      cout << "p ";
    }
    if (multipliers & FG_YAWRATE) {
      mult_idx[mult_count] = FG_YAWRATE;
      mult_count++;
      cout << "r ";
    }
    if (multipliers & FG_ELEVATOR) {
      mult_idx[mult_count] = FG_ELEVATOR;
      mult_count++;
      cout << "De ";
    }
    if (multipliers & FG_AILERON) {
      mult_idx[mult_count] = FG_AILERON;
      mult_count++;
      cout << "Da ";
    }
    if (multipliers & FG_RUDDER) {
      mult_idx[mult_count] = FG_RUDDER;
      mult_count++;
      cout << "Dr ";
    }
    if (multipliers & FG_MACH) {
      mult_idx[mult_count] = FG_MACH;
      mult_count++;
      cout << "Mach ";
    }
    if (multipliers & FG_ALTITUDE) {
      mult_idx[mult_count] = FG_ALTITUDE;
      mult_count++;
      cout << "h ";
    }
    if (multipliers & FG_BI2VEL) {
      mult_idx[mult_count] = FG_BI2VEL;
      mult_count++;
      cout << "b /(2*Vt) ";
    }
    if (multipliers & FG_CI2VEL) {
      mult_idx[mult_count] = FG_CI2VEL;
      mult_count++;
      cout << "c /(2*Vt) ";
    }
    cout << endl;

    switch(type) {
    case VALUE:
      *AC_cfg >> StaticValue;
      cout << "      Value = " << StaticValue << endl;
      break;
    case VECTOR:
      Allocate(rows,2);

      for (r=1;r<=rows;r++) {
        *AC_cfg >> Table3D[r][0];
        *AC_cfg >> Table3D[r][1];
      }

      for (r=1;r<=rows;r++) {
        cout << "	";
        for (c=0;c<columns;c++) {
          cout << Table3D[r][c] << "	";
        }
        cout << endl;
      }

      break;
    case TABLE:
      Allocate(rows, columns);

      Table3D[0][0] = 0.0;
      for (c=1;c<=columns;c++) {
        *AC_cfg >> Table3D[0][c];
        for (r=1;r<=rows;r++) {
          if ( c==1 ) *AC_cfg >> Table3D[r][0];
          else *AC_cfg >> ftrashcan;
          *AC_cfg >> Table3D[r][c];
        }
      }

      for (r=0;r<=rows;r++) {
        cout << "	";
        for (c=0;c<=columns;c++) {
          cout << Table3D[r][c] << "	";
        }
        cout << endl;
      }

      break;
    }
    AC_cfg->GetNextConfigLine();
  }
}


FGCoefficient::~FGCoefficient(void)
{
}


bool FGCoefficient::Allocate(int r, int c)
{
  rows = r;
  columns = c;
  Table3D = new float*[r+1];
  for (int i=0;i<=r;i++) Table3D[i] = new float[c+1];
  return true;
}


bool FGCoefficient::Allocate(int n)
{
  rows = n;
  columns = 0;
  Table2D = new float[n+1];
  return true;
}


float FGCoefficient::Value(float rVal, float cVal)
{
  float rFactor, cFactor, col1temp, col2temp, Value;
  int r, c, midx;

  if (rows < 2 || columns < 2) return 0.0;

  for (r=1;r<=rows;r++) if (Table3D[r][0] >= rVal) break;
  for (c=1;c<=columns;c++) if (Table3D[0][c] >= cVal) break;

  c = c < 2 ? 2 : (c > columns ? columns : c);
  r = r < 2 ? 2 : (r > rows    ? rows    : r);

  rFactor = (rVal - Table3D[r-1][0]) / (Table3D[r][0] - Table3D[r-1][0]);
  cFactor = (cVal - Table3D[0][c-1]) / (Table3D[0][c] - Table3D[0][c-1]);

  col1temp = rFactor*(Table3D[r][c-1] - Table3D[r-1][c-1]) + Table3D[r-1][c-1];
  col2temp = rFactor*(Table3D[r][c] - Table3D[r-1][c]) + Table3D[r-1][c];

  SD = Value = col1temp + cFactor*(col2temp - col1temp);

  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

  return Value;
}


float FGCoefficient::Value(float Val)
{
  float Factor, Value;
  int r, midx;

  if (rows < 2) return 0.0;

  for (r=1;r<=rows;r++) if (Table3D[r][0] >= Val) break;
  r = r < 2 ? 2 : (r > rows    ? rows    : r);

  // make sure denominator below does not go to zero.
  if (Table3D[r][0] != Table3D[r-1][0]) {
    Factor = (Val - Table3D[r-1][0]) / (Table3D[r][0] - Table3D[r-1][0]);
  } else {
    Factor = 1.0;
  }

  SD = Value = Factor*(Table3D[r][1] - Table3D[r-1][1]) + Table3D[r-1][1];

  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

  return Value;
}


float FGCoefficient::Value(void)
{
	float Value;
	int midx;
	
	SD = Value = StaticValue;

  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

  return Value;
}

float FGCoefficient::TotalValue()
{
  switch(type) {
  case 0:
    return -1;
  case 1:
    return (Value());
  case 2:
    return (Value(GetCoeffVal(LookupR)));
  case 3:
    return (Value(GetCoeffVal(LookupR),GetCoeffVal(LookupC)));
  case 4:
    return 0.0;
  }
  return 0;
}

float FGCoefficient::GetCoeffVal(int val_idx)
{
  switch(val_idx) {
  case FG_QBAR:
    return State->Getqbar();
  case FG_WINGAREA:
    return Aircraft->GetWingArea();
  case FG_WINGSPAN:
    return Aircraft->GetWingSpan();
  case FG_CBAR:
    return Aircraft->Getcbar();
  case FG_ALPHA:
    return Translation->Getalpha();
  case FG_ALPHADOT:
    return State->Getadot();
  case FG_BETA:
    return Translation->Getbeta();
  case FG_BETADOT:
    return State->Getbdot();
  case FG_PITCHRATE:
    return Rotation->GetQ();
  case FG_ROLLRATE:
    return Rotation->GetP();
  case FG_YAWRATE:
    return Rotation->GetR();
  case FG_ELEVATOR:
    return FCS->GetDe();
  case FG_AILERON:
    return FCS->GetDa();
  case FG_RUDDER:
    return FCS->GetDr();
  case FG_MACH:
    return State->GetMach();
  case FG_ALTITUDE:
    return State->Geth();
  case FG_BI2VEL:
    return Aircraft->GetWingSpan()/(2.0 * State->GetVt());
  case FG_CI2VEL:
    return Aircraft->Getcbar()/(2.0 * State->GetVt());
  }
  return 0;
}


void FGCoefficient::DumpSD(void)
{
  cout << "   " << name << " = " << SD << endl;
}
