/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTable.h
 Author:       Jon S. Berndt
 Date started: 1/9/2001

 ------------- Copyright (C) 2001  Jon S. Berndt (jon@jsbsim.org) --------------

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

HISTORY
--------------------------------------------------------------------------------
JSB  1/9/00          Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTABLE_H
#define FGTABLE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGParameter.h"
#include "math/FGPropertyValue.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class Element;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Lookup table class.
Models a one, two, or three dimensional lookup table for use in aerodynamics
and function definitions.

For a single "vector" lookup table, the format is as follows:

@code
<table name="property_name">
  <independentVar lookup="row"> property_name </independentVar>
  <tableData>
    key_1 value_1
    key_2 value_2
    ...  ...
    key_n value_n
  </tableData>
</table>
@endcode

The lookup="row" attribute in the independentVar element is option in this case;
it is assumed that the independentVar is a row variable.

A "real life" example is as shown here:

@code
<table>
  <independentVar lookup="row"> aero/alpha-rad </independentVar>
  <tableData>
   -1.57  1.500
   -0.26  0.033
    0.00  0.025
    0.26  0.033
    1.57  1.500
  </tableData>
</table>
@endcode

The first column in the data table represents the lookup index (or "key").  In
this case, the lookup index is aero/alpha-rad (angle of attack in radians).
If alpha is 0.26 radians, the value returned from the lookup table
would be 0.033.

The definition for a 2D table, is as follows:

@code
<table name="property_name">
  <independentVar lookup="row"> property_name </independentVar>
  <independentVar lookup="column"> property_name </independentVar>
  <tableData>
                {col_1_key   col_2_key   ...  col_n_key }
    {row_1_key} {col_1_data  col_2_data  ...  col_n_data}
    {row_2_key} {...         ...         ...  ...       }
    { ...     } {...         ...         ...  ...       }
    {row_n_key} {...         ...         ...  ...       }
  </tableData>
</table>
@endcode

The data is in a gridded format.

A "real life" example is as shown below. Alpha in radians is the row lookup (alpha
breakpoints are arranged in the first column) and flap position in degrees is

@code
<table>
  <independentVar lookup="row">aero/alpha-rad</independentVar>
  <independentVar lookup="column">fcs/flap-pos-deg</independentVar>
  <tableData>
                0.0         10.0        20.0         30.0
    -0.0523599  8.96747e-05 0.00231942  0.0059252    0.00835082
    -0.0349066  0.000313268 0.00567451  0.0108461    0.0140545
    -0.0174533  0.00201318  0.0105059   0.0172432    0.0212346
     0.0        0.0051894   0.0168137   0.0251167    0.0298909
     0.0174533  0.00993967  0.0247521   0.0346492    0.0402205
     0.0349066  0.0162201   0.0342207   0.0457119    0.0520802
     0.0523599  0.0240308   0.0452195   0.0583047    0.0654701
     0.0698132  0.0333717   0.0577485   0.0724278    0.0803902
     0.0872664  0.0442427   0.0718077   0.088081     0.0968405
  </tableData>
</table>
@endcode

The definition for a 3D table in a coefficient would be (for example):

@code
<table name="property_name">
  <independentVar lookup="row"> property_name </independentVar>
  <independentVar lookup="column"> property_name </independentVar>
  <tableData breakpoint="table_1_key">
                {col_1_key   col_2_key   ...  col_n_key }
    {row_1_key} {col_1_data  col_2_data  ...  col_n_data}
    {row_2_key} {...         ...         ...  ...       }
    { ...     } {...         ...         ...  ...       }
    {row_n_key} {...         ...         ...  ...       }
  </tableData>
  <tableData breakpoint="table_2_key">
                {col_1_key   col_2_key   ...  col_n_key }
    {row_1_key} {col_1_data  col_2_data  ...  col_n_data}
    {row_2_key} {...         ...         ...  ...       }
    { ...     } {...         ...         ...  ...       }
    {row_n_key} {...         ...         ...  ...       }
  </tableData>
  ...
  <tableData breakpoint="table_n_key">
                {col_1_key   col_2_key   ...  col_n_key }
    {row_1_key} {col_1_data  col_2_data  ...  col_n_data}
    {row_2_key} {...         ...         ...  ...       }
    { ...     } {...         ...         ...  ...       }
    {row_n_key} {...         ...         ...  ...       }
  </tableData>
</table>
@endcode

[Note the "breakpoint" attribute in the tableData element, above.]

Here's an example:

@code
<table>
  <independentVar lookup="row">fcs/row-value</independentVar>
  <independentVar lookup="column">fcs/column-value</independentVar>
  <independentVar lookup="table">fcs/table-value</independentVar>
  <tableData breakPoint="-1.0">
           -1.0     1.0
    0.0     1.0000  2.0000
    1.0     3.0000  4.0000
  </tableData>
  <tableData breakPoint="0.0000">
            0.0     10.0
    2.0     1.0000  2.0000
    3.0     3.0000  4.0000
  </tableData>
  <tableData breakPoint="1.0">
           0.0     10.0     20.0
     2.0   1.0000   2.0000   3.0000
     3.0   4.0000   5.0000   6.0000
    10.0   7.0000   8.0000   9.0000
  </tableData>
