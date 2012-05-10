/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPID.h
 Author:       Jon Berndt
 Date started: 6/17/2006

 ------------- Copyright (C) 2006 by Jon S. Berndt, jon@jsbsim.org -------------

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
Initial Code 6/17/2006 JSB

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPID_H
#define FGPID_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PID "$Id: FGPID.h,v 1.13 2012/05/10 12:10:48 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;
class Element;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates a PID control component for the flight control system.

<h3>Configuration Format:</h3>

@code
<pid name="{string}" [type="standard"]>
  <kp> {number|property} </kp>
  <ki> {number|property} </ki>
  <kd> {number|property} </kd>
  <trigger> {property} </trigger>
</pid>
@endcode

For the integration constant element, one can also supply the type attribute for
what kind of integrator to be used, one of:

- rect, for a rectangular integrator
- trap, for a trapezoidal integrator
- ab2, for a second order Adams Bashforth integrator
- ab3, for a third order Adams Bashforth integrator

For example,

@code
<pid name="fcs/heading-control">
  <kp> 3 </kp>
  <ki type="ab3"> 1 </ki>
  <kd> 1 </kd>
</pid>
@endcode



<h3>Configuration Parameters:</h3>
<pre>

  The values of kp, ki, and kd have slightly different interpretations depending on
  whether the PID controller is a standard one, or an ideal/parallel one - with the latter
  being the default.
  
  kp      - Proportional constant, default value 0.
  ki      - Integrative constant, default value 0.
  kd      - Derivative constant, default value 0.
  trigger - Property which is used to sense wind-up, optional. Most often, the trigger
            will be driven by the "saturated" property of a particular actuator. When
            the relevant actuator has reached it's limits (if there are any, specified
            by the <clipto> element) the automatically generated saturated property will
            be greater than zero (true). If this property is used as the trigger for the
            integrator, the integrator will not continue to integrate while the property
            is still true (> 1), preventing wind-up.
  pvdot   - The property to be used as the process variable time derivative. 



</pre>

    @author Jon S. Berndt
    @version $Revision: 1.13 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPID  : public FGFCSComponent
{
public:
  FGPID(FGFCS* fcs, Element* element);
  ~FGPID();

  bool Run (void);
  void ResetPastStates(void) {Input_prev = Input_prev2 = Output = I_out_total = 0.0;}

    /// These define the indices use to select the various integrators.
  enum eIntegrateType {eNone = 0, eRectEuler, eTrapezoidal, eAdamsBashforth2, eAdamsBashforth3};

private:
  double Kp, Ki, Kd;
  double I_out_total;
  double Input_prev, Input_prev2;
  double KpPropertySign;
  double KiPropertySign;
  double KdPropertySign;

  bool IsStandard;

  eIntegrateType IntType;

  FGPropertyManager *Trigger;
  FGPropertyManager* KpPropertyNode;
  FGPropertyManager* KiPropertyNode;
  FGPropertyManager* KdPropertyNode;
  FGPropertyManager* ProcessVariableDot;

  void Debug(int from);
};
}
#endif
