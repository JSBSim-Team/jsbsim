/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAerodynamics.h
 Author:       Jon S. Berndt
 Date started: 09/13/00

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
09/13/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGAERODYNAMICS_H
#define FGAERODYNAMICS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <vector>
#    include <map>
#  else
#    include <vector.h>
#    include <map.h>
#  endif
#else
#  include <vector>
#  include <map>
#endif

#include "FGModel.h"
#include "FGConfigFile.h"
#include "FGState.h"
#include "FGMassBalance.h"
#include "FGTranslation.h"
#include "FGCoefficient.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AERODYNAMICS "$Id: FGAerodynamics.h,v 1.16 2001/06/26 00:21:31 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the aerodynamic calculations.
    This class owns and contains the list of coefficients that define the
    aerodynamic properties of this aircraft. Here also, such unique phenomena
    as ground effect and maximum lift curve tailoff are handled.
    @author Jon S. Berndt
    @version $Id: FGAerodynamics.h,v 1.16 2001/06/26 00:21:31 jberndt Exp $
    @see -
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAerodynamics : public FGModel {

public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGAerodynamics(FGFDMExec*);
  /// Destructor
  ~FGAerodynamics();

  /** Runs the Aerodynamics model; called by the Executive
      @return false if no error */
  bool Run(void);

  /** Loads the Aerodynamics model
      @return true if successful */
  bool Load(FGConfigFile* AC_cfg);

  /** Outputs coefficient information.
      Non-dimensionalizing parameter descriptions are output
      for each aero coefficient defined.
      @param multipliers the list of multipliers for this coefficient.*/
  void DisplayCoeffFactors(vector <eParam> multipliers);

  /** Gets the total aerodynamic force vector.
      @return a force vector reference. */
  FGColumnVector& GetForces(void) {return vForces;}

  /** Gets the total aerodynamic moment vector.
      @return a moment vector reference. */
  FGColumnVector& GetMoments(void) {return vMoments;}

  inline FGColumnVector GetvLastFs(void) { return vLastFs; }
  inline float GetvLastFs(int axis) { return vLastFs(axis); }
  inline FGColumnVector GetvFs(void) { return vFs; }
  inline float GetvFs(int axis) { return vFs(axis); }
  float GetLoD(void);

    /** Gets the strings for the current set of coefficients.
      @return a string containing the descriptive names for all coefficients */
  string GetCoefficientStrings(void);

  /** Gets the coefficient values.
      @return a string containing the numeric values for the current set of
      coefficients */
  string GetCoefficientValues(void);

  /// Gets the Normal Load Factor
  float GetNlf(void);

private:
  typedef map<string,int> AxisIndex;
  AxisIndex AxisIdx;
  typedef vector<FGCoefficient*> CoeffArray;
  CoeffArray* Coeff;
  FGColumnVector vFs;
  FGColumnVector vForces;
  FGColumnVector vMoments;
  FGColumnVector vLastFs;
  FGColumnVector vDXYZcg;

  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