</table>
@endcode

In addition to using a Table for something like a coefficient, where all the
row and column elements are read in from a file, a Table could be created
and populated completely within program code:

@code
// First column is thi, second is neta (combustion efficiency)
Lookup_Combustion_Efficiency = new FGTable(12);

*Lookup_Combustion_Efficiency << 0.00 << 0.980;
*Lookup_Combustion_Efficiency << 0.90 << 0.980;
*Lookup_Combustion_Efficiency << 1.00 << 0.970;
*Lookup_Combustion_Efficiency << 1.05 << 0.950;
*Lookup_Combustion_Efficiency << 1.10 << 0.900;
*Lookup_Combustion_Efficiency << 1.15 << 0.850;
*Lookup_Combustion_Efficiency << 1.20 << 0.790;
*Lookup_Combustion_Efficiency << 1.30 << 0.700;
*Lookup_Combustion_Efficiency << 1.40 << 0.630;
*Lookup_Combustion_Efficiency << 1.50 << 0.570;
*Lookup_Combustion_Efficiency << 1.60 << 0.525;
*Lookup_Combustion_Efficiency << 2.00 << 0.345;
@endcode

The first column in the table, above, is thi (the lookup index, or key). The
second column is the output data - in this case, "neta" (the Greek letter
referring to combustion efficiency). Later on, the table is used like this:

@code
combustion_efficiency = Lookup_Combustion_Efficiency->GetValue(equivalence_ratio);
@endcode

@author Jon S. Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGTable : public FGParameter, public FGJSBBase
{
public:
  /// Destructor
  ~FGTable();

  /** This is the very important copy constructor.
      @param table a const reference to a table.*/
  FGTable(const FGTable& table);
  /// Copy assignment constructor.
  /* MSVC issues an error C2280 if not defined : it is needed by
     std::unique_ptr<FGTable>.
     See StackOverflow: https://stackoverflow.com/questions/31264984/c-compiler-error-c2280-attempting-to-reference-a-deleted-function-in-visual */
  FGTable& operator=(const FGTable&);

  /// The constructor for a table
  FGTable (std::shared_ptr<FGPropertyManager> propMan, Element* el,
           const std::string& prefix="");
  FGTable (int);
  FGTable (int, int);

  /// Get the current table value
  double GetValue(void) const;
  /// @brief Get a value from a 1D internal table
  /// @param key Row coordinate at which the value must be interpolated
  /// @return The interpolated value
  double GetValue(double key) const;
  /// @brief Get a value from a 2D internal table
  /// @param rowKey Row coordinate at which the value must be interpolated
  /// @param colKey Column coordinate at which the value must be interpolated
  /// @return The interpolated value
  double GetValue(double rowKey, double colKey) const;
  /// @brief Get a value from a 3D internal table
  /// @param rowKey Row coordinate at which the value must be interpolated
  /// @param colKey Column coordinate at which the value must be interpolated
  /// @param TableKey Table coordinate at which the value must be interpolated
  /// @return The interpolated value
  double GetValue(double rowKey, double colKey, double TableKey) const;

  double GetMinValue(void) const;
  double GetMinValue(double colKey) const;
  double GetMinValue(double colKey, double TableKey) const;

  /** Read the table in.
      Data in the config file should be in matrix format with the row
      independents as the first column and the column independents in
      the first row.  The implication of this layout is that there should
      be no value in the upper left corner of the matrix e.g:
      <pre>
           0  10  20 30 ...
      -5   1  2   3  4  ...
       ...
       </pre>

       For multiple-table (i.e. 3D) data sets there is an additional number
       key in the table definition. For example:

      <pre>
       0.0
           0  10  20 30 ...
      -5   1  2   3  4  ...
       ...
       </pre>
       */

  void operator<<(std::istream&);
  FGTable& operator<<(const double x);

  double GetElement(unsigned int r, unsigned int c) const;
  double operator()(unsigned int r, unsigned int c) const
  { return GetElement(r, c); }

  void SetRowIndexProperty(SGPropertyNode *node)
  { lookupProperty[eRow] = new FGPropertyValue(node); }
  void SetColumnIndexProperty(SGPropertyNode *node)
  { lookupProperty[eColumn] = new FGPropertyValue(node); }

  unsigned int GetNumRows() const {return nRows;}

  void Print(void);

  std::string GetName(void) const {return Name;}

private:
  enum type {tt1D, tt2D, tt3D} Type;
  enum axis {eRow=0, eColumn, eTable};
  bool internal = false;
  std::shared_ptr<FGPropertyManager> PropertyManager; // Property root used to do late binding.
  FGPropertyValue_ptr lookupProperty[3];
  std::vector<double> Data;
  std::vector<std::unique_ptr<FGTable>> Tables;
  unsigned int nRows, nCols;
  std::string Name;
  void bind(Element* el, const std::string& Prefix);
  void missingData(Element *el, unsigned int expected_size, size_t actual_size);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif
