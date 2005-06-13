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
#include <math/FGFunction.h>
#include <math/FGColumnVector3.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AERODYNAMICS "$Id: FGAerodynamics.h,v 1.3 2005/06/13 16:59:17 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the aerodynamic calculations.
    This class owns and contains the list of coefficients that define the
    aerodynamic properties of this aircraft. Here also, such unique phenomena
    as ground effect and maximum lift curve tailoff are handled.
    @config
    <pre>
    \<AERODYNAMICS>
       \<AXIS NAME="{LIFT|DRAG|SIDE|ROLL|PITCH|YAW}">
         {Coefficient definitions}
       \</AXIS>
       {Additional axis definitions}
    \</AERODYNAMICS> </pre>

    @author Jon S. Berndt, Tony Peden
    $Id: FGAerodynamics.h,v 1.3 2005/06/13 16:59:17 ehofman Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAerodynamics : public FGModel {

public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGAerodynamics(FGFDMExec* Executive);
  /// Destructor
  ~FGAerodynamics();

  /** Runs the Aerodynamics model; called by the Executive
      @return false if no error */
  bool Run(void);

  /** Loads the Aerodynamics model.
      The Load function for this class expects the XML parser to
      have found the AERODYNAMICS keyword in the configuration file.
      @param element pointer to the current XML element for aerodynamics parameters.
      @return true if successful */
  bool Load(Element* element);

  /** Gets the total aerodynamic force vector.
      @return a force vector reference. */
  FGColumnVector3& GetForces(void) {return vForces;}

  /** Gets the aerodynamic force for an axis.
      @param n Axis index. This could be 0, 1, or 2, or one of the
               axis enums: eX, eY, eZ.
      @return the force acting on an axis */
  double GetForces(int n) const {return vForces(n);}

  /** Gets the total aerodynamic moment vector.
      @return a moment vector reference. */
  FGColumnVector3& GetMoments(void) {return vMoments;}

  /** Gets the aerodynamic moment for an axis.
      @return the moment about a single axis (as described also in the
              similar call to GetForces(int n).*/
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
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the descriptive names for all coefficients */
  string GetCoefficientStrings(string delimeter);

  /** Gets the coefficient values.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the numeric values for the current set of
      coefficients */
  string GetCoefficientValues(string delimeter);

  void bind(void);
  void unbind(void);

private:
  typedef map<string,int> AxisIndex;
  AxisIndex AxisIdx;
  vector <FGFunction*> variables;
  typedef vector <FGFunction*> CoeffArray;
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
  double clsq, lod, qbar_area;

  typedef double (FGAerodynamics::*PMF)(int) const;

  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

