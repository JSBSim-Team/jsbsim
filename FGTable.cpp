/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTable.cpp
 Author:       Jon S. Berndt
 Date started: 1/9/2001
 Purpose:      Models a lookup table

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

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
Models a lookup table

HISTORY
--------------------------------------------------------------------------------
JSB  1/9/00          Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGTable.h"

#if defined ( sgi ) && !defined( __GNUC__ )
#include <iomanip.h>
#else
#include <iomanip>
#endif

static const char *IdSrc = "$Id: FGTable.cpp,v 1.24 2002/01/31 21:17:11 jberndt Exp $";
static const char *IdHdr = ID_TABLE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

using namespace std;

FGTable::FGTable(int NRows, int NCols) : nRows(NRows), nCols(NCols)
{
  if (NCols > 1) {
    Type = tt2D;
    colCounter = 1;
    rowCounter = 0;
  } else if (NCols == 1) {
    Type = tt1D;
    colCounter = 0;
    rowCounter = 1;
  } else {
    cerr << "FGTable cannot accept 'Rows=0'" << endl;
  }

  Data = Allocate();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(int NRows) : nRows(NRows), nCols(1)
{
  Type = tt1D;
  colCounter = 0;
  rowCounter = 1;

  Data = Allocate();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double** FGTable::Allocate(void)
{
  Data = new double*[nRows+1];
  for (int r=0; r<=nRows; r++) {
    Data[r] = new double[nCols+1];
    for (int c=0; c<=nCols; c++) {
      Data[r][c] = 0.0;
    }
  }
  return Data;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::~FGTable()
{
  for (int r=0; r<=nRows; r++) if (Data[r]) delete[] Data[r];
  if (Data) delete[] Data;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double key)
{
  double Factor, Value, Span;
  int r;

  for (r=1; r<=nRows; r++) if (Data[r][0] >= key) break;
  r = r < 2 ? 2 : (r > nRows ? nRows : r);

  // make sure denominator below does not go to zero.

  Span = Data[r][0] - Data[r-1][0];
  if (Span != 0.0) {
    Factor = (key - Data[r-1][0]) / Span;
    if (Factor > 1.0) Factor = 1.0;
  } else {
    Factor = 1.0;
  }

  Value = Factor*(Data[r][1] - Data[r-1][1]) + Data[r-1][1];

  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double rowKey, double colKey)
{
  double rFactor, cFactor, col1temp, col2temp, Value;
  int r, c;

  for (r=1;r<=nRows;r++) if (Data[r][0] >= rowKey) break;
  for (c=1;c<=nCols;c++) if (Data[0][c] >= colKey) break;

  c = c < 2 ? 2 : (c > nCols ? nCols : c);
  r = r < 2 ? 2 : (r > nRows ? nRows : r);

  rFactor = (rowKey - Data[r-1][0]) / (Data[r][0] - Data[r-1][0]);
  cFactor = (colKey - Data[0][c-1]) / (Data[0][c] - Data[0][c-1]);

  if (rFactor > 1.0) rFactor = 1.0;
  else if (rFactor < 0.0) rFactor = 0.0;

  if (cFactor > 1.0) cFactor = 1.0;
  else if (cFactor < 0.0) cFactor = 0.0;

  col1temp = rFactor*(Data[r][c-1] - Data[r-1][c-1]) + Data[r-1][c-1];
  col2temp = rFactor*(Data[r][c] - Data[r-1][c]) + Data[r-1][c];

  Value = col1temp + cFactor*(col2temp - col1temp);

  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTable::operator<<(FGConfigFile& infile)
{
  int startRow;

  if (Type == tt1D) startRow = 1;
  else startRow = 0;

  for (int r=startRow; r<=nRows; r++) {
    for (int c=0; c<=nCols; c++) {
      if (r != 0 || c != 0) {
        infile >> Data[r][c];
      }
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable& FGTable::operator<<(const double n)
{
  Data[rowCounter][colCounter] = n;
  if (colCounter == nCols) {
    colCounter = 0;
    rowCounter++;
  } else {
    colCounter++;
  }
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable& FGTable::operator<<(const int n)
{
  *this << (double)n;
  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTable::Print(void)
{
  int startRow;

  if (Type == tt1D) startRow = 1;
  else startRow = 0;

#if defined (sgi) && !defined(__GNUC__)
  unsigned long flags = cout.setf(ios::fixed);
#else
  ios::fmtflags flags = cout.setf(ios::fixed); // set up output stream
#endif

  cout.precision(4);

  for (int r=startRow; r<=nRows; r++) {
    cout << "	";
    for (int c=0; c<=nCols; c++) {
      if (r == 0 && c == 0) {
      cout << "	";
      } else {
      cout << Data[r][c] << "	";
      }
    }
    cout << endl;
  }
  cout.setf(flags); // reset
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

void FGTable::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGTable" << endl;
    if (from == 1) cout << "Destroyed:    FGTable" << endl;
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



