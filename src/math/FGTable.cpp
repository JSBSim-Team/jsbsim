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

#include <algorithm>
#include <cctype>
#include <limits>
#include <assert.h>

#include "FGTable.h"
#include "input_output/FGXMLElement.h"
#include "input_output/string_utilities.h"

using namespace std;

namespace JSBSim {

  namespace { // anonymous namespace for helper functions

    unsigned int FindNumColumns(const string &test_line)
    {
      size_t position = 0;
      unsigned int nCols = 0;
      while ((position = test_line.find_first_not_of(" \t", position)) != string::npos) {
        nCols++;
        position = test_line.find_first_of(" \t", position);
      }
      return nCols;
    }

    unsigned int InferLeafDimension(Element *tableData)
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
        for (unsigned int i = 1; i < nLines; ++i) {
          if (FindNumColumns(tableData->GetDataLine(i)) != secondLineColumns) {
            XMLLogException err(tableData);
            err << "Invalid number of columns in line "
                << tableData->GetLineNumber() + i << "\n";
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

      for (unsigned int i = 1; i < nLines; ++i) {
        if (FindNumColumns(tableData->GetDataLine(i)) != 2u) {
          XMLLogException err(tableData);
          err << "Invalid number of columns in line "
              << tableData->GetLineNumber() + i << "\n";
          throw err;
        }
      }

      return 1u;
    }

    unsigned int ParseLookupAxis(const string &lookup_axis)
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
            std::all_of(suffix.begin(), suffix.end(),
                        [](unsigned char c)
                        { return std::isdigit(c) != 0; })) {
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

    void AppendNumericData(Element *tableData, stringstream &buf)
    {
      for (unsigned int i = 0; i < tableData->GetNumDataLines(); ++i) {
        const string line = tableData->GetDataLine(i);
        if (line.find_first_not_of("0123456789.-+eE \t\n") != string::npos) {
          XMLLogException err(tableData);
          err << "   Illegal character found in line "
              << tableData->GetLineNumber() + i + 1 << ": \n"
              << line << "\n";
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
      : nRows(NRows), nCols(1u), nDims(1u) {
    Type = tt1D;
    Data.push_back(std::numeric_limits<double>::quiet_NaN());
    Data.push_back(std::numeric_limits<double>::quiet_NaN());
    Debug(0);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  FGTable::FGTable(int NRows, int NCols)
      : nRows(NRows), nCols(NCols), nDims(2u) {
    Type = tt2D;
    Data.push_back(std::numeric_limits<double>::quiet_NaN());
    Debug(0);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  FGTable::FGTable(const FGTable &t)
      : PropertyManager(t.PropertyManager) {
    Type = t.Type;
    nRows = t.nRows;
    nCols = t.nCols;
    nDims = t.nDims;
    internal = t.internal;
    Name = t.Name;
    lookupProperty = t.lookupProperty;

    Tables.reserve(t.Tables.size());
    for (const auto &table : t.Tables)
      Tables.push_back(std::make_unique<FGTable>(*table));

    Data = t.Data;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  FGTable::FGTable(std::shared_ptr<FGPropertyManager> pm, Element *el,
                   const std::string &Prefix)
      : PropertyManager(pm) {
    Name = el->GetAttributeValue("name");

    const string call_type = el->GetAttributeValue("type");
    if (call_type == "internal") {
      internal = true;
    }
    else if (!call_type.empty()) {
      XMLLogException err(el);
      err << "  An unknown table type attribute is listed: " << call_type << "\n";
      throw err;
    }

    unsigned int declared_dimension = 0u;

    Element *axisElement = el->FindElement("independentVar");
    if (axisElement) {
      if (internal) {
        FGXMLLogging log(el, LogLevel::ERROR);
        log << LogFormat::RED
            << "  This table specifies both 'internal' call type\n"
            << "  and specific lookup properties via the 'independentVar' element.\n"
            << "  These are mutually exclusive specifications. The 'internal'\n"
            << "  attribute will be ignored.\n"
            << LogFormat::DEFAULT;
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
    }
    else if (!internal && el->GetAttributeValue("breakPoint").empty() &&
             el->GetName() != "tableData") {
      XMLLogException err(el);
      err << "No independentVars found, and table is not marked as internal,"
          << " nor is it a sliced sub-table.\n";
      throw err;
    }

    Element *leafData = nullptr;

    if (el->GetName() == "tableData") {
      leafData = el;
    }
    else {
      const unsigned int nTableData = el->GetNumElements("tableData");
      const unsigned int nSlices = el->GetNumElements("slice");

      if (nTableData > 0u && nSlices > 0u) {
        XMLLogException err(el);
        err << "FGTable: mixed <slice> and <tableData> children are not allowed\n";
        throw err;
      }

      if (nSlices > 0u || nTableData > 1u) {
        Type = ttND;
        Data.push_back(std::numeric_limits<double>::quiet_NaN());
        nCols = 1u;

        const char *child_name = (nSlices > 0u) ? "slice" : "tableData";
        Element *child = el->FindElement(child_name);

        while (child) {
          const string brkpt_string = child->GetAttributeValue("breakPoint");
          if (brkpt_string.empty()) {
            XMLLogException err(child);
            err << "FGTable: missing breakPoint on <" << child_name << ">\n";
            throw err;
          }

          auto subtable = std::make_unique<FGTable>(PropertyManager, child);

          if (nDims == 0u) {
            nDims = subtable->nDims + 1u;
            if (nDims < 3u) {
              XMLLogException err(child);
              err << "FGTable: sliced tables must contain at least 2D subtables\n";
              throw err;
            }
          }
          else if (subtable->nDims + 1u != nDims) {
            XMLLogException err(child);
            err << "FGTable: inconsistent sub-table dimensionality in sliced table\n";
            throw err;
          }

          for (unsigned int axis = 0u; axis + 1u < nDims; ++axis) {
            if (HasLookupProperty(axis) && !subtable->HasLookupProperty(axis))
              subtable->SetLookupProperty(axis, lookupProperty[axis]);
          }

          Data.push_back(child->GetAttributeValueAsNumber("breakPoint"));
          Tables.push_back(std::move(subtable));
          child = el->FindNextElement(child_name);
        }

        nRows = static_cast<unsigned int>(Tables.size());

        if (declared_dimension != 0u && declared_dimension != nDims) {
          XMLLogException err(el);
          err << "FGTable: " << declared_dimension
              << " lookup axes were declared, but the slice nesting implies a "
              << nDims << "D table.\n";
          throw err;
        }
      }
      else if (nTableData == 1u) {
        leafData = el->FindElement("tableData");
      }
      else {
        XMLLogException err(el);
        err << "FGTable: <tableData> or <slice> elements are missing\n";
        throw err;
      }
    }

    if (leafData) {
      const unsigned int dimension = InferLeafDimension(leafData);

      if (declared_dimension != 0u && declared_dimension != dimension) {
        XMLLogException err(el);
        err << "FGTable: " << declared_dimension
            << " lookup axes were declared, but the data layout is "
            << dimension << "D.\n";
        throw err;
      }

      nDims = dimension;

      stringstream buf;
      AppendNumericData(leafData, buf);

      switch (dimension) {
      case 1u:
        nRows = leafData->GetNumDataLines();
        nCols = 1u;
        Type = tt1D;
        Data.push_back(std::numeric_limits<double>::quiet_NaN());
        Data.push_back(std::numeric_limits<double>::quiet_NaN());
        *this << buf;
        break;

      case 2u:
        nRows = leafData->GetNumDataLines() - 1u;
        nCols = FindNumColumns(leafData->GetDataLine(0));
        Type = tt2D;
        Data.push_back(std::numeric_limits<double>::quiet_NaN());
        *this << buf;
        break;

      default:
        assert(false);
        break;
      }
    }

    if (!internal && el->GetAttributeValue("breakPoint").empty()) {
      for (unsigned int axis = 0u; axis < nDims; ++axis) {
        if (!HasLookupProperty(axis)) {
          XMLLogException err(el);
          err << "FGTable: missing lookup axis \"" << LookupAxisName(axis) << "\"\n";
          throw err;
        }
      }
    }

    Debug(0);

    Element *nameel = el;
    while (nameel && nameel->GetAttributeValue("name") == "")
      nameel = nameel->GetParent();

    if (Type == ttND) {
      for (unsigned int b = 2; b <= Tables.size(); ++b) {
        if (Data[b] <= Data[b - 1]) {
          XMLLogException err(el);
          err << LogFormat::RED << LogFormat::BOLD
              << "  FGTable: breakpoint lookup is not monotonically increasing\n"
              << "  in breakpoint " << b;
          if (nameel)
            err << " of table in " << nameel->GetAttributeValue("name");
          err << ":\n"
              << LogFormat::RESET
              << "  " << Data[b] << "<=" << Data[b - 1] << "\n";
          throw err;
        }
      }
    }

    if (Type == tt2D) {
      for (unsigned int c = 2; c <= nCols; ++c) {
        if (Data[c] <= Data[c - 1]) {
          XMLLogException err(el);
          err << LogFormat::RED << LogFormat::BOLD
              << "  FGTable: column lookup is not monotonically increasing\n"
              << "  in column " << c;
          if (nameel != 0)
            err << " of table in " << nameel->GetAttributeValue("name");
          err << ":\n"
              << LogFormat::RESET
              << "  " << Data[c] << "<=" << Data[c - 1] << "\n";
          throw err;
        }
      }
    }

    if (Type != ttND) {
      for (size_t r = 2; r <= nRows; ++r) {
        if (Data[r * (nCols + 1)] <= Data[(r - 1) * (nCols + 1)]) {
          XMLLogException err(el);
          err << LogFormat::RED << LogFormat::BOLD
              << "  FGTable: row lookup is not monotonically increasing\n"
              << "  in row " << r;
          if (nameel != 0)
            err << " of table in " << nameel->GetAttributeValue("name");
          err << ":\n"
              << LogFormat::RESET
              << "  " << Data[r * (nCols + 1)] << "<=" << Data[(r - 1) * (nCols + 1)] << "\n";
          throw err;
        }
      }
    }

    switch (Type) {
    case tt1D:
      if (Data.size() != 2u * nRows + 2u)
        missingData(el, 2u * nRows, Data.size() - 2u);
      break;
    case tt2D:
      if (Data.size() != static_cast<size_t>(nRows + 1u) * (nCols + 1u))
        missingData(el, (nRows + 1u) * (nCols + 1u) - 1u, Data.size() - 1u);
      break;
    case ttND:
      if (Data.size() != nRows + 1u)
        missingData(el, nRows, Data.size() - 1u);
      break;
    default:
      assert(false);
      break;
    }

    bind(el, Prefix);

    if (debug_lvl & 1)
      Print();
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  void FGTable::missingData(Element *el, unsigned int expected_size, size_t actual_size) 
  {
    XMLLogException err(el);
    err << LogFormat::RED << LogFormat::BOLD
        << "  FGTable: Missing data";
    if (!Name.empty())
      err << " in table " << Name;
    err << ":\n"
        << LogFormat::RESET
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
      SGPropertyNode *node = PropertyManager->GetNode(tmp);
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
      assert(Data.size() == nRows + 1u);
      return Data[r];
    }
    assert(Data.size() == static_cast<size_t>(nCols + 1u) * (nRows + 1u));
    return Data[r * (nCols + 1u) + c];
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(void) const 
  {
    assert(!internal);
    assert(nDims > 0u);

    std::vector<double> keys(nDims);
    for (unsigned int axis = 0u; axis < nDims; ++axis) {
      assert(HasLookupProperty(axis));
      keys[axis] = lookupProperty[axis]->getDoubleValue();
    }

    return GetValue(keys.data(), nDims);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(const std::vector<double> &keys) const 
  {
    assert(!keys.empty());
    return GetValue(keys.data(), static_cast<unsigned int>(keys.size()));
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(const double *keys, unsigned int dimension) const 
  {
    assert(dimension == nDims);

    if (Type == tt1D)
      return GetValue(keys[0]);

    if (Type == tt2D)
      return GetValue(keys[0], keys[1]);

    assert(Type == ttND);
    assert(dimension >= 3u);
    assert(Data.size() == nRows + 1u);

    const double outerKey = keys[dimension - 1u];

    if (outerKey <= Data[1])
      return Tables[0]->GetValue(keys, dimension - 1u);
    else if (outerKey >= Data[nRows])
      return Tables[nRows - 1u]->GetValue(keys, dimension - 1u);

    unsigned int r = 2u;
    while (Data[r] < outerKey)
      r++;

    const double x0 = Data[r - 1u];
    const double span = Data[r] - x0;
    assert(span > 0.0);
    const double factor = (outerKey - x0) / span;
    assert(factor >= 0.0 && factor <= 1.0);

    const double y0 = Tables[r - 2u]->GetValue(keys, dimension - 1u);
    return factor * (Tables[r - 1u]->GetValue(keys, dimension - 1u) - y0) + y0;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(double key) const 
  {
    assert((Type == tt1D) || (Type == tt2D && nCols == 1u));
    assert(Data.size() == 2u * nRows + 2u);

    if (key <= Data[2])
      return Data[3];
    else if (key >= Data[2u * nRows])
      return Data[2u * nRows + 1u];

    unsigned int r = 2u;
    while (Data[2u * r] < key)
      r++;

    const double x0 = Data[2u * r - 2u];
    const double span = Data[2u * r] - x0;
    assert(span > 0.0);
    const double factor = (key - x0) / span;
    assert(factor >= 0.0 && factor <= 1.0);

    const double y0 = Data[2u * r - 1u];
    return factor * (Data[2u * r + 1u] - y0) + y0;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(double rowKey, double colKey) const 
  {
    if (Type == tt1D || (Type == tt2D && nCols == 1u))
      return GetValue(rowKey);

    assert(Type == tt2D);
    assert(Data.size() == static_cast<size_t>(nCols + 1u) * (nRows + 1u));

    unsigned int c = 2u;
    while (Data[c] < colKey && c < nCols)
      c++;
    double x0 = Data[c - 1u];
    double span = Data[c] - x0;
    assert(span > 0.0);
    double cFactor = Constrain(0.0, (colKey - x0) / span, 1.0);

    if (nRows == 1u) {
      const double y0 = Data[(nCols + 1u) + c - 1u];
      return cFactor * (Data[(nCols + 1u) + c] - y0) + y0;
    }

    size_t r = 2u;
    while (Data[r * (nCols + 1u)] < rowKey && r < nRows)
      r++;
    x0 = Data[(r - 1u) * (nCols + 1u)];
    span = Data[r * (nCols + 1u)] - x0;
    assert(span > 0.0);
    const double rFactor = Constrain(0.0, (rowKey - x0) / span, 1.0);

    const double col1temp =
        rFactor * Data[r * (nCols + 1u) + c - 1u] +
        (1.0 - rFactor) * Data[(r - 1u) * (nCols + 1u) + c - 1u];

    const double col2temp =
        rFactor * Data[r * (nCols + 1u) + c] +
        (1.0 - rFactor) * Data[(r - 1u) * (nCols + 1u) + c];

    return cFactor * (col2temp - col1temp) + col1temp;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(double rowKey, double colKey, double tableKey) const 
  {
    const double keys[3] = {rowKey, colKey, tableKey};
    return GetValue(keys, 3u);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(double a1, double a2, double a3, double a4) const 
  {
    const double keys[4] = {a1, a2, a3, a4};
    return GetValue(keys, 4u);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(double a1, double a2, double a3, double a4, double a5) const 
  {
    const double keys[5] = {a1, a2, a3, a4, a5};
    return GetValue(keys, 5u);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetValue(double a1, double a2, double a3, double a4,
                           double a5, double a6) const 
  {
    const double keys[6] = {a1, a2, a3, a4, a5, a6};
    return GetValue(keys, 6u);
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  double FGTable::GetMinValue(void) const 
  {
    assert(Type == tt1D);
    assert(Data.size() == 2 * nRows + 2);

    double minValue = HUGE_VAL;

    for (unsigned int i = 1; i <= nRows; ++i)
      minValue = std::min(minValue, Data[2 * i + 1]);

    return minValue;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  void FGTable::operator<<(istream &in_stream) 
  {
    double x;
    assert(Type != ttND);

    in_stream >> x;
    while (in_stream) {
      Data.push_back(x);
      in_stream >> x;
    }
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  FGTable &FGTable::operator<<(const double x) 
  {
    assert(Type != ttND);
    Data.push_back(x);

    // Check column is monotically increasing
    size_t n = Data.size();
    if (Type == tt2D && nCols > 1 && n >= 3 && n <= nCols + 1) {
      if (Data.at(n - 1) <= Data.at(n - 2))
        throw BaseException("FGTable: column lookup is not monotonically increasing");
    }

    // Check row is monotically increasing
    size_t row = (n - 1) / (nCols + 1);
    if (row >= 2 && row * (nCols + 1) == n - 1) {
      if (Data.at(row * (nCols + 1)) <= Data.at((row - 1) * (nCols + 1)))
        throw BaseException("FGTable: row lookup is not monotonically increasing");
    }

    return *this;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  void FGTable::Print(void) 
  {
    FGLogging out(LogLevel::STDOUT);
    out << std::setprecision(4);

    switch (Type) {
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
    unsigned int startCol = 1, startRow = 1;
    unsigned int p = 1;

    if (Type == tt1D) {
      startCol = 0;
      p = 2;
    }
    if (Type == tt2D)
      startRow = 0;

    for (unsigned int r = startRow; r <= nRows; r++) {
      out << "\t";
      if (Type == tt2D) {
        if (r == startRow)
          out << "\t";
        else
          startCol = 0;
      }

      for (unsigned int c = startCol; c <= nCols; c++) {
        out << Data[p++] << "\t";
        if (Type == ttND) {
          out << "\n";
          Tables[r - 1]->Print();
        }
      }
      out << "\n";
    }
    out << std::fixed;
  }

  //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

  void FGTable::bind(Element *el, const string &Prefix) 
  {
    if (!Name.empty() && !internal) {
      if (!Prefix.empty()) {
        if (is_number(Prefix)) {
          if (Name.find("#") != string::npos) {
            Name = replace(Name, "#", Prefix);
          }
          else {
            XMLLogException err(el);
            err << "Malformed table name with number: " << Prefix
                << " and property name: " << Name
                << " but no \"#\" sign for substitution.\n";
            throw err;
          }
        }
        else {
          Name = Prefix + "/" + Name;
        }
      }
      string tmp = PropertyManager->mkPropertyName(Name, false);

      if (PropertyManager->HasNode(tmp)) {
        SGPropertyNode *_property = PropertyManager->GetNode(tmp);
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
    if (debug_lvl <= 0)
      return;

    if (debug_lvl & 1) { // Standard console startup message output
      if (from == 0) {
      } // Constructor
    }
    if (debug_lvl & 2) { // Instantiation/Destruction notification
      FGLogging log(LogLevel::DEBUG);
      if (from == 0)
        log << "Instantiated: FGTable\n";
      if (from == 1)
        log << "Destroyed:    FGTable\n";
    }
    if (debug_lvl & 4) { // Run() method entry print for FGModel-derived objects
    }
    if (debug_lvl & 8) { // Runtime state variables
    }
    if (debug_lvl & 16) { // Sanity checking
    }
    if (debug_lvl & 64) {
      if (from == 0) { // Constructor
      }
    }
  }
}
