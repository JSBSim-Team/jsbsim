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
#include "FGPropertyManager.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_COEFFICIENT "$Id: FGCoefficient.h,v 1.46 2003/01/22 15:53:32 jberndt Exp $"

using std::vector;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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

/** This class models the aero coefficient and stability derivative coefficient
    lookup table, value, vector, or equation (equation not modeled, yet).
    Each coefficient for an axis is stored in that axes' vector of coefficients.
    Each FDM execution frame the Run() method of the FGAerodynamics model
    is called and the coefficient values are calculated.
    @author Jon S. Berndt
    @version $Id: FGCoefficient.h,v 1.46 2003/01/22 15:53:32 jberndt Exp $
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGCoefficient.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGCoefficient.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGCoefficient : public FGJSBBase
{
public:
  /** Constructor.
      @param exec a pointer to the FGFDMExec instance. */
  FGCoefficient(FGFDMExec* exec);
  /// Destructor.
  virtual ~FGCoefficient();
  
  /** Loads the stability derivative/aero coefficient data from the config file
      as directed by the FGAerodynamics instance.
      @param AC_cfg a pointer to the current config file instance. */
  virtual bool Load(FGConfigFile* AC_cfg);
  
  typedef vector <FGPropertyManager*> MultVec;

  enum Type {UNKNOWN, VALUE, VECTOR, TABLE, EQUATION};

  /** Returns the value for this coefficient.
      Each instance of FGCoefficient stores a value for the "type" of coefficient
      it is, one of: VALUE, VECTOR, TABLE, or EQUATION. This TotalValue function 
      is called when the value for a coefficient needs to be known. When it is called,
      depending on what type of coefficient is represented by the FGCoefficient
      instance, TotalValue() directs the appropriate Value() function to be called.
      The type of coefficient represented is determined when the config file is read.
      The coefficient definition includes the "type" specifier.
      @return the current value of the coefficient represented by this instance of
      FGCoefficient. */
  virtual double TotalValue(void);
  
  /** Returns the value for this coefficient.
      TotalValue is stored each time TotalValue() is called. This function returns
      the stored value but does not calculate it anew. This is valuable for merely
      printing out the value.
      @return the most recently calculated and stored value of the coefficient
      represented by this instance of FGCoefficient. */
  virtual inline double GetValue(void) const { return totalValue; }
  
  /// Returns the name of this coefficient.
  virtual inline string Getname(void) const {return name;}
  
  /// Returns the value of the coefficient only - before it is re-dimensionalized.
  virtual inline double GetSD(void) const { return SD;}
  
  /** Outputs coefficient information.
      Non-dimensionalizing parameter descriptions are output
      for each aero coefficient defined.
      @param multipliers the list of multipliers for this coefficient.*/
  virtual void DisplayCoeffFactors(void);
  
  /// Returns the name of the coefficient.
  virtual inline string GetCoefficientName(void) { return name; }
  /// Returns the stability derivative or coefficient value as a string.
  virtual string GetSDstring(void);
  
  
  inline void setBias(double b) { bias=b; }
  inline void setGain(double g) { gain=g; };
  inline double getBias(void) const { return bias; }
  inline double getGain(void) const { return gain; }
  
  virtual void bind(FGPropertyManager *parent);
  virtual void unbind(void);

protected:
  FGFDMExec* FDMExec;

private:
  int numInstances;
  string description;
  string name;
  string filename;
  string method;
  string multparms;
  string multparmsRow;
  string multparmsCol;
  double Value(double, double);
  double Value(double);
  double Value(void);
  double StaticValue;
  double totalValue;
  double bias,gain;
  FGPropertyManager *LookupR, *LookupC;
  
  FGPropertyManager *node; // must be private!!
  
  MultVec multipliers;
  int rows, columns;
  Type type;
  double SD; // Actual stability derivative (or other coefficient) value
  FGTable *Table;

  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGOutput*       Output;
  FGPropertyManager* PropertyManager;
  
  FGPropertyManager* resolveSymbol(string name);

  virtual void Debug(int from);
};

} // using namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

