/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Header:       FGScript.h
 Author:       Jon Berndt
 Date started: 12/21/2001

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

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
12/21/01   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSCRIPT_HEADER_H
#define FGSCRIPT_HEADER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include <math/FGFunction.h>
#include <math/FGCondition.h>
#include <vector>
#include <input_output/FGXMLFileRead.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FGSCRIPT "$Id: FGScript.h,v 1.14 2009/03/25 12:02:49 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the JSBSim scripting capability.
    <h4>Scripting support provided via FGScript.</h4>

    <p>There is support for scripting provided in the FGScript
    class. Commands are specified using the <em>Scripting
    Directives for JSBSim</em>. The script file is in XML
    format. A test condition (or conditions) can be set up in an event in a
    script and when the condition evaluates to true, the specified
    action[s] is/are taken. An event can be <em>persistent</em>,
    meaning that at every time the test condition first evaluates to true
    (toggling from false to true) then the specified <em>set</em> actions take
    place. An event can also be defined to execute or evaluate continuously
    while the condition is true. When the set of tests evaluates to true for a given
    condition, an item may be set to another value. This value may
    be a value, or a delta value, and the change from the
    current value to the new value can be either via a step action,
    a ramp, or an exponential approach. The speed of a ramp or exponential
    approach is specified via the time constant. Here is an example
    illustrating the format of the script file:

    @code
<?xml version="1.0"?>
<runscript name="C172-01A takeoff run">
  <!--
    This run is for testing the C172 altitude hold autopilot
  -->

  <use aircraft="c172x"/>
  <use initialize="reset00"/>
  <run start="0.0" end="3000" dt="0.0083333">

    <event name="engine start">
      <notify/>
      <condition>
        sim-time-sec >= 0.25
      </condition>
      <set name="fcs/throttle-cmd-norm" value="1.0" action="FG_RAMP" tc ="0.5"/>
      <set name="fcs/mixture-cmd-norm" value="0.87" action="FG_RAMP" tc ="0.5"/>
      <set name="propulsion/magneto_cmd" value="3"/>
      <set name="propulsion/starter_cmd" value="1"/>
    </event>

    <event name="set heading hold">
      <!-- Set Heading when reach 5 ft -->
      <notify/>
      <condition>
        position/h-agl-ft >= 5
      </condition>
      <set name="ap/heading_setpoint" value="200"/>
      <set name="ap/attitude_hold" value="0"/>
      <set name="ap/heading_hold" value="1"/>
    </event>

    <event name="set autopilot">
      <!-- Set Autopilot for 20 ft -->
      <notify/>
      <condition>
        aero/qbar-psf >= 4
      </condition>
      <set name="ap/altitude_setpoint" value="100.0" action="FG_EXP" tc ="2.0"/>
      <set name="ap/altitude_hold" value="1"/>
      <set name="fcs/flap-cmd-norm" value=".33"/>
    </event>

    <event name="set autopilot 2" persistent="true">
      <!-- Set Autopilot for 6000 ft -->
      <notify/>
      <condition>
        aero/qbar-psf > 5
      </condition>
      <set name="ap/altitude_setpoint" value="6000.0"/>
    </event>

    <event name="Time Notify">
      <notify/>
      <condition> sim-time-sec >= 500 </condition>
    </event>

    <event name="Time Notify">
      <notify/>
      <condition> sim-time-sec >= 1000 </condition>
    </event>

  </run>

</runscript>
    @endcode

    The first line must always be present - it identifies the file
    as an XML format file. The second line
    identifies this file as a script file, and gives a descriptive
    name to the script file. Comments are next, delineated by the
    &lt;!-- and --&gt; symbols. The aircraft and initialization files
    to be used are specified in the &quot;use&quot; lines. Next,
    comes the &quot;run&quot; section, where the conditions are
    described in &quot;event&quot; clauses.</p>
    @author Jon S. Berndt
    @version "$Id: FGScript.h,v 1.14 2009/03/25 12:02:49 jberndt Exp $"
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGScript : public FGJSBBase, public FGXMLFileRead
{
public:
  /// Default constructor
  FGScript(FGFDMExec* exec);

  /// Default destructor
  ~FGScript();

  /** Loads a script to drive JSBSim (usually in standalone mode).
      The language is the Script Directives for JSBSim.
      @param script the filename (including path name, if any) for the script.
      @return true if successful */
  bool LoadScript( string script );

  /** This function is called each pass through the executive Run() method IF
      scripting is enabled.
      @return false if script should exit (i.e. if time limits are violated */
  bool RunScript(void);

  void ResetEvents(void) {
    for (unsigned int i=0; i<Events.size(); i++) Events[i].reset();
  }

private:
  enum eAction {
    FG_RAMP  = 1,
    FG_STEP  = 2,
    FG_EXP   = 3
  };

  enum eType {
    FG_VALUE = 1,
    FG_DELTA = 2,
    FG_BOOL  = 3
  };

  struct event {
    FGCondition     *Condition;
    bool             Persistent;
    bool             Continuous;
    bool             Triggered;
    bool             Notify;
    bool             Notified;
    double           Delay;
    double           StartTime;
    double           TimeSpan;
    string           Name;
    vector <FGPropertyManager*>  SetParam;
    vector <FGPropertyManager*>  NotifyProperties;
    vector <eAction> Action;
    vector <eType>   Type;
    vector <double>  SetValue;
    vector <double>  TC;
    vector <double>  newValue;
    vector <double>  OriginalValue;
    vector <double>  ValueSpan;
    vector <bool>    Transiting;
    vector <FGFunction*> Functions;

    event() {
      Triggered = false;
      Persistent = false;
      Continuous = false;
      Delay = 0.0;
      Notify = Notified = false;
      Name = "";
      StartTime = 0.0;
      TimeSpan = 0.0;
    }

    void reset(void) {
      Triggered = false;
      Notified = false;
      StartTime = 0.0;
    }
  };

  struct LocalProps {
    double *value;
    string title;
    LocalProps(double initial_value=0) {
      value = new double(initial_value);
      title = "";
    }
  };

  string  ScriptName;
  double  StartTime;
  double  EndTime;
  vector <struct event> Events;
  vector <LocalProps*> local_properties;

  FGFDMExec* FDMExec;
  FGState* State;
  FGPropertyManager* PropertyManager;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

