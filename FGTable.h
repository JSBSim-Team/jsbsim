/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGTable.h
 Author:       Jon S. Berndt
 Date started: 1/9/2001

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) --------------

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

#include "FGConfigFile.h"
#include "FGJSBBase.h"
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TABLE "$Id: FGTable.h,v 1.19 2005/01/20 07:27:35 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

using std::vector;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Lookup table class.
    Models a one, two, or three dimensional lookup table for use in FGCoefficient,
    FGPropeller, etc.  A one-dimensional table is called a "VECTOR" in a coefficient
    definition. For example:
<pre>
    \<COEFFICIENT NAME="{short name}" TYPE="VECTOR">
      {name}
      {number of rows}
      {row lookup property}
      {non-dimensionalizing properties}
      {row_1_key} {col_1_data}
      {row_2_key} {...       }
      { ...     } {...       }
      {row_n_key} {...       }
    \</COEFFICIENT>
</pre>
    A "real life" example is as shown here:
<pre>
    \<COEFFICIENT NAME="CLDf" TYPE="VECTOR">
      Delta_lift_due_to_flap_deflection
      4
      fcs/flap-pos-deg
      aero/qbar-psf | metrics/Sw-sqft
      0   0
      10  0.20
      20  0.30
      30  0.35
    \</COEFFICIENT>
</pre>
    The first column in the data table represents the lookup index (or "key").  In
    this case, the lookup index is fcs/flap-pos-deg (flap extension in degrees).
    If the flap position is 10 degrees, the value returned from the lookup table
    would be 0.20.  This value would be multiplied by qbar (aero/qbar-psf) and wing
    area (metrics/Sw-sqft) to get the total lift force that is a result of flap
    deflection (measured in pounds force).  If the value of the flap-pos-deg property
    was 15 (degrees), the value output by the table routine would be 0.25 - an
    interpolation.  If the flap position in degrees ever went below 0.0, or above
    30 (degrees), the output from the table routine would be 0 and 0.35, respectively.
    That is, there is no _extrapolation_ to values outside the range of the lookup
    index.  This is why it is important to chose the data for the table wisely.

    The definition for a 2D table - referred to simply as a TABLE, is as follows:
<pre>
    \<COEFFICIENT NAME="{short name}" TYPE="TABLE">
      {name}
      {number of rows}
      {number of columns}
      {row lookup property}
      {column lookup property}
      {non-dimensionalizing}
                  {col_1_key   col_2_key   ...  col_n_key }
      {row_1_key} {col_1_data  col_2_data  ...  col_n_data}
      {row_2_key} {...         ...         ...  ...       }
      { ...     } {...         ...         ...  ...       }
      {row_n_key} {...         ...         ...  ...       }
    \</COEFFICIENT>
</pre>
    A "real life" example is as shown here:
<pre>
    \<COEFFICIENT NAME="CYb" TYPE="TABLE">
      Side_force_due_to_beta
      3
      2
      aero/beta-rad
      fcs/flap-pos-deg
      aero/qbar-psf | metrics/Sw-sqft
               0     30
      -0.349   0.137  0.106
       0       0      0
       0.349  -0.137 -0.106
    \</COEFFICIENT>
</pre>
    The definition for a 3D table in a coefficient would be (for example):
<pre>
    \<COEFFICIENT NAME="{short name}" TYPE="TABLE3D">
      {name}
      {number of rows}
      {number of columns}
      {number of tables}
      {row lookup property}
      {column lookup property}
      {table lookup property}
      {non-dimensionalizing}
      {first table key}
                  {col_1_key   col_2_key   ...  col_n_key }
      {row_1_key} {col_1_data  col_2_data  ...  col_n_data}
      {row_2_key} {...         ...         ...  ...       }
      { ...     } {...         ...         ...  ...       }
      {row_n_key} {...         ...         ...  ...       }

      {second table key}
                  {col_1_key   col_2_key   ...  col_n_key }
      {row_1_key} {col_1_data  col_2_data  ...  col_n_data}
      {row_2_key} {...         ...         ...  ...       }
      { ...     } {...         ...         ...  ...       }
      {row_n_key} {...         ...         ...  ...       }

      ...

    \</COEFFICIENT>
</pre>
    [At the present time, all rows and columns for each table must have the
    same dimension.]

    In addition to using a Table for something like a coefficient, where all the
    row and column elements are read in from a file, a Table could be created
    and populated completely within program code:
<pre>
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
</pre>
    The first column in the table, above, is thi (the lookup index, or key). The
    second column is the output data - in this case, "neta" (the Greek letter
    referring to combustion efficiency). Later on, the table is used like this:

    combustion_efficiency = Lookup_Combustion_Efficiency->GetValue(equivalence_ratio);

    @author Jon S. Berndt
    @version $Id: FGTable.h,v 1.19 2005/01/20 07:27:35 jberndt Exp $
    @see FGCoefficient
    @see FGPropeller
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTable : public FGJSBBase
{
public:
  /// Destructor
  ~FGTable();

  /** This is the very important copy constructor.
      @param table a const reference to a table.*/
  FGTable(const FGTable& table);

  /** The constructor for a VECTOR table
      @param nRows the number of rows in this VECTOR table. */
  FGTable(int nRows);
  FGTable(int nRows, int nCols);
  FGTable(int nRows, int nCols, int numTables);
  double GetValue(double key);
  double GetValue(double rowKey, double colKey);
  double GetValue(double rowKey, double colKey, double TableKey);
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

  void operator<<(FGConfigFile&);
  FGTable& operator<<(const double n);
  FGTable& operator<<(const int n);
  inline double GetElement(int r, int c) {return Data[r][c];}
  inline double GetElement(int r, int c, int t);
  void Print(int spaces=0);

private:
  enum type {tt1D, tt2D, tt3D} Type;
  double** Data;
  vector <FGTable> Tables;
  int nRows, nCols, nTables;
  int colCounter, rowCounter, tableCounter;
  int lastRowIndex, lastColumnIndex, lastTableIndex;
  double** Allocate(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif

