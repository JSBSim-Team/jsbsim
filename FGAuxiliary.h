/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Header:       FGAuxiliary.h
 Author:       Jon Berndt
 Date started: 01/26/99
 
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
11/22/98   JSB   Created
  1/1/00   TP    Added calcs and getters for VTAS, VCAS, VEAS, Vground, in knots
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGAUXILIARY_H
#define FGAUXILIARY_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGMatrix.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AUXILIARY "$Header"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates various uncategorized scheduled functions.
    @author Tony Peden, Jon Berndt
    @version $Id: FGAuxiliary.h,v 1.16 2001/03/20 16:10:47 jberndt Exp $
    @see -
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAuxiliary : public FGModel {
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGAuxiliary(FGFDMExec*);
  /// Destructor
  ~FGAuxiliary();

  /** Runs the Auxiliary routines; called by the Executive
      @return false if no error */
  bool Run(void);

  // Use FGInitialCondition to set these speeds
  inline float GetVcalibratedFPS(void) { return vcas; }
  inline float GetVcalibratedKTS(void) { return vcas*FPSTOKTS; }
  inline float GetVequivalentFPS(void) { return veas; }
  inline float GetVequivalentKTS(void) { return veas*FPSTOKTS; }
  
  inline FGColumnVector GetPilotAccel(void) { return vPilotAccel; }
  inline FGColumnVector GetNpilot(void) { return vPilotAccel*INVGRAVITY; }
  
  inline float GetEarthPositionAngle(void) { return earthPosAngle; }
  
  float GetHeadWind(void);
  float GetCrossWind(void);
  
 
protected:

private:
  float vcas;
  float veas;
  float mach;
  float qbar,rhosl,rho,p,psl,pt;

  // Don't add a getter for pt!
  // pt above is freestream total pressure for subsonic only
  // for supersonic it is the 1D total pressure behind a normal shock
  // if a general freestream total is needed, e-mail Tony Peden
  // (apeden@earthlink.net) or you can add it your self using the
  // isentropic flow equations

  FGColumnVector vPilotAccel;
  
  float earthPosAngle;
  

  void GetState(void);
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

