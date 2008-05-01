/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGActuator.h
 Author:       Jon Berndt
 Date started: 21 February 2007

 ------------- Copyright (C) 2006 Jon S. Berndt (jsb@hal-pc.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGACTUATOR_H
#define FGACTUATOR_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <input_output/FGXMLElement.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ACTUATOR "$Id: FGActuator.h,v 1.8 2008/05/01 01:03:14 dpculp Exp $"

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

Syntax:

@code
<actuator name="name">
  <input> {[-]property} </input>
  <lag> number </lag>
  <rate_limit> number <rate_limit>
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
<actuator name="fcs/gimbal_pitch_position">
  <input> fcs/gimbal_pitch_command </input>
  <lag> 60 </lag>
  <rate_limit> 0.085 <rate_limit> <!-- 5 degrees/sec -->
  <bias> 0.002 </bias>
  <deadband_width> 0.002 </deadband_width>
  <hysteresis_width> 0.05 </hysteresis_width>
  <clipto> <!-- +/- 10 degrees -->
    <min> -0.17 </min>
    <max>  0.17 </max>
   </clipto>
</actuator>
@endcode

@author Jon S. Berndt
@version $Revision: 1.8 $
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
  bool Run (void);

  // these may need to have the bool argument replaced with a double
  /** This function fails the actuator to zero. The motion to zero
      will flow through the lag, hysteresis, and rate limiting
      functions if those are activated. */
  inline void SetFailZero(bool set) {fail_zero = set;}
  inline void SetFailHardover(bool set) {fail_hardover = set;}
  inline void SetFailStuck(bool set) {fail_stuck = set;}

  inline bool GetFailZero(void) const {return fail_zero;}
  inline bool GetFailHardover(void) const {return fail_hardover;}
  inline bool GetFailStuck(void) const {return fail_stuck;}
  
private:
  double dt;
  double span;
  double bias;
  double rate_limit;
  double hysteresis_width;
  double deadband_width;
  double lag;
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

  void Hysteresis(void);
  void Lag(void);
  void RateLimit(void);
  void Deadband(void);
  void Bias(void);

  void bind(void);

  void Debug(int from);
};
}
#endif
