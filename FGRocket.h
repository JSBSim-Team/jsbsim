/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGRocket.h
 Author:       Jon S. Berndt
 Date started: 09/12/2000

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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
09/12/2000  JSB  Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGROCKET_H
#define FGROCKET_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGEngine.h"
#include "FGConfigFile.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ROCKET "$Id: FGRocket.h,v 1.21 2001/12/10 23:34:58 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a generic rocket engine.
    The rocket engine is modeled given the following parameters:
    <ul>
        <li>Chamber pressure (in psf)</li>
        <li>Specific heat ratio (usually about 1.2 for hydrocarbon fuel and LOX)</li>
        <li>Propulsive efficiency (in percent, from 0 to 1.0)</li>
        <li>Variance (in percent, from 0 to 1.0, nominally 0.05)</li>
    </ul>
    Additionally, the following control inputs, operating characteristics, and
    location are required, as with all other engine types:</font>
    <ul>
        <li>Throttle setting (in percent, from 0 to 1.0)</font></li>
        <li>Maximum allowable throttle setting</li>
        <li>Minimum working throttle setting</li>
        <li>Sea level fuel flow at maximum thrust</li>
        <li>Sea level oxidizer flow at maximum thrust</li>
        <li>X, Y, Z location in structural coordinate frame</li>
        <li>Pitch and Yaw</li>
    </ul>
    The nozzle exit pressure (p2) is returned via a
    call to FGNozzle::GetPowerRequired(). This exit pressure is used,
    along with chamber pressure and specific heat ratio, to get the
    thrust coefficient for the throttle setting. This thrust
    coefficient is multiplied by the chamber pressure and then passed
    to the nozzle Calculate() routine, where the thrust force is
    determined.

    @author Jon S. Berndt
    @version $Id: FGRocket.h,v 1.21 2001/12/10 23:34:58 jberndt Exp $
    @see FGNozzle
    @see FGThruster
    @see FGForce
    @see FGEngine
    @see FGPropulsion
    @see FGTank
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGRocket : public FGEngine
{
public:
  /** Constructor.
      @param exec pointer to JSBSim parent object, the FDM Executive.
      @param Eng_cfg pointer to the config file object. */
  FGRocket(FGFDMExec* exec, FGConfigFile* Eng_cfg);

  /** Destructor */
  ~FGRocket();

  /** Determines the thrust coefficient.
      This routine takes the nozzle exit pressure and calculates the thrust
      coefficient times the chamber pressure.
      @param pe nozzle exit pressure
      @return thrust coefficient times chamber pressure */
  double Calculate(double pe);
  
  /** Gets the chamber pressure.
      @return chamber pressure in psf. */
  double GetChamberPressure(void) {return PC;}

private:
  double SHR;
  double maxPC;
  double propEff;
  double kFactor;
  double Variance;
  double PC;
  void Debug(int from);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

