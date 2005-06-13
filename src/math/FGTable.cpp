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

#if defined ( sgi ) && !defined( __GNUC__ ) && (_COMPILER_VERSION < 740)
# include <iomanip.h>
#else
# include <iomanip>
#endif

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGTable.cpp,v 1.2 2005/06/13 00:54:43 jberndt Exp $";
static const char *IdHdr = ID_TABLE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTable::FGTable(int NRows) : nRows(NRows), nCols(1), PropertyManager(0)
{
  Type = tt1D;
  colCounter = 0;
  rowCounter = 1;

  Data = Allocate();
  Debug(0);
  lastRowIndex=lastColumnIndex=2;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(const FGTable& t) : PropertyManager(t.PropertyManager)
{
  Type = t.Type;
  colCounter = t.colCounter;
  rowCounter = t.rowCounter;
  tableCounter = t.tableCounter;
  nRows = t.nRows;
  nCols = t.nCols;
  nTables = t.nTables;
  dimension = t.dimension;

  Tables = t.Tables;
  Data = Allocate();
  for (int r=0; r<=nRows; r++) {
    for (int c=0; c<=nCols; c++) {
      Data[r][c] = t.Data[r][c];
    }
  }
  lastRowIndex = t.lastRowIndex;
  lastColumnIndex = t.lastColumnIndex;
  lastTableIndex = t.lastTableIndex;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(FGPropertyManager* propMan, Element* el) : PropertyManager(propMan)
{
  stringstream buf;
  string property_string;
  FGPropertyManager* node;
  dimension = 0;
  Element *tableData;
  Element *axisElement;

  // Determine and store the lookup properties for this table unless this table
  // is part of a 3D table, in which case its independentVar property indexes will
  // be set by a call from the owning table during creation

  tableData = el->FindElement("tableData");
  axisElement = el->FindElement("independentVar");

  if (!tableData) { // this is a tableData dataset from a 3D table
    tableData = el;
    dimension = 2;
  } else {
    if (!axisElement) {
      cerr << "No independent variable found for table." << endl;
      return;
    }

    for (int i=0; i<3; i++) lookupProperty[i] = 0;
    while (axisElement) {
      property_string = axisElement->GetDataLine();
      node = PropertyManager->GetNode(property_string);
      if (axisElement->GetAttributeValue("lookup") == string("row")) {
        lookupProperty[eRow] = node;
      } else if (axisElement->GetAttributeValue("lookup") == string("column")) {
        lookupProperty[eColumn] = node;
      } else if (axisElement->GetAttributeValue("lookup") == string("table")) {
        lookupProperty[eTable] = node;
      } else { // assumed single dimension table; row lookup
        lookupProperty[eRow] = node;
      }
      axisElement = el->FindNextElement("independentVar");
    }
    for (int i=0; i<3; i++) if (lookupProperty[i] != 0) dimension++;
  }
  // end lookup property code

  for (int i=0; i<tableData->GetNumDataLines(); i++) {
    buf << tableData->GetDataLine(i) << string(" ");
  }
  switch (dimension) {
  case 1:
    nRows = tableData->GetNumDataLines();
    nCols = 1;
    Type = tt1D;
    colCounter = 0;
    rowCounter = 1;
    Data = Allocate();
    Debug(0);
    lastRowIndex = lastColumnIndex = 2;
    *this << buf;
    break;
  case 2:
    nRows = tableData->GetNumDataLines()-1;
    if (nRows >= 2) {
      nCols = 0;
      // determine number of data columns in table (first column is row lookup - don't count)
      int position=0;
      while ((position = tableData->GetDataLine(0).find_first_not_of(" \t", position)) != string::npos) {
        nCols++;
        position = tableData->GetDataLine(0).find_first_of(" \t", position);
      }
    } else {
      cerr << "Not enough rows" << endl;
    }
    if (nCols > 1) {
      Type = tt2D;
      colCounter = 1;
      rowCounter = 0;
    } else if (nCols == 1) {
      Type = tt1D;
      colCounter = 0;
      rowCounter = 1;
    } else {
      cerr << "FGTable cannot accept 'Rows=0'" << endl;
    }

    Data = Allocate();
    lastRowIndex = lastColumnIndex = 2;
    *this << buf;
    break;
  case 3: // not done, yet
    nTables=0;
    tableData = el->FindElement("tableData");
    while (tableData) {
      nTables++;
      tableData = el->FindNextElement("tableData");
    }
    nRows = nTables;
    nCols = 1;
    Type = tt3D;
    colCounter = 1;
    rowCounter = 1;

    Data = Allocate(); // this data array will contain the keys for the associated tables
    Tables.reserve(nTables); // necessary?
    tableData = el->FindElement("tableData");
    for (int i=0; i<nTables; i++) {
      Tables.push_back(new FGTable(PropertyManager, tableData));
      Data[i+1][1] = tableData->GetAttributeValueAsNumber("breakPoint");
      Tables[i]->SetRowIndexProperty(lookupProperty[eRow]);
      Tables[i]->SetColumnIndexProperty(lookupProperty[eColumn]);
      tableData = el->FindNextElement("tableData");
    }

    Debug(0);
    break;
  default:
    cout << "No dimension given" << endl;
    break;
  }
  if (debug_lvl & 1) Print();
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
  if (nTables > 0) {
    for (int i=0; i<nTables; i++) delete Tables[i];
    Tables.clear();
  }
  for (int r=0; r<=nRows; r++) if (Data[r]) delete[] Data[r];
  if (Data) delete[] Data;
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(void) const
{
  double temp = 0;
  double temp2 = 0;

  switch (Type) {
  case tt1D:
    temp = lookupProperty[eRow]->getDoubleValue();
    temp2 = GetValue(temp);
    return temp2;
  case tt2D:
    return GetValue(lookupProperty[eRow]->getDoubleValue(),
                    lookupProperty[eColumn]->getDoubleValue());
  case tt3D:
    return GetValue(lookupProperty[eRow]->getDoubleValue(),
                    lookupProperty[eColumn]->getDoubleValue(),
                    lookupProperty[eTable]->getDoubleValue());
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double key) const
{
  double Factor, Value, Span;
  int r=lastRowIndex;

  //if the key is off the end of the table, just return the
  //end-of-table value, do not extrapolate
  if( key <= Data[1][0] ) {
    lastRowIndex=2;
    //cout << "Key underneath table: " << key << endl;
    return Data[1][1];
  } else if ( key >= Data[nRows][0] ) {
    lastRowIndex=nRows;
    //cout << "Key over table: " << key << endl;
    return Data[nRows][1];
  }

  // the key is somewhere in the middle, search for the right breakpoint
  // assume the correct breakpoint has not changed since last frame or
  // has only changed very little

  if ( r > 2 && Data[r-1][0] > key ) {
    while( Data[r-1][0] > key && r > 2) { r--; }
  } else if ( Data[r][0] < key ) {
    while( Data[r][0] <= key && r <= nRows) { r++; }
  }

  lastRowIndex=r;
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


double FGTable::GetValue(double rowKey, double colKey) const
{
  double rFactor, cFactor, col1temp, col2temp, Value;
  int r=lastRowIndex;
  int c=lastColumnIndex;

  if ( r > 2 && Data[r-1][0] > rowKey ) {
    while ( Data[r-1][0] > rowKey && r > 2) { r--; }
  } else if ( Data[r][0] < rowKey ) {
//    cout << Data[r][0] << endl;
    while ( r <= nRows && Data[r][0] <= rowKey ) { r++; }
    if ( r > nRows ) r = nRows;
  }

  if ( c > 2 && Data[0][c-1] > colKey ) {
    while( Data[0][c-1] > colKey && c > 2) { c--; }
  } else if ( Data[0][c] < colKey ) {
    while( Data[0][c] <= colKey && c <= nCols) { c++; }
    if ( c > nCols ) c = nCols;
  }

  lastRowIndex=r;
  lastColumnIndex=c;

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

double FGTable::GetValue(double rowKey, double colKey, double tableKey) const
{
  double Factor, Value, Span;
  int r=lastRowIndex;

  //if the key is off the end  (or before the beginning) of the table,
  // just return the boundary-table value, do not extrapolate

  if( tableKey <= Data[1][1] ) {
    lastRowIndex=2;
    return Tables[0]->GetValue(rowKey, colKey);
  } else if ( tableKey >= Data[nRows][1] ) {
    lastRowIndex=nRows;
    return Tables[nRows-1]->GetValue(rowKey, colKey);
  }

  // the key is somewhere in the middle, search for the right breakpoint
  // assume the correct breakpoint has not changed since last frame or
  // has only changed very little

  if ( r > 2 && Data[r-1][1] > tableKey ) {
    while( Data[r-1][1] > tableKey && r > 2) { r--; }
  } else if ( Data[r][1] < tableKey ) {
    while( Data[r][1] <= tableKey && r <= nRows) { r++; }
  }

  lastRowIndex=r;
  // make sure denominator below does not go to zero.

  Span = Data[r][1] - Data[r-1][1];
  if (Span != 0.0) {
    Factor = (tableKey - Data[r-1][1]) / Span;
    if (Factor > 1.0) Factor = 1.0;
  } else {
    Factor = 1.0;
  }

  Value = Factor*(Tables[r-1]->GetValue(rowKey, colKey) - Tables[r-2]->GetValue(rowKey, colKey))
                              + Tables[r-2]->GetValue(rowKey, colKey);

  return Value;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTable::operator<<(stringstream& in_stream)
{
  int startRow=0;
  int startCol=0;
  int tableCtr=0;

  if (Type == tt1D || Type == tt3D) startRow = 1;
  if (Type == tt3D) startCol = 1;

  for (int r=startRow; r<=nRows; r++) {
    for (int c=startCol; c<=nCols; c++) {
      if (r != 0 || c != 0) {
        in_stream >> Data[r][c];
//        if (Type == tt3D) {
//          *Tables[tableCtr] << in_stream;
//          tableCtr++;
//        }
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
  int startRow=0;
  int startCol=0;

  if (Type == tt1D || Type == tt3D) startRow = 1;
  if (Type == tt3D) startCol = 1;

#if defined (sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
  unsigned long flags = cout.setf(ios::fixed);
#else
  ios::fmtflags flags = cout.setf(ios::fixed); // set up output stream
#endif

  switch(Type) {
    case tt1D:
      cout << "    1 dimensional table with " << nRows << " rows." << endl;
      break;
    case tt2D:
      cout << "    2 dimensional table with " << nRows << " rows, " << nCols << " columns." << endl;
      break;
    case tt3D:
      cout << "    3 dimensional table with " << nRows << " rows, "
                                          << nCols << " columns "
                                          << nTables << " tables." << endl;
      break;
  }
  cout.precision(4);
  for (int r=startRow; r<=nRows; r++) {
    cout << "	";
    for (int c=startCol; c<=nCols; c++) {
      if (r == 0 && c == 0) {
        cout << "	";
      } else {
        cout << Data[r][c] << "	";
        if (Type == tt3D) {
          cout << endl;
          Tables[r-1]->Print();
        }
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
}
