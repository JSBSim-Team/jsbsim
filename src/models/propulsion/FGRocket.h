/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGRocket.h
 Author:       Jon S. Berndt
 Date started: 09/12/2000

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) --------------

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
#include <input_output/FGXMLElement.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ROCKET "$Id: FGRocket.h,v 1.4 2006/08/30 12:04:38 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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
    location are required, as with all other engine types:
    <ul>
        <li>Throttle setting (in percent, from 0 to 1.0)</li>
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
    $Id: FGRocket.h,v 1.4 2006/08/30 12:04:38 jberndt Exp $
    @see FGNozzle,
    FGThruster,
    FGForce,
    FGEngine,
    FGPropulsion,
    FGTank
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGRocket : public FGEngine
{
public:
  /** Constructor.
      @param exec pointer to JSBSim parent object, the FDM Executive.
      @param el a pointer to the XML Element instance representing the engine.
      @param engine_number engine number */
  FGRocket(FGFDMExec* exec, Element *el, int engine_number);

  /** Destructor */
  ~FGRocket(void);

  /** Determines the thrust coefficient.
      @return thrust coefficient times chamber pressure */
  double Calculate(void);

  /** Gets the chamber pressure.
      @return chamber pressure in psf. */
  double GetChamberPressure(void) {return PC;}

  /** Gets the flame-out status.
      The engine will "flame out" if the throttle is set below the minimum
      sustainable setting.
      @return true if engine has flamed out. */
  bool GetFlameout(void) {return Flameout;}
  string GetEngineLabels(string delimeter);
  string GetEngineValues(string delimeter);

private:
  double SHR;
  double maxPC;
  double propEff;
  double kFactor;
  double Variance;
  double PC;
  bool Flameout;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

