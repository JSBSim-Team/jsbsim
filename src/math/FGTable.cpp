/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGTable.cpp
 Author:       Jon S. Berndt
 Date started: 1/9/2001
 Purpose:      Models a lookup table

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
Models a lookup table

HISTORY
--------------------------------------------------------------------------------
JSB  1/9/00          Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <limits>
#include <assert.h>

#include "FGTable.h"
#include "input_output/FGXMLElement.h"
#include "input_output/string_utilities.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTable::FGTable(int NRows)
  : nRows(NRows), nCols(1)
{
  Type = tt1D;
  // Fill unused elements with NaNs to detect illegal access.
  Data.push_back(std::numeric_limits<double>::quiet_NaN());
  Data.push_back(std::numeric_limits<double>::quiet_NaN());
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(int NRows, int NCols)
  : nRows(NRows), nCols(NCols)
{
  Type = tt2D;
  // Fill unused elements with NaNs to detect illegal access.
  Data.push_back(std::numeric_limits<double>::quiet_NaN());
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(const FGTable& t)
  : PropertyManager(t.PropertyManager)
{
  Type = t.Type;
  nRows = t.nRows;
  nCols = t.nCols;
  internal = t.internal;
  Name = t.Name;
  lookupProperty[0] = t.lookupProperty[0];
  lookupProperty[1] = t.lookupProperty[1];
  lookupProperty[2] = t.lookupProperty[2];

  // Deep copy of t.Tables
  Tables.reserve(t.Tables.size());
  for(const auto &t: t.Tables)
    Tables.push_back(std::make_unique<FGTable>(*t));

  Data = t.Data;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned int FindNumColumns(const string& test_line)
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

FGTable::FGTable(std::shared_ptr<FGPropertyManager> pm, Element* el,
                 const std::string& Prefix)
  : PropertyManager(pm)
{
  string brkpt_string;
  Element *tableData = nullptr;

  // Is this an internal lookup table?

  Name = el->GetAttributeValue("name"); // Allow this table to be named with a property
  string call_type = el->GetAttributeValue("type");
  if (call_type == "internal") {
    internal = true;
  } else if (!call_type.empty()) {
    std::cerr << el->ReadFrom()
              <<"  An unknown table type attribute is listed: " << call_type
              << endl;
    throw BaseException("Unknown table type.");
  }

  // Determine and store the lookup properties for this table unless this table
  // is part of a 3D table, in which case its independentVar property indexes
  // will be set by a call from the owning table during creation

  unsigned int dimension = 0;

  Element* axisElement = el->FindElement("independentVar");
  if (axisElement) {

    // The 'internal' attribute of the table element cannot be specified
    // at the same time that independentVars are specified.
    if (internal) {
      cerr  << el->ReadFrom()
            << fgred << "  This table specifies both 'internal' call type" << endl
            << "  and specific lookup properties via the 'independentVar' element." << endl
            << "  These are mutually exclusive specifications. The 'internal'" << endl
            << "  attribute will be ignored." << fgdef << endl << endl;
      internal = false;
    }

    while (axisElement) {
      string property_string = axisElement->GetDataLine();
      if (property_string.find("#") != string::npos) {
        if (is_number(Prefix)) {
          property_string = replace(property_string,"#",Prefix);
        }
      }

      FGPropertyValue_ptr node = new FGPropertyValue(property_string,
                                                     PropertyManager, axisElement);
      string lookup_axis = axisElement->GetAttributeValue("lookup");
      if (lookup_axis == string("row")) {
        lookupProperty[eRow] = node;
        dimension = std::max(dimension, 1u);
      } else if (lookup_axis == string("column")) {
        lookupProperty[eColumn] = node;
        dimension = std::max(dimension, 2u);
      } else if (lookup_axis == string("table")) {
        lookupProperty[eTable] = node;
        dimension = std::max(dimension, 3u);
      } else if (!lookup_axis.empty()) {
        throw BaseException("Lookup table axis specification not understood: " + lookup_axis);
      } else { // assumed single dimension table; row lookup
        lookupProperty[eRow] = node;
        dimension = std::max(dimension, 1u);
      }
      axisElement = el->FindNextElement("independentVar");
    }

  } else if (internal) { // This table is an internal table

  // determine how many rows, columns, and tables in this table (dimension).

    if (el->GetNumElements("tableData") > 1) {
      dimension = 3; // this is a 3D table
    } else {
      tableData = el->FindElement("tableData");
      if (tableData) {
        unsigned int nLines = tableData->GetNumDataLines();
        unsigned int nColumns = FindNumColumns(tableData->GetDataLine(0));
        if (nLines > 1) {
          unsigned int nColumns1 = FindNumColumns(tableData->GetDataLine(1));
          if (nColumns1 == nColumns + 1) {
            dimension = 2;
            nColumns = nColumns1;
          }
          else
            dimension = 1;

          // Check that every line (but the header line) has the same number of
          // columns.
          for(unsigned int i=1; i<nLines; ++i) {
            if (FindNumColumns(tableData->GetDataLine(i)) != nColumns) {
              std::cerr << tableData->ReadFrom()
                        << "Invalid number of columns in line "
                        << tableData->GetLineNumber()+i << endl;
              throw BaseException("Invalid number of columns in table");
            }
          }
        }
        else
          dimension = 1;

        if (dimension == 1 && nColumns != 2) {
          std::cerr << tableData->ReadFrom()
                    << "Too many columns for a 1D table" << endl;
          throw BaseException("Too many columns for a 1D table");
        }
      }
    }

  } else {
    brkpt_string = el->GetAttributeValue("breakPoint");
    if (brkpt_string.empty()) {
      // no independentVars found, and table is not marked as internal, nor is it
      // a 3D table
      std::cerr << el->ReadFrom()
                << "No independentVars found, and table is not marked as internal,"
                << " nor is it a 3D table." << endl;
      throw BaseException("No independent variable found for table.");
    }
  }
  // end lookup property code

  if (brkpt_string.empty()) {                  // Not a 3D table "table element"
    // Force the dimension to 3 if there are several instances of <tableData>.
    // This is needed for sanity checks.
    if (el->GetNumElements("tableData") > 1) dimension = 3;
    tableData = el->FindElement("tableData");
  } else {                                     // This is a table in a 3D table
    tableData = el;
    dimension = 2;                             // Currently, infers 2D table
  }

  if (!tableData) {
    std::cerr << el->ReadFrom()
              << "FGTable: <tableData> elements are missing" << endl;
    throw BaseException("FGTable: <tableData> elements are missing");
  }
  else if (tableData->GetNumDataLines() == 0) {
    std::cerr << tableData->ReadFrom() << "<tableData> is empty." << endl;
    throw BaseException("<tableData> is empty.");
  }

  // Check that the lookup axes match the declared dimension of the table.
  if (!internal && brkpt_string.empty()) {
    switch (dimension) {
    case 3u:
      if (!lookupProperty[eTable]) {
        std::cerr << el->ReadFrom()
                  << "FGTable: missing lookup axis \"table\"";
        throw BaseException("FGTable: missing lookup axis \"table\"");
      }
      // Don't break as we want to investigate the other lookup axes as well.
    case 2u:
      if (!lookupProperty[eColumn]) {
        std::cerr << el->ReadFrom()
                  << "FGTable: missing lookup axis \"column\"";
        throw BaseException("FGTable: missing lookup axis \"column\"");
      }
      // Don't break as we want to investigate the last lookup axes as well.
    case 1u:
      if (!lookupProperty[eRow]) {
        std::cerr << el->ReadFrom()
                  << "FGTable: missing lookup axis \"row\"";
        throw BaseException("FGTable: missing lookup axis \"row\"");
      }
      break;
    default:
      assert(false); // Should never be called
      break;
    }
  }

  stringstream buf;

  for (unsigned int i=0; i<tableData->GetNumDataLines(); i++) {
    string line = tableData->GetDataLine(i);
    if (line.find_first_not_of("0123456789.-+eE \t\n") != string::npos) {
      cerr << " In file " << tableData->GetFileName() << endl
           << "   Illegal character found in line "
           << tableData->GetLineNumber() + i + 1 << ": " << endl << line << endl;
      throw BaseException("Illegal character");
    }
    buf << line << " ";
  }

  switch (dimension) {
  case 1:
    nRows = tableData->GetNumDataLines();
    nCols = 1;
    Type = tt1D;
    // Fill unused elements with NaNs to detect illegal access.
    Data.push_back(std::numeric_limits<double>::quiet_NaN());
    Data.push_back(std::numeric_limits<double>::quiet_NaN());
    *this << buf;
    break;
  case 2:
    nRows = tableData->GetNumDataLines()-1;
    nCols = FindNumColumns(tableData->GetDataLine(0));
    Type = tt2D;
    // Fill unused elements with NaNs to detect illegal access.
    Data.push_back(std::numeric_limits<double>::quiet_NaN());
    *this << buf;
    break;
  case 3:
    nRows = el->GetNumElements("tableData");
    nCols = 1;
    Type = tt3D;
    // Fill unused elements with NaNs to detect illegal access.
    Data.push_back(std::numeric_limits<double>::quiet_NaN());

    tableData = el->FindElement("tableData");
    while (tableData) {
      Tables.push_back(std::make_unique<FGTable>(PropertyManager, tableData));
      Data.push_back(tableData->GetAttributeValueAsNumber("breakPoint"));
      Tables.back()->lookupProperty[eRow] = lookupProperty[eRow];
      Tables.back()->lookupProperty[eColumn] = lookupProperty[eColumn];
      tableData = el->FindNextElement("tableData");
    }

    break;
  default:
    assert(false); // Should never be called
    break;
  }

  Debug(0);

  // Sanity checks: lookup indices must be increasing monotonically

  // find next xml element containing a name attribute
  // to indicate where the error occured
  Element* nameel = el;
  while (nameel != 0 && nameel->GetAttributeValue("name") == "")
    nameel=nameel->GetParent();

  // check breakpoints, if applicable
  if (Type == tt3D) {
    for (unsigned int b=2; b<=Tables.size(); ++b) {
      if (Data[b] <= Data[b-1]) {
        std::cerr << el->ReadFrom()
                  << fgred << highint
                  << "  FGTable: breakpoint lookup is not monotonically increasing" << endl
                  << "  in breakpoint " << b;
        if (nameel != 0) std::cerr << " of table in " << nameel->GetAttributeValue("name");
        std::cerr << ":" << reset << endl
                  << "  " << Data[b] << "<=" << Data[b-1] << endl;
        throw BaseException("Breakpoint lookup is not monotonically increasing");
      }
    }
  }

  // check columns, if applicable
  if (Type == tt2D) {
    for (unsigned int c=2; c<=nCols; ++c) {
      if (Data[c] <= Data[c-1]) {
        std::cerr << el->ReadFrom()
                  << fgred << highint
                  << "  FGTable: column lookup is not monotonically increasing" << endl
                  << "  in column " << c;
        if (nameel != 0) std::cerr << " of table in " << nameel->GetAttributeValue("name");
        std::cerr << ":" << reset << endl
                  << "  " << Data[c] << "<=" << Data[c-1] << endl;
        throw BaseException("FGTable: column lookup is not monotonically increasing");
      }
    }
  }

  // check rows
  if (Type != tt3D) { // in 3D tables, check only rows of subtables
    for (size_t r=2; r<=nRows; ++r) {
      if (Data[r*(nCols+1)]<=Data[(r-1)*(nCols+1)]) {
        std::cerr << el->ReadFrom()
                  << fgred << highint
                  << "  FGTable: row lookup is not monotonically increasing" << endl
                  << "  in row " << r;
        if (nameel != 0) std::cerr << " of table in " << nameel->GetAttributeValue("name");
        std::cerr << ":" << reset << endl
                  << "  " << Data[r*(nCols+1)] << "<=" << Data[(r-1)*(nCols+1)] << endl;
        throw BaseException("FGTable: row lookup is not monotonically increasing");
      }
    }
  }

  // Check the table has been entirely populated.
  switch (Type) {
  case tt1D:
    if (Data.size() != 2*nRows+2) missingData(el, 2*nRows, Data.size()-2);
    break;
  case tt2D:
    if (Data.size() != static_cast<size_t>(nRows+1)*(nCols+1))
      missingData(el, (nRows+1)*(nCols+1)-1, Data.size()-1);
    break;
  case tt3D:
    if (Data.size() != nRows+1) missingData(el, nRows, Data.size()-1);
    break;
  default:
    assert(false);  // Should never be called
    break;
  }

  bind(el, Prefix);

  if (debug_lvl & 1) Print();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTable::missingData(Element *el, unsigned int expected_size, size_t actual_size)
{
  std::cerr << el->ReadFrom()
            << fgred << highint
            << "  FGTable: Missing data";
  if (!Name.empty()) std::cerr << " in table " << Name;
  std::cerr << ":" << reset << endl
            << "  Expecting " << expected_size << " elements while "
            << actual_size << " elements were provided." << endl;
  throw BaseException("FGTable: missing data");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::~FGTable()
{
  // Untie the bound property so that it makes no further reference to this
  // instance of FGTable after the destruction is completed.
  if (!Name.empty() && !internal) {
    string tmp = PropertyManager->mkPropertyName(Name, false);
    SGPropertyNode* node = PropertyManager->GetNode(tmp);
    if (node && node->isTied())
      PropertyManager->Untie(node);
  }

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetElement(unsigned int r, unsigned int c) const
{
  assert(r <= nRows && c <= nCols);
  if (Type == tt3D) {
    assert(Data.size() == nRows+1);
    return Data[r];
  }
  assert(Data.size() == (nCols+1)*(nRows+1));
  return Data[r*(nCols+1)+c];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(void) const
{
  assert(!internal);

  switch (Type) {
  case tt1D:
    assert(lookupProperty[eRow]);
    return GetValue(lookupProperty[eRow]->getDoubleValue());
  case tt2D:
    assert(lookupProperty[eRow]);
    assert(lookupProperty[eColumn]);
    return GetValue(lookupProperty[eRow]->getDoubleValue(),
                    lookupProperty[eColumn]->getDoubleValue());
  case tt3D:
    assert(lookupProperty[eRow]);
    assert(lookupProperty[eColumn]);
    assert(lookupProperty[eTable]);
    return GetValue(lookupProperty[eRow]->getDoubleValue(),
                    lookupProperty[eColumn]->getDoubleValue(),
                    lookupProperty[eTable]->getDoubleValue());
  default:
    assert(false); // Should never be called
    return std::numeric_limits<double>::quiet_NaN();
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double key) const
{
  assert(nCols == 1);
  assert(Data.size() == 2*nRows+2);
  // If the key is off the end (or before the beginning) of the table, just
  // return the boundary-table value, do not extrapolate.
  if (key <= Data[2])
    return Data[3];
  else if (key >= Data[2*nRows])
    return Data[2*nRows+1];

  // Search for the right breakpoint.
  // This is a linear search, the algorithm is O(n).
  unsigned int r = 2;
  while (Data[2*r] < key) r++;

  double x0 = Data[2*r-2];
  double Span = Data[2*r] - x0;
  assert(Span > 0.0);
  double Factor = (key - x0) / Span;
  assert(Factor >= 0.0 && Factor <= 1.0);

  double y0 = Data[2*r-1];
  return Factor*(Data[2*r+1] - y0) + y0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double rowKey, double colKey) const
{
  if (nCols == 1) return GetValue(rowKey);

  assert(Type == tt2D);
  assert(Data.size() == (nCols+1)*(nRows+1));

  unsigned int c = 2;
  while(Data[c] < colKey && c < nCols) c++;
  double x0 = Data[c-1];
  double Span = Data[c] - x0;
  assert(Span > 0.0);
  double cFactor = Constrain(0.0, (colKey - x0) / Span, 1.0);

  if (nRows == 1) {
    double y0 = Data[(nCols+1)+c-1];
    return cFactor*(Data[(nCols+1)+c] - y0) + y0;
  }

  size_t r = 2;
  while(Data[r*(nCols+1)] < rowKey && r < nRows) r++;
  x0 = Data[(r-1)*(nCols+1)];
  Span = Data[r*(nCols+1)] - x0;
  assert(Span > 0.0);
  double rFactor = Constrain(0.0, (rowKey - x0) / Span, 1.0);
  double col1temp = rFactor*Data[r*(nCols+1)+c-1]+(1.0-rFactor)*Data[(r-1)*(nCols+1)+c-1];
  double col2temp = rFactor*Data[r*(nCols+1)+c]+(1.0-rFactor)*Data[(r-1)*(nCols+1)+c];

  return cFactor*(col2temp-col1temp)+col1temp;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double rowKey, double colKey, double tableKey) const
{
  assert(Type == tt3D);
  assert(Data.size() == nRows+1);
  // If the key is off the end (or before the beginning) of the table, just
  // return the boundary-table value, do not extrapolate.
  if(tableKey <= Data[1])
    return Tables[0]->GetValue(rowKey, colKey);
  else if (tableKey >= Data[nRows])
    return Tables[nRows-1]->GetValue(rowKey, colKey);

  // Search for the right breakpoint.
  // This is a linear search, the algorithm is O(n).
  unsigned int r = 2;
  while (Data[r] < tableKey) r++;

  double x0 = Data[r-1];
  double Span = Data[r] - x0;
  assert(Span > 0.0);
  double Factor = (tableKey - x0) / Span;
  assert(Factor >= 0.0 && Factor <= 1.0);

  double y0 = Tables[r-2]->GetValue(rowKey, colKey);
  return Factor*(Tables[r-1]->GetValue(rowKey, colKey) - y0) + y0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetMinValue(void) const
{
  assert(Type == tt1D);
  assert(Data.size() == 2*nRows+2);

  double minValue = HUGE_VAL;

  for(unsigned int i=1; i<=nRows; ++i)
    minValue = std::min(minValue, Data[2*i+1]);

  return minValue;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTable::operator<<(istream& in_stream)
{
  double x;
  assert(Type != tt3D);

  in_stream >> x;
  while(in_stream) {
    Data.push_back(x);
    in_stream >> x;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable& FGTable::operator<<(const double x)
{
  assert(Type != tt3D);
  Data.push_back(x);

  // Check column is monotically increasing
  size_t n = Data.size();
  if (Type == tt2D && nCols > 1 && n >= 3 && n <= nCols+1) {
    if (Data.at(n-1) <= Data.at(n-2))
      throw BaseException("FGTable: column lookup is not monotonically increasing");
  }

  // Check row is monotically increasing
  size_t row = (n-1) / (nCols+1);
  if (row >=2 && row*(nCols+1) == n-1) {
    if (Data.at(row*(nCols+1)) <= Data.at((row-1)*(nCols+1)))
      throw BaseException("FGTable: row lookup is not monotonically increasing");
  }

  return *this;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTable::Print(void)
{
  ios::fmtflags flags = cout.setf(ios::fixed); // set up output stream
  cout.precision(4);

  switch(Type) {
    case tt1D:
      cout << "    1 dimensional table with " << nRows << " rows." << endl;
      break;
    case tt2D:
      cout << "    2 dimensional table with " << nRows << " rows, " << nCols << " columns." << endl;
      break;
    case tt3D:
      cout << "    3 dimensional table with " << nRows << " breakpoints, "
                                          << Tables.size() << " tables." << endl;
      break;
  }
  unsigned int startCol=1, startRow=1;
  unsigned int p = 1;

  if (Type == tt1D) {
    startCol = 0;
    p = 2;
  }
  if (Type == tt2D) startRow = 0;

  for (unsigned int r=startRow; r<=nRows; r++) {
    cout << "\t";
    if (Type == tt2D) {
      if (r == startRow)
        cout << "\t";
      else
        startCol = 0;
    }

    for (unsigned int c=startCol; c<=nCols; c++) {
      cout << Data[p++] << "\t";
      if (Type == tt3D) {
        cout << endl;
        Tables[r-1]->Print();
      }
    }
    cout << endl;
  }
  cout.setf(flags); // reset
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGTable::bind(Element* el, const string& Prefix)
{
  if ( !Name.empty() && !internal) {
    if (!Prefix.empty()) {
      if (is_number(Prefix)) {
        if (Name.find("#") != string::npos) {
          Name = replace(Name, "#", Prefix);
        } else {
          cerr << el->ReadFrom()
                << "Malformed table name with number: " << Prefix
                << " and property name: " << Name
                << " but no \"#\" sign for substitution." << endl;
          throw BaseException("Missing \"#\" sign for substitution");
        }
      } else {
        Name = Prefix + "/" + Name;
      }
    }
    string tmp = PropertyManager->mkPropertyName(Name, false);

    if (PropertyManager->HasNode(tmp)) {
      SGPropertyNode* _property = PropertyManager->GetNode(tmp);
      if (_property->isTied()) {
        cerr << el->ReadFrom()
             << "Property " << tmp << " has already been successfully bound (late)." << endl;
        throw BaseException("Failed to bind the property to an existing already tied node.");
      }
    }

    PropertyManager->Tie<FGTable, double>(tmp, this, &FGTable::GetValue);
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
    if (from == 0) { } // Constructor
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
    }
  }
}
}
