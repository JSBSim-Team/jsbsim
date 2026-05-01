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
ADM  2026/04/17      Added support for 4D and higher tables.

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

namespace { // anonymous namespace for helper functions

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

unsigned int InferLeafDimension(Element* tableData)
{
  const unsigned int nLines = tableData->GetNumDataLines();
  if (nLines == 0u) {
    XMLLogException err(tableData);
    err << "<tableData> is empty.\n";
    throw err;
  }

  const unsigned int firstLineColumns = FindNumColumns(tableData->GetDataLine(0));

  if (nLines == 1u) {
    if (firstLineColumns != 2u) {
      XMLLogException err(tableData);
      err << "Too many columns for a 1D table\n";
      throw err;
    }
    return 1u;
  }

  const unsigned int secondLineColumns = FindNumColumns(tableData->GetDataLine(1));
  if (secondLineColumns == firstLineColumns + 1u) {
    for(unsigned int i=1; i<nLines; ++i) {
      if (FindNumColumns(tableData->GetDataLine(i)) != secondLineColumns) {
        XMLLogException err(tableData);
        err << "Invalid number of columns in line "
            << tableData->GetLineNumber()+i << "\n";
        throw err;
      }
    }
    return 2u;
  }

  if (firstLineColumns != 2u) {
    XMLLogException err(tableData);
    err << "Too many columns for a 1D table\n";
    throw err;
  }

  for(unsigned int i=1; i<nLines; ++i) {
    if (FindNumColumns(tableData->GetDataLine(i)) != 2u) {
      XMLLogException err(tableData);
      err << "Invalid number of columns in line "
          << tableData->GetLineNumber()+i << "\n";
      throw err;
    }
  }

  return 1u;
}

unsigned int ParseLookupAxis(const string& lookup_axis)
{
  if (lookup_axis.empty() || lookup_axis == "row" || lookup_axis == "axis1")
    return 0u;
  if (lookup_axis == "column" || lookup_axis == "axis2")
    return 1u;
  if (lookup_axis == "table" || lookup_axis == "axis3")
    return 2u;

  if (lookup_axis.rfind("axis", 0) == 0) {
    const string suffix = lookup_axis.substr(4);
    if (!suffix.empty() &&
        suffix.find_first_not_of("0123456789") == string::npos) {
      const unsigned long axis = std::stoul(suffix);
      if (axis >= 1ul)
        return static_cast<unsigned int>(axis - 1ul);
    }
  }

  throw BaseException("Lookup table axis specification not understood: " + lookup_axis);
}

string LookupAxisName(unsigned int axis)
{
  if (axis == 0u)
    return "row";
  if (axis == 1u)
    return "column";
  if (axis == 2u)
    return "table";
  return "axis" + std::to_string(axis + 1u);
}

void AppendNumericData(Element* tableData, stringstream& buf)
{
  for (unsigned int i=0; i<tableData->GetNumDataLines(); ++i) {
    const string line = tableData->GetDataLine(i);
    if (line.find_first_not_of("0123456789.-+eE \t\n") != string::npos) {
      XMLLogException err(tableData);
      err << "   Illegal character found in line "
          << tableData->GetLineNumber() + i + 1 << ": \n" << line << "\n";
      throw err;
    }
    buf << line << " ";
  }
}

} // anonymous namespace

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGTable::FGTable(int NRows)
  : nRows(NRows), nCols(1u), nDims(1u)
{
  Type = tt1D;
  // Fill unused elements with NaNs to detect illegal access.
  Data.push_back(std::numeric_limits<double>::quiet_NaN());
  Data.push_back(std::numeric_limits<double>::quiet_NaN());
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(int NRows, int NCols)
  : nRows(NRows), nCols(NCols), nDims(2u)
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
  nDims = t.nDims;
  internal = t.internal;
  Name = t.Name;
  lookupProperty = t.lookupProperty;

  // Deep copy of t.Tables
  Tables.reserve(t.Tables.size());
  for(const auto& table: t.Tables)
    Tables.push_back(std::make_unique<FGTable>(*table));

  Data = t.Data;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable::FGTable(std::shared_ptr<FGPropertyManager> pm, Element* el,
                 const std::string& Prefix)
  : PropertyManager(pm)
{
  // Is this an internal lookup table?

  Name = el->GetAttributeValue("name"); // Allow this table to be named with a property
  const string call_type = el->GetAttributeValue("type");
  if (call_type == "internal") {
    internal = true;
  } else if (!call_type.empty()) {
    XMLLogException err(el);
    err << "  An unknown table type attribute is listed: " << call_type << "\n";
    throw err;
  }

  // Determine and store the lookup properties for this table unless this table
  // is part of a higher-dimensional table, in which case its independentVar
  // property indexes will be set by a call from the owning table during creation

  unsigned int declared_dimension = 0u;

  Element* axisElement = el->FindElement("independentVar");
  if (axisElement) {

    // The 'internal' attribute of the table element cannot be specified
    // at the same time that independentVars are specified.
    if (internal) {
      FGXMLLogging log(el, LogLevel::ERROR);
      log << LogFormat::RED << "  This table specifies both 'internal' call type\n"
          << "  and specific lookup properties via the 'independentVar' element.\n"
          << "  These are mutually exclusive specifications. The 'internal'\n"
          << "  attribute will be ignored.\n" << LogFormat::DEFAULT;
      internal = false;
    }

    while (axisElement) {
      string property_string = axisElement->GetDataLine();
      if (property_string.find("#") != string::npos && is_number(Prefix))
        property_string = replace(property_string, "#", Prefix);

      FGPropertyValue_ptr node = new FGPropertyValue(property_string,
                                                     PropertyManager, axisElement);

      const unsigned int axis = ParseLookupAxis(axisElement->GetAttributeValue("lookup"));
      if (HasLookupProperty(axis)) {
        XMLLogException err(axisElement);
        err << "FGTable: duplicate lookup axis \"" << LookupAxisName(axis) << "\"\n";
        throw err;
      }

      SetLookupProperty(axis, node);
      declared_dimension = std::max(declared_dimension, axis + 1u);
      axisElement = el->FindNextElement("independentVar");
    }

    // Check that the lookup axes match the declared dimension of the table.
    for (unsigned int axis=0u; axis<declared_dimension; ++axis) {
      if (!HasLookupProperty(axis)) {
        XMLLogException err(el);
        err << "FGTable: missing lookup axis \"" << LookupAxisName(axis) << "\"\n";
        throw err;
      }
    }

  } else if (!internal && el->GetName() != "tableData") {
    // no independentVars found, and table is not marked as internal, nor is it
    // a nested sub-table
    XMLLogException err(el);
    err << "No independentVars found, and table is not marked as internal,"
        << " nor is it a nested sub-table.\n";
    throw err;
  }
  // end lookup property code

  Element* leafData = nullptr;

  const unsigned int nChildTableData = el->GetNumElements("tableData");

  if (el->GetName() == "tableData" && nChildTableData == 0u) {
    // This is a leaf <tableData> element with numeric content
    leafData = el;
  } else if (el->GetName() != "tableData" && nChildTableData == 1u) {
    // 1D and 2D tables
    leafData = el->FindElement("tableData");
  } else if (nChildTableData > 1u) {
    // N-dimensional table: multiple <tableData> children (3D+), or a container
    // <tableData> with nested <tableData> children (4D+)
    Type = ttND;
    // Fill unused elements with NaNs to detect illegal access.
    Data.push_back(std::numeric_limits<double>::quiet_NaN());
    nCols = 1u;

    Element* child = el->FindElement("tableData");

    while (child) {
      const string brkpt_string = child->GetAttributeValue("breakPoint");
      if (brkpt_string.empty()) {
        XMLLogException err(child);
        err << "FGTable: missing breakPoint on <tableData>\n";
        throw err;
      }

      auto subtable = std::make_unique<FGTable>(PropertyManager, child);

      if (nDims == 0u) {
        nDims = subtable->nDims + 1u;
        if (nDims < 3u) {
          XMLLogException err(child);
          err << "FGTable: nested tables must contain at least 2D subtables\n";
          throw err;
        }
      } else if (subtable->nDims + 1u != nDims) {
        XMLLogException err(child);
        err << "FGTable: inconsistent sub-table dimensionality in nested table\n";
        throw err;
      }

      Data.push_back(child->GetAttributeValueAsNumber("breakPoint"));
      Tables.push_back(std::move(subtable));
      child = el->FindNextElement("tableData");
    }

    nRows = static_cast<unsigned int>(Tables.size());
  } else {
    XMLLogException err(el);
    err << "FGTable: <tableData> elements are missing\n";
    throw err;
  }

  if (leafData) {
    stringstream buf;
    AppendNumericData(leafData, buf);

    nDims = InferLeafDimension(leafData);

    switch (nDims) {
    case 1u:
      nRows = leafData->GetNumDataLines();
      nCols = 1u;
      Type = tt1D;
      // Fill unused elements with NaNs to detect illegal access.
      Data.push_back(std::numeric_limits<double>::quiet_NaN());
      Data.push_back(std::numeric_limits<double>::quiet_NaN());
      *this << buf;
      break;
    case 2u:
      nRows = leafData->GetNumDataLines()-1u;
      nCols = FindNumColumns(leafData->GetDataLine(0));
      Type = tt2D;
      // Fill unused elements with NaNs to detect illegal access.
      Data.push_back(std::numeric_limits<double>::quiet_NaN());
      *this << buf;
      break;
    default:
      assert(false); // Should never be called
      break;
    }
  }

  if (declared_dimension != 0u && declared_dimension != nDims) {
    XMLLogException err(el);
    err << "FGTable: " << declared_dimension
        << " lookup axes were declared, but the tableData nesting implies a "
        << nDims << "D table.\n";
    throw err;
  }

  Debug(0);

  // Sanity checks: lookup indices must be increasing monotonically

  // find next xml element containing a name attribute
  // to indicate where the error occured
  Element* nameel = el;
  while (nameel && nameel->GetAttributeValue("name") == "")
    nameel=nameel->GetParent();

  // check breakpoints, if applicable
  if (Type == ttND) {
    for (unsigned int b=2; b<=Tables.size(); ++b) {
      if (Data[b] <= Data[b-1]) {
        XMLLogException err(el);
        err << LogFormat::RED << LogFormat::BOLD
            << "  FGTable: breakpoint lookup is not monotonically increasing\n"
            << "  in breakpoint " << b;
        if (nameel) err << " of table in " << nameel->GetAttributeValue("name");
        err << ":\n" << LogFormat::RESET
            << "  " << Data[b] << "<=" << Data[b-1] << "\n";
        throw err;
      }
    }
  }

  // check columns, if applicable
  if (Type == tt2D) {
    for (unsigned int c=2; c<=nCols; ++c) {
      if (Data[c] <= Data[c-1]) {
        XMLLogException err(el);
        err << LogFormat::RED << LogFormat::BOLD
            << "  FGTable: column lookup is not monotonically increasing\n"
            << "  in column " << c;
        if (nameel != 0) err << " of table in " << nameel->GetAttributeValue("name");
        err << ":\n" << LogFormat::RESET
            << "  " << Data[c] << "<=" << Data[c-1] << "\n";
        throw err;
      }
    }
  }

  // check rows
  if (Type != ttND) { // in ND tables, check only rows of subtables
    for (size_t r=2; r<=nRows; ++r) {
      if (Data[r*(nCols+1)]<=Data[(r-1)*(nCols+1)]) {
        XMLLogException err(el);
        err << LogFormat::RED << LogFormat::BOLD
            << "  FGTable: row lookup is not monotonically increasing\n"
            << "  in row " << r;
        if (nameel != 0) err << " of table in " << nameel->GetAttributeValue("name");
        err << ":\n" << LogFormat::RESET
            << "  " << Data[r*(nCols+1)] << "<=" << Data[(r-1)*(nCols+1)] << "\n";
        throw err;
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
  case ttND:
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

void FGTable::missingData(Element* el, unsigned int expected_size, size_t actual_size)
{
  XMLLogException err(el);
  err << LogFormat::RED << LogFormat::BOLD
      << "  FGTable: Missing data";
  if (!Name.empty()) err << " in table " << Name;
  err << ":\n" << LogFormat::RESET
      << "  Expecting " << expected_size << " elements while "
      << actual_size << " elements were provided.\n";
  throw err;
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
  if (Type == ttND) {
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
  assert(nDims > 0u);

  switch(Type) {
  case tt1D:
    return GetValue(lookupProperty[eRow]->getDoubleValue());
  case tt2D:
    return GetValue(lookupProperty[eRow]->getDoubleValue(),
                    lookupProperty[eColumn]->getDoubleValue());
  case ttND:
  {
    std::vector<double> keys(nDims);
    for (unsigned int axis=0u; axis<nDims; ++axis) {
      assert(HasLookupProperty(axis));
      keys[axis] = lookupProperty[axis]->getDoubleValue();
    }

    return GetValue(keys.data());
  }
  default:
    assert(false);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(const std::vector<double>& keys) const
{
  assert(!keys.empty());
  assert(keys.size() >= nDims);
  return GetValue(keys.data());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(const double* keys) const
{
  if (Type == tt1D)
    return GetValue(keys[0]);

  if (Type == tt2D)
    return GetValue(keys[0], keys[1]);

  assert(Type == ttND);
  assert(Data.size() == nRows+1);

  const double outerKey = keys[nDims-1];

  // If the key is off the end (or before the beginning) of the table, just
  // return the boundary-table value, do not extrapolate.
  if (outerKey <= Data[1])
    return Tables[0]->GetValue(keys);
  else if (outerKey >= Data[nRows])
    return Tables[nRows-1]->GetValue(keys);

  // Search for the right breakpoint.
  // This is a linear search, the algorithm is O(n).
  unsigned int r = 2u;
  while (Data[r] < outerKey) r++;

  double x0 = Data[r-1u];
  double Span = Data[r] - x0;
  assert(Span > 0.0);
  double Factor = (outerKey - x0) / Span;
  assert(Factor >= 0.0 && Factor <= 1.0);

  double y0 = Tables[r-2u]->GetValue(keys);
  return Factor*(Tables[r-1u]->GetValue(keys) - y0) + y0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double key) const
{
  assert((Type == tt1D) || (Type == tt2D && nCols == 1));
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
  assert(Type == tt2D);
  assert(Data.size() == (nCols+1)*(nRows+1));

  if (nCols == 1) return GetValue(rowKey);

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
  const double keys[3] = {rowKey, colKey, tableKey};
  assert(nDims == 3);
  return GetValue(keys);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double a1, double a2, double a3, double a4) const
{
  const double keys[4] = {a1, a2, a3, a4};
  assert(nDims == 4);
  return GetValue(keys);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double a1, double a2, double a3, double a4, double a5) const
{
  const double keys[5] = {a1, a2, a3, a4, a5};
  assert(nDims == 5);
  return GetValue(keys);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGTable::GetValue(double a1, double a2, double a3, double a4,
                         double a5, double a6) const
{
  const double keys[6] = {a1, a2, a3, a4, a5, a6};
  assert(nDims == 6);
  return GetValue(keys);
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
  assert(Type != ttND);

  in_stream >> x;
  while(in_stream) {
    Data.push_back(x);
    in_stream >> x;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGTable& FGTable::operator<<(const double x)
{
  assert(Type != ttND);
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
  FGLogging out(LogLevel::STDOUT);
  out << std::setprecision(4);

  switch(Type) {
    case tt1D:
      out << "    1 dimensional table with " << nRows << " rows.\n";
      break;
    case tt2D:
      out << "    2 dimensional table with " << nRows << " rows, " << nCols << " columns.\n";
      break;
    case ttND:
      out << "    " << nDims << " dimensional table with " << nRows
          << " breakpoints, " << Tables.size() << " subtables.\n";
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
    out << "\t";
    if (Type == tt2D) {
      if (r == startRow)
        out << "\t";
      else
        startCol = 0;
    }

    for (unsigned int c=startCol; c<=nCols; c++) {
      out << Data[p++] << "\t";
      if (Type == ttND) {
        out << "\n";
        Tables[r-1]->Print();
      }
    }
    out << "\n";
  }
  out << std::fixed;
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
          XMLLogException err(el);
          err << "Malformed table name with number: " << Prefix
              << " and property name: " << Name
              << " but no \"#\" sign for substitution.\n";
          throw err;
        }
      } else {
        Name = Prefix + "/" + Name;
      }
    }
    string tmp = PropertyManager->mkPropertyName(Name, false);

    if (PropertyManager->HasNode(tmp)) {
      SGPropertyNode* _property = PropertyManager->GetNode(tmp);
      if (_property->isTied()) {
        XMLLogException err(el);
        err << "Property " << tmp << " has already been successfully bound (late).\n";
        throw err;
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
    FGLogging log(LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGTable\n";
    if (from == 1) log << "Destroyed:    FGTable\n";
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
