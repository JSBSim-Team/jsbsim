/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Header:       FGScript.h
 Author:       Jon Berndt
 Date started: 12/21/2001

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

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
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FGSCRIPT "$Id: FGScript.h,v 1.2 2001/12/22 15:22:19 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the JSBSim scripting capability.
    @author Jon S. Berndt
    @version $Id: FGScript.h,v 1.2 2001/12/22 15:22:19 jberndt Exp $

    <h4>Scripting support provided via FGScript.</h4>

    <p>There is simple scripting support provided in the FGScript
    class. Commands are specified using the <u>Simple Scripting
    Directives for JSBSim</u> (SSDJ). The script file is in XML
    format. A test condition (or conditions) can be set up in the
    script and when the condition evaluates to true, the specified
    action[s] is/are taken. A test condition can be <em>persistent</em>,
    meaning that if a test condition evaluates to true, then passes
    and evaluates to false, the condition is reset and may again be
    triggered. When the set of tests evaluates to true for a given
    condition, an item may be set to another value. This value might
    be a boolean, a value, or a delta value, and the change from the
    current value to the new value can be either via a step function,
    a ramp, or an exponential approach. The speed of a ramp or
    approach is specified via the time constant. Here is the format
    of the script file:</p>

    <pre><strong>&lt;?xml version=&quot;1.0&quot;?&gt;
    &lt;runscript name=&quot;C172-01A&quot;&gt;

    &lt;!--
    This run is for testing C172 runs
    --&gt;

    &lt;use aircraft=&quot;c172&quot;&gt;
    &lt;use initialize=&quot;reset00&quot;&gt;

    &lt;run start=&quot;0.0&quot; end=&quot;4.5&quot; dt=&quot;0.05&quot;&gt;
      &lt;when&gt;
        &lt;parameter name=&quot;FG_TIME&quot; comparison=&quot;ge&quot; value=&quot;0.25&quot;&gt;
        &lt;parameter name=&quot;FG_TIME&quot; comparison=&quot;le&quot; value=&quot;0.50&quot;&gt;
        &lt;set name=&quot;FG_AILERON_CMD&quot; type=&quot;FG_VALUE&quot; value=&quot;0.25&quot;
        action=&quot;FG_STEP&quot; persistent=&quot;false&quot; tc =&quot;0.25&quot;&gt;
      &lt;/when&gt;
      &lt;when&gt;
        &lt;parameter name=&quot;FG_TIME&quot; comparison=&quot;ge&quot; value=&quot;0.5&quot;&gt;
        &lt;parameter name=&quot;FG_TIME&quot; comparison=&quot;le&quot; value=&quot;1.5&quot;&gt;
        &lt;set name=&quot;FG_AILERON_CMD&quot; type=&quot;FG_DELTA&quot; value=&quot;0.5&quot;
        action=&quot;FG_EXP&quot; persistent=&quot;false&quot; tc =&quot;0.5&quot;&gt;
      &lt;/when&gt;
      &lt;when&gt;
        &lt;parameter name=&quot;FG_TIME&quot; comparison=&quot;ge&quot; value=&quot;1.5&quot;&gt;
        &lt;parameter name=&quot;FG_TIME&quot; comparison=&quot;le&quot; value=&quot;2.5&quot;&gt;
        &lt;set name=&quot;FG_RUDDER_CMD&quot; type=&quot;FG_DELTA&quot; value=&quot;0.5&quot;
        action=&quot;FG_RAMP&quot; persistent=&quot;false&quot; tc =&quot;0.5&quot;&gt;
      &lt;/when&gt;
    &lt;/run&gt;

    &lt;/runscript&gt;</strong></pre>

    <p>The first line must always be present. The second line
    identifies this file as a script file, and gives a descriptive
    name to the script file. Comments are next, delineated by the
    &lt;!-- and --&gt; symbols. The aircraft and initialization files
    to be used are specified in the &quot;use&quot; lines. Next,
    comes the &quot;run&quot; section, where the conditions are
    described in &quot;when&quot; clauses.</p>

*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGScript : public FGJSBBase
{
public:
  /// Default constructor
  FGScript(FGFDMExec* exec);

  /// Default destructor
  ~FGScript();

  /** Loads a script to drive JSBSim (usually in standalone mode).
      The language is the Simple Script Directives for JSBSim (SSDJ).
      @param script the filename (including path name, if any) for the script.
      @return true if successful */
  bool LoadScript(string script);

  /** This function is called each pass through the executive Run() method IF
      scripting is enabled. 
      @return false if script should exit (i.e. if time limits are violated */
  bool RunScript(void);

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

  struct condition {
    vector <eParam>  TestParam;
    vector <eParam>  SetParam;
    vector <double>  TestValue;
    vector <double>  SetValue;
    vector <string>  Comparison;
    vector <double>  TC;
    vector <bool>    Persistent;
    vector <eAction> Action;
    vector <eType>   Type;
    vector <bool>    Triggered;
    vector <double>  newValue;
    vector <double>  OriginalValue;
    vector <double>  StartTime;
    vector <double>  EndTime;

    condition() {
    }
  };

  bool Scripted;

  string  ScriptName;
  double  StartTime;
  double  EndTime;
  vector <struct condition> Conditions;

  FGFDMExec* FDMExec;
  FGState* State;
  void Debug(int from);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

