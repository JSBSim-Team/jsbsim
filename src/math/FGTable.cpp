/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTable.cpp
 Author:       Jon S. Berndt
 Date started: 1/9/2001
 Purpose:      Models a lookup table

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

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
Models a lookup table

HISTORY
--------------------------------------------------------------------------------
JSB  1/9/00          Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGTable.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGPropertyManager.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGTable.cpp,v 1.28 2011/06/13 12:07:10 jberndt Exp $";
static const char *IdHdr = ID_TABLE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTable::FGTable(int NRows) : nRows(NRows), nCols(1), PropertyManager(0)
{
  Type = tt1D;
  colCounter = 0;
  rowCounter = 1;
  nTables = 0;

  Data = Allocate();
  Debug(0);
  lastRowIndex=lastColumnIndex=2;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(int NRows, int NCols) : nRows(NRows), nCols(NCols), PropertyManager(0)
{
  Type = tt2D;
  colCounter = 1;
  rowCounter = 0;
  nTables = 0;

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
  internal = t.internal;
  Name = t.Name;
  lookupProperty[0] = t.lookupProperty[0];
  lookupProperty[1] = t.lookupProperty[1];
  lookupProperty[2] = t.lookupProperty[2];

  Tables = t.Tables;
  Data = Allocate();
  for (unsigned int r=0; r<=nRows; r++) {
    for (unsigned int c=0; c<=nCols; c++) {
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
  unsigned int i;

  stringstream buf;
  string property_string;
  string lookup_axis;
  string call_type;
  string parent_type;
  string brkpt_string;
  FGPropertyManager* node;
  Element *tableData=0;
  Element *parent_element=0;
  Element *axisElement=0;
  string operation_types = "function, product, sum, difference, quotient,"
                           "pow, abs, sin, cos, asin, acos, tan, atan, table";

  nTables = 0;

  // Is this an internal lookup table?

  internal = false;
  Name = el->GetAttributeValue("name"); // Allow this table to be named with a property
  call_type = el->GetAttributeValue("type");
  if (call_type == string("internal")) {
    parent_element = el->GetParent();
    parent_type = parent_element->GetName();
    if (operation_types.find(parent_type) == string::npos) {
      internal = true;
    } else {
      // internal table is a child element of a restricted type
      throw("  An internal table cannot be nested within another type,"
            " such as a function. The 'internal' keyword is ignored.");
    }
  } else if (!call_type.empty()) {
    throw("  An unknown table type attribute is listed: "  
          ". Execution cannot continue.");
  }

  // Determine and store the lookup properties for this table unless this table
  // is part of a 3D table, in which case its independentVar property indexes will
  // be set by a call from the owning table during creation

  dimension = 0;

  axisElement = el->FindElement("independentVar");
  if (axisElement) {

    // The 'internal' attribute of the table element cannot be specified
    // at the same time that independentVars are specified.
    if (internal) {
      cerr << endl << fgred << "  This table specifies both 'internal' call type" << endl;
      cerr << "  and specific lookup properties via the 'independentVar' element." << endl;
      cerr << "  These are mutually exclusive specifications. The 'internal'" << endl;
      cerr << "  attribute will be ignored." << fgdef << endl << endl;
      internal = false;
    }

    for (i=0; i<3; i++) lookupProperty[i] = 0;

    while (axisElement) {
      property_string = axisElement->GetDataLine();
      // The property string passed into GetNode() must have no spaces or tabs.
      node = PropertyManager->GetNode(property_string);

      if (node == 0) {
        throw("IndependentVar property, " + property_string + " in Table definition is not defined.");
      }

      lookup_axis = axisElement->GetAttributeValue("lookup");
      if (lookup_axis == string("row")) {
        lookupProperty[eRow] = node;
      } else if (lookup_axis == string("column")) {
        lookupProperty[eColumn] = node;
      } else if (lookup_axis == string("table")) {
        lookupProperty[eTable] = node;
      } else if (!lookup_axis.empty()) {
        throw("Lookup table axis specification not understood: " + lookup_axis);
      } else { // assumed single dimension table; row lookup
        lookupProperty[eRow] = node;
      }
      dimension++;
      axisElement = el->FindNextElement("independentVar");
    }

  } else if (internal) { // This table is an internal table

  // determine how many rows, columns, and tables in this table (dimension).

    if (el->GetNumElements("tableData") > 1) {
      dimension = 3; // this is a 3D table
    } else {
      tableData = el->FindElement("tableData");
      string test_line = tableData->GetDataLine(1);  // examine second line in table for dimension
      if (FindNumColumns(test_line) == 2) dimension = 1;    // 1D table
      else if (FindNumColumns(test_line) > 2) dimension = 2; // 2D table
      else {
        cerr << "Invalid number of columns in table" << endl;
      }
    }

  } else {
    brkpt_string = el->GetAttributeValue("breakPoint");
    if (brkpt_string.empty()) {
     // no independentVars found, and table is not marked as internal, nor is it a 3D table
      throw("No independent variable found for table.");
    }
  }
  // end lookup property code

  if (brkpt_string.empty()) {                  // Not a 3D table "table element"
    tableData = el->FindElement("tableData");
  } else {                                     // This is a table in a 3D table
    tableData = el;
    dimension = 2;                             // Currently, infers 2D table
  }

  for (i=0; i<tableData->GetNumDataLines(); i++) {
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
      nCols = FindNumColumns(tableData->GetDataLine(0));
      if (nCols < 2) throw(string("Not enough columns in table data."));
    } else {
      throw(string("Not enough rows in the table data."));
    }

    Type = tt2D;
    colCounter = 1;
    rowCounter = 0;

    Data = Allocate();
    lastRowIndex = lastColumnIndex = 2;
    *this << buf;
    break;
  case 3:
    nTables = el->GetNumElements("tableData");
    nRows = nTables;
    nCols = 1;
    Type = tt3D;
    colCounter = 1;
    rowCounter = 1;
    lastRowIndex = lastColumnIndex = 2;

    Data = Allocate(); // this data array will contain the keys for the associated tables
    Tables.reserve(nTables); // necessary?
    tableData = el->FindElement("tableData");
    for (i=0; i<nTables; i++) {
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

  // Sanity checks: lookup indices must be increasing monotonically
  unsigned int r,c,b;

  // find next xml element containing a name attribute
  // to indicate where the error occured
  Element* nameel = el;
  while (nameel != 0 && nameel->GetAttributeValue("name") == "")
    nameel=nameel->GetParent();

  // check breakpoints, if applicable
  if (dimension > 2) {
    for (b=2; b<=nTables; ++b) {
      if (Data[b][1] <= Data[b-1][1]) {
        stringstream errormsg;
        errormsg << fgred << highint << endl
             << "  FGTable: breakpoint lookup is not monotonically increasing" << endl
             << "  in breakpoint " << b;
        if (nameel != 0) errormsg << " of table in " << nameel->GetAttributeValue("name");
        errormsg << ":" << reset << endl
                 << "  " << Data[b][1] << "<=" << Data[b-1][1] << endl;
        throw(errormsg.str());
      }
    }
  }

  // check columns, if applicable
  if (dimension > 1) {
    for (c=2; c<=nCols; ++c) {
      if (Data[0][c] <= Data[0][c-1]) {
        stringstream errormsg;
        errormsg << fgred << highint << endl
             << "  FGTable: column lookup is not monotonically increasing" << endl
             << "  in column " << c;
        if (nameel != 0) errormsg << " of table in " << nameel->GetAttributeValue("name");
        errormsg << ":" << reset << endl
                 << "  " << Data[0][c] << "<=" << Data[0][c-1] << endl;
        throw(errormsg.str());
      }
    }
  }

  // check rows
  if (dimension < 3) { // in 3D tables, check only rows of subtables
    for (r=2; r<=nRows; ++r) {
      if (Data[r][0]<=Data[r-1][0]) {
        stringstream errormsg;
        errormsg << fgred << highint << endl
             << "  FGTable: row lookup is not monotonically increasing" << endl
             << "  in row " << r;
        if (nameel != 0) errormsg << " of table in " << nameel->GetAttributeValue("name");
        errormsg << ":" << reset << endl
                 << "  " << Data[r][0] << "<=" << Data[r-1][0] << endl;
        throw(errormsg.str());
      }
    }
  }

  bind();

  if (debug_lvl & 1) Print();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double** FGTable::Allocate(void)
{
  Data = new double*[nRows+1];
  for (unsigned int r=0; r<=nRows; r++) {
    Data[r] = new double[nCols+1];
    for (unsigned int c=0; c<=nCols; c++) {
      Data[r][c] = 0.0;
    }
  }
  return Data;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::~FGTable()
{
  if (nTables > 0) {
    for (unsigned int i=0; i<nTables; i++) delete Tables[i];
    Tables.clear();
  }
  for (unsigned int r=0; r<=nRows; r++) delete[] Data[r];
  delete[] Data;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned int FGTable::FindNumColumns(const string& test_line)
{
  // determine number of data columns in table (first column is row lookup - don't count)
  size_t position=0;
  unsigned int nCols=0;
  while ((position = test_line.find_first_not_of(" \t", position)) != string::npos) {
    nCols++;
    position = test_line.find_first_of(" \t", position);
  }
  return nCols;
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
  default:
    cerr << "Attempted to GetValue() for invalid/unknown table type" << endl;
    throw(string("Attempted to GetValue() for invalid/unknown table type"));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double key) const
{
  double Factor, Value, Span;
  unsigned int r = lastRowIndex;

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
  // The search is particularly efficient if 
  // the correct breakpoint has not changed since last frame or
  // has only changed very little

  while (r > 2     && Data[r-1][0] > key) { r--; }
  while (r < nRows && Data[r][0]   < key) { r++; }

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
  unsigned int r = lastRowIndex;
  unsigned int c = lastColumnIndex;

  while(r > 2     && Data[r-1][0] > rowKey) { r--; }
  while(r < nRows && Data[r]  [0] < rowKey) { r++; }

  while(c > 2     && Data[0][c-1] > colKey) { c--; }
  while(c < nCols && Data[0][c]   < colKey) { c++; }

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
  unsigned int r = lastRowIndex;

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
  // The search is particularly efficient if 
  // the correct breakpoint has not changed since last frame or
  // has only changed very little

  while(r > 2     && Data[r-1][1] > tableKey) { r--; }
  while(r < nRows && Data[r]  [1] < tableKey) { r++; }

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

void FGTable::operator<<(istream& in_stream)
{
  int startRow=0;
  int startCol=0;

// In 1D table, no pseudo-row of column-headers (i.e. keys):
  if (Type == tt1D) startRow = 1;

  for (unsigned int r=startRow; r<=nRows; r++) {
    for (unsigned int c=startCol; c<=nCols; c++) {
      if (r != 0 || c != 0) {
        in_stream >> Data[r][c];
      }
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Put some error handling in here if trying to access out of range row, col.

FGTable& FGTable::operator<<(const double n)
{
  Data[rowCounter][colCounter] = n;
  if (colCounter == (int)nCols) {
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
  for (unsigned int r=startRow; r<=nRows; r++) {
    cout << "	";
    for (unsigned int c=startCol; c<=nCols; c++) {
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

void FGTable::bind(void)
{
  typedef double (FGTable::*PMF)(void) const;
  if ( !Name.empty() && !internal) {
    string tmp = PropertyManager->mkPropertyName(Name, false); // Allow upper
    PropertyManager->Tie( tmp, this, (PMF)&FGTable::GetValue);
  }
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
