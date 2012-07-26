/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAerodynamics.h
 Author:       Jon S. Berndt
 Date started: 09/13/00

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
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

#include <string>
#include <vector>
#include <map>

#include "FGModel.h"
#include "math/FGFunction.h"
#include "math/FGColumnVector3.h"
#include "math/FGMatrix33.h"
#include "input_output/FGXMLFileRead.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AERODYNAMICS "$Id: FGAerodynamics.h,v 1.26 2012/07/26 04:33:46 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the aerodynamic calculations.
    This class owns and contains the list of force/coefficients that define the
    aerodynamic properties of an aircraft. Here also, such unique phenomena
    as ground effect, aerodynamic reference point shift, and maximum lift curve
    tailoff are handled.

    @code
    <aerodynamics>
       <alphalimits unit="{RAD | DEG}">
         <min> {number} </min>
         <max> {number} </max>
       </alphalimits>
       <hysteresis_limits unit="{RAD | DEG}">
         <min> {number} </min>
         <max> {number} </max>
       </hysteresis_limits>
       <aero_ref_pt_shift_x>  
         <function>
           {function contents}
         </function> 
       </aero_ref_pt_shift_x>  
       <function>
         {function contents}
       </function>
       <axis name="{LIFT | DRAG | SIDE | ROLL | PITCH | YAW}">
         {force or moment definitions}
       </axis>
       {additional axis definitions}
    </aerodynamics>
    @endcode

    Optionally two other coordinate systems may be used.<br><br>
    1) Body coordinate system:
    @code
       <axis name="{X | Y | Z}">
    @endcode
    <br>
    2) Axial-Normal coordinate system:
    @code
       <axis name="{AXIAL | NORMAL | SIDE}">
    @endcode
    <br>
    Systems may NOT be combined, or a load error will occur.

    @author Jon S. Berndt, Tony Peden
    @version $Revision: 1.26 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAerodynamics : public FGModel, public FGXMLFileRead
{

public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGAerodynamics(FGFDMExec* Executive);
  /// Destructor
  ~FGAerodynamics();

  bool InitModel(void);

  /** Runs the Aerodynamics model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);

  /** Loads the Aerodynamics model.
      The Load function for this class expects the XML parser to
      have found the aerodynamics keyword in the configuration file.
      @param element pointer to the current XML element for aerodynamics parameters.
      @return true if successful */
  bool Load(Element* element);

  /** Gets the total aerodynamic force vector.
      @return a force vector reference. */
  const FGColumnVector3& GetForces(void) const {return vForces;}

  /** Gets the aerodynamic force for an axis.
      @param n Axis index. This could be 0, 1, or 2, or one of the
               axis enums: eX, eY, eZ.
      @return the force acting on an axis */
  double GetForces(int n) const {return vForces(n);}

  /** Gets the total aerodynamic moment vector about the CG.
      @return a moment vector reference. */
  const FGColumnVector3& GetMoments(void) const {return vMoments;}

  /** Gets the aerodynamic moment about the CG for an axis.
      @return the moment about a single axis (as described also in the
              similar call to GetForces(int n).*/
  double GetMoments(int n) const {return vMoments(n);}

  /** Gets the total aerodynamic moment vector about the Moment Reference Center.
      @return a moment vector reference. */
  const FGColumnVector3& GetMomentsMRC(void) const {return vMomentsMRC;}

  /** Gets the aerodynamic moment about the Moment Reference Center for an axis.
      @return the moment about a single axis (as described also in the
              similar call to GetForces(int n).*/
  double GetMomentsMRC(int n) const {return vMomentsMRC(n);}

  /** Retrieves the aerodynamic forces in the wind axes.
      @return a reference to a column vector containing the wind axis forces. */
  const FGColumnVector3& GetvFw(void) const { return vFw; }

  /** Retrieves the aerodynamic forces in the wind axes, given an axis.
      @param axis the axis to return the force for (eX, eY, eZ).
      @return a reference to a column vector containing the requested wind
      axis force. */
  double GetvFw(int axis) const { return vFw(axis); }

  /** Retrieves the lift over drag ratio */
  double GetLoD(void) const { return lod; }

  /** Retrieves the square of the lift coefficient. */
  double GetClSquared(void) const { return clsq; }
  double GetAlphaCLMax(void) const { return alphaclmax; }
  double GetAlphaCLMin(void) const { return alphaclmin; }

  double GetHysteresisParm(void) const { return stall_hyst; }
  double GetStallWarn(void) const { return impending_stall; }
  double GetAlphaW(void) const { return alphaw; }

  double GetBI2Vel(void) const { return bi2vel; }
  double GetCI2Vel(void) const { return ci2vel; }

  void SetAlphaCLMax(double tt) { alphaclmax=tt; }
  void SetAlphaCLMin(double tt) { alphaclmin=tt; }

  /** Gets the strings for the current set of aero functions.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the descriptive names for all aero functions */
  std::string GetAeroFunctionStrings(const std::string& delimeter) const;

  /** Gets the aero function values.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the numeric values for the current set of
      aero functions */
  std::string GetAeroFunctionValues(const std::string& delimeter) const;

  std::vector <FGFunction*> * GetAeroFunctions(void) const { return AeroFunctions; }

  struct Inputs {
    double Alpha;
    double Beta;
    double Vt;
    double Qbar;
    double Wingarea;
    double Wingspan;
    double Wingchord;
    double Wingincidence;
    FGColumnVector3 RPBody;
    FGMatrix33 Tb2w;
    FGMatrix33 Tw2b;
  } in;

private:
  enum eAxisType {atNone, atLiftDrag, atAxialNormal, atBodyXYZ} axisType;
  typedef std::map<std::string,int> AxisIndex;
  AxisIndex AxisIdx;
  FGFunction* AeroRPShift;
  typedef vector <FGFunction*> AeroFunctionArray;
  AeroFunctionArray* AeroFunctions;
  FGColumnVector3 vFnative;
  FGColumnVector3 vFw;
  FGColumnVector3 vForces;
  FGColumnVector3 vMoments;
  FGColumnVector3 vMomentsMRC;
  FGColumnVector3 vDXYZcg;
  FGColumnVector3 vDeltaRP;
  double alphaclmax, alphaclmin;
  double alphahystmax, alphahystmin;
  double impending_stall, stall_hyst;
  double bi2vel, ci2vel,alphaw;
  double clsq, lod, qbar_area;

  typedef double (FGAerodynamics::*PMF)(int) const;
  void DetermineAxisSystem(void);
  void bind(void);

  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

