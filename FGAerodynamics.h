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
#include "FGFactorGroup.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AERODYNAMICS "$Id: FGAerodynamics.h,v 1.36 2003/06/03 09:53:40 ehofman Exp $"


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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
    @version $Id: FGAerodynamics.h,v 1.36 2003/06/03 09:53:40 ehofman Exp $
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGAerodynamics.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGAerodynamics.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
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

  /** Gets the total aerodynamic force vector.
      @return a force vector reference. */
  FGColumnVector3& GetForces(void) {return vForces;}
  double GetForces(int n) const {return vForces(n);}

  /** Gets the total aerodynamic moment vector.
      @return a moment vector reference. */
  FGColumnVector3& GetMoments(void) {return vMoments;}
  double GetMoments(int n) const {return vMoments(n);}

  FGColumnVector3& GetvLastFs(void) { return vLastFs; }
  double GetvLastFs(int axis) const { return vLastFs(axis); }
  FGColumnVector3& GetvFs(void) { return vFs; }
  double GetvFs(int axis) const { return vFs(axis); }
  inline double GetLoD(void) const { return lod; }
  inline double GetClSquared(void) const { return clsq; } 
  inline double GetAlphaCLMax(void) const { return alphaclmax; }
  inline double GetAlphaCLMin(void) const { return alphaclmin; }
  
  inline double GetAlphaHystMax(void) const { return alphahystmax; }
  inline double GetAlphaHystMin(void) const { return alphahystmin; }
  inline double GetHysteresisParm(void) const { return stall_hyst; }
  inline double GetStallWarn(void) const { return impending_stall; }
  double GetAlphaW(void) const { return alphaw; }

  double GetBI2Vel(void) const { return bi2vel; }
  double GetCI2Vel(void) const { return ci2vel; }
  
  inline void SetAlphaCLMax(double tt) { alphaclmax=tt; }
  inline void SetAlphaCLMin(double tt) { alphaclmin=tt; }

    /** Gets the strings for the current set of coefficients.
      @return a string containing the descriptive names for all coefficients */
  string GetCoefficientStrings(void);

  /** Gets the coefficient values.
      @return a string containing the numeric values for the current set of
      coefficients */
  string GetCoefficientValues(void);
  
  void bind(void);
  void bindModel(void);
  void unbind(void);
  
private:
  typedef map<string,int> AxisIndex;
  AxisIndex AxisIdx;
  typedef vector<FGCoefficient*> CoeffArray;
  CoeffArray* Coeff;
  FGColumnVector3 vFs;
  FGColumnVector3 vForces;
  FGColumnVector3 vMoments;
  FGColumnVector3 vLastFs;
  FGColumnVector3 vDXYZcg;
  double alphaclmax, alphaclmin;
  double alphahystmax, alphahystmin;
  double impending_stall, stall_hyst;
  double bi2vel, ci2vel,alphaw;
  double clsq,lod;
  
  typedef double (FGAerodynamics::*PMF)(int) const;

  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

