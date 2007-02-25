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

#define ID_ACTUATOR "$Id: FGActuator.h,v 1.2 2007/02/25 13:52:57 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates an Actuator component for the flight control system.

Syntax:

@code
<actuator name=”name”>
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
<actuator name=”fcs/gimbal_pitch_position”>
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
@version $Revision: 1.2 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGActuator  : public FGFCSComponent
{
public:
  FGActuator(FGFCS* fcs, Element* element);
  ~FGActuator();

  inline void Hysteresis(void);
  inline void Lag(void);
  inline void RateLimit(void);
  inline void Deadband(void);
  inline void Bias(void);

  bool Run (void);

  // these may need to have the bool argument replaced with a double
  inline void SetFailZero(bool set) {fail_zero = set;}
  inline void SetFailHardover(bool set) {fail_hardover = set;}
  inline void SetFailStuck(bool set) {fail_stuck = set;}

  inline bool GetFailZero(void) const {return fail_zero;}
  inline bool GetFailHardover(void) const {return fail_hardover;}
  inline bool GetFailStuck(void) const {return fail_stuck;}
  
private:
  enum eNoiseType {ePercent=0, eAbsolute} NoiseType;
  double dt;
  double min, max;
  double span;
  double bias;
  double rate_limit;
  double hysteresis_width;
  double deadband_width;
  double lag;
  double ca; /// lag filter coefficient "a"
  double cb; /// lag filter coefficient "b"
  double PreviousOutput;
  double PreviousInput;
  bool fail_zero;
  bool fail_hardover;
  bool fail_stuck;

  void bind(void);

  void Debug(int from);
};
}
#endif
