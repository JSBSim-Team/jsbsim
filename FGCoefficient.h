/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGCoefficient.h
 Author:       Jon Berndt
 Date started: 12/28/98

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

HISTORY
--------------------------------------------------------------------------------
12/28/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGCOEFFICIENT_H
#define FGCOEFFICIENT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#endif

#include <vector>
#include <string>
#include "FGConfigFile.h"
#include "FGTable.h"
#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_COEFFICIENT "$Id: FGCoefficient.h,v 1.32 2001/11/12 05:06:27 jberndt Exp $"

using std::vector;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFDMExec;
class FGState;
class FGAtmosphere;
class FGFCS;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPosition;
class FGAuxiliary;
class FGOutput;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Note that the coefficients need not be calculated each delta-t. This is
something that may be fixed someday.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** This class models the stability derivative coefficient lookup tables.
    Each coefficient for an axis is stored in that axes' vector of coefficients.
    Each FDM execution frame the Run() method of the [currently] FGAircraft model
    is called and the coefficient value is calculated.
    @author Jon S. Berndt
    @version $Id: FGCoefficient.h,v 1.32 2001/11/12 05:06:27 jberndt Exp $
    @see -
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGCoefficient : public FGJSBBase
{
public:
  FGCoefficient(FGFDMExec*);
  virtual ~FGCoefficient();
  
  virtual bool Load(FGConfigFile* AC_cfg);
  
  typedef vector <eParam> MultVec;
  virtual float TotalValue(void);
  virtual inline string Getname(void) {return name;}
  virtual inline float GetSD(void) { return SD;}
  inline MultVec Getmultipliers(void) {return multipliers;}
  void DumpSD(void);  
  
  /** Outputs coefficient information.
      Non-dimensionalizing parameter descriptions are output
      for each aero coefficient defined.
      @param multipliers the list of multipliers for this coefficient.*/
  virtual void DisplayCoeffFactors(void);
  virtual inline string GetCoefficientStrings(void) { return name; }
  virtual string GetCoefficientValues(void);

private:
  enum Type {UNKNOWN, VALUE, VECTOR, TABLE, EQUATION};

  int numInstances;
  string filename;
  string description;
  string name;
  string method;
  float Value(float, float);
  float Value(float);
  float Value(void);
  float StaticValue;
  eParam LookupR, LookupC;
  MultVec multipliers;
  int rows, columns;
  Type type;
  float SD; // Actual stability derivative (or other coefficient) value
  FGTable *Table;

  FGFDMExec*      FDMExec;
  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGOutput*       Output;

  virtual void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

