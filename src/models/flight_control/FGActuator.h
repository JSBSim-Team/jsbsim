/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGActuator.h
 Author:       Jon Berndt
 Date started: 21 February 2007

 ------------- Copyright (C) 2006 Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGACTUATOR_H
#define FGACTUATOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates an Actuator component for the flight control system.
    The actuator can be modeled as a "perfect actuator", with the Output
    being set directly to the input. The actuator can be made more "real"
    by specifying any/all of the following additional effects that can be
    applied to the actuator. In order of application to the input signal,
    these are:
    
    - System lag (input lag, really)
    - Rate limiting
    - Deadband
    - Hysteresis (mechanical hysteresis)
    - Bias (mechanical bias)
    - Position limiting ("hard stops")
    
    There are also several malfunctions that can be applied to the actuator
    by setting a property to true or false (or 1 or 0).

    Rate limits can be specified either as a single number or property. If a
    single <rate_limit> is supplied (with no "sense" attribute) then the
    actuator is rate limited at +/- the specified rate limit. If the
    <rate_limit> element is supplied with a "sense" attribute of either
    "incr[easing]" or "decr[easing]" then the actuator is limited to the
    provided numeric or property value) exactly as provided.
    
    Lag filter's numerical integration assumes that the lag parameter is
    constant. So a continuously varying parameter via a property will introduce
    a non negligible error that will accumulate as the simulation progresses.

Syntax:

@code
<actuator name="name">
  <input> {[-]property} </input>
  <lag> {property name | value} </lag>
  [<rate_limit> {property name | value} </rate_limit>]
  [<rate_limit sense="incr"> {property name | value} </rate_limit>
   <rate_limit sense="decr"> {property name | value} </rate_limit>]
  <bias> number </bias>
  <deadband_width> number </deadband_width>
  <hysteresis_width> number </hysteresis_width>
  [<clipto>
    <min> {property name | value} </min>
    <max> {property name | value} </max>
   </clipto>]
  [<output> {property} </output>]
</actuator>
@endcode

Example:

@code
<actuator name="fcs/gimbal_pitch_position_radians">
  <input> fcs/gimbal_pitch_command </input>
  <lag> 60 </lag>
  <rate_limit> 0.085 </rate_limit> <!-- 0.085 radians/sec -->
  <bias> 0.002 </bias>
  <deadband_width> 0.002 </deadband_width>
  <hysteresis_width> 0.05 </hysteresis_width>
  <clipto> <!-- +/- 0.17 radians -->
    <min> -0.17 </min>
    <max>  0.17 </max>
   </clipto>
</actuator>
@endcode

@author Jon S. Berndt
@version $Revision: 1.20 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGActuator  : public FGFCSComponent
{
public:
  /// Constructor
  FGActuator(FGFCS* fcs, Element* element);
  /// Destructor
  ~FGActuator();

  /** This function processes the input.
      It calls private functions if needed to perform the hysteresis, lag,
      limiting, etc. functions. */
  bool Run (void) override;
  void ResetPastStates(void) override;

  // these may need to have the bool argument replaced with a double
  /** This function fails the actuator to zero. The motion to zero
      will flow through the lag, hysteresis, and rate limiting
      functions if those are activated. */
  void SetFailZero(bool set) {fail_zero = set;}
  void SetFailHardover(bool set) {fail_hardover = set;}
  void SetFailStuck(bool set) {fail_stuck = set;}

  bool GetFailZero(void) const {return fail_zero;}
  bool GetFailHardover(void) const {return fail_hardover;}
  bool GetFailStuck(void) const {return fail_stuck;}
  bool IsSaturated(void) const {return saturated;}
  
private:
  //double span;
  double bias;
  FGParameter* rate_limit_incr;
  FGParameter* rate_limit_decr;
  double hysteresis_width;
  double deadband_width;
  FGParameter* lag;
  double lagVal;
  double ca; // lag filter coefficient "a"
  double cb; // lag filter coefficient "b"
  double PreviousOutput;
  double PreviousHystOutput;
  double PreviousRateLimOutput;
  double PreviousLagInput;
  double PreviousLagOutput;
  bool fail_zero;
  bool fail_hardover;
  bool fail_stuck;
  bool initialized;
  bool saturated;

  void Hysteresis(void);
  void Lag(void);
  void RateLimit(void);
  void Deadband(void);
  void Bias(void);

  void bind(Element* el, FGPropertyManager* pm) override;

  void InitializeLagCoefficients();

  void Debug(int from) override;
};
}
#endif
