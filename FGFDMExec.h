/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Header:       FGFDMExec.h
 Author:       Jon Berndt
 Date started: 11/17/98

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
11/17/98   JSB   Created
7/31/99     TP   Added RunIC function that runs the sim so that every frame
                 begins with the IC values from the given FGInitialCondition
	  	  	  	   object and dt=0.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFDMEXEC_HEADER_H
#define FGFDMEXEC_HEADER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <vector>
#  else
#    include <vector.h>
#  endif
#else
#  include <vector>
#endif

#include "FGModel.h"
#include "FGInitialCondition.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FDMEXEC "$Id: FGFDMExec.h,v 1.40 2001/05/29 20:13:31 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGState;
class FGAtmosphere;
class FGFCS;
class FGPropulsion;
class FGMassBalance;
class FGAerodynamics;
class FGInertial;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPosition;
class FGAuxiliary;
class FGOutput;
class FGInitialCondition;

struct condition {
  vector <eParam>  TestParam;
  vector <eParam>  SetParam;
  vector <float>   TestValue;
  vector <float>   SetValue;
  vector <string>  Comparison;
  vector <float>   TC;
  vector <bool>    Persistent;
  vector <eAction> Action;
  vector <eType>   Type;
  vector <bool>    Triggered;
  vector <float>   newValue;
  vector <float>   OriginalValue;
  vector <float>   StartTime;
  vector <float>   EndTime;

  condition() {
  }
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the JSBSim simulation executive.
    @author Jon S. Berndt
    @version $Id: FGFDMExec.h,v 1.40 2001/05/29 20:13:31 jberndt Exp $

    @doc This class is the interface class through which all other simulation classes
    are instantiated, initialized, and run. When integrated with FlightGear (or
    other flight simulator) this class is typically instantiated by an interface
    class on the simulator side.

    <h4>Scripting support provided in the Executive</h4>

    <p>There is simple scripting support provided in the FGFDMExec
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

    <h4>JSBSim Debugging Directives</h4>

    This describes to any interested entity the debug level
    requested by setting the JSBSIM_DEBUG environment variable.
    The bitmasked value choices are as follows:<ol>
    <li><b>unset</b>: In this case (the default) JSBSim would only print
       out the normally expected messages, essentially echoing
       the config files as they are read. If the environment
       variable is not set, debug_lvl is set to 1 internally</li>
    <li><b>0</b>: This requests JSBSim not to output any messages
       whatsoever.</li>
    <li><b>1</b>: This value explicity requests the normal JSBSim
       startup messages</li>
    <li><b>2</b>: This value asks for a message to be printed out when
       a class is instantiated</li>
    <li><b>4</b>: When this value is set, a message is displayed when a
       FGModel object executes its Run() method</li>
    <li><b>8</b>: When this value is set, various runtime state variables
       are printed out periodically</li>
    <li><b>16</b>: When set various parameters are sanity checked and
       a message is printed out when they go out of bounds</li>
    </ol>

*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFDMExec
{
public:
  /// Default constructor
  FGFDMExec(void);

  /// Default destructor
  ~FGFDMExec();

  /** This routine places a model into the runlist at the specified rate. The
      "rate" is not really a clock rate. It represents how many calls to the
      FGFDMExec::Run() method must be made before the model is executed. A
      value of 1 means that the model will be executed for each call to the
      exec's Run() method. A value of 5 means that the model will only be
      executed every 5th call to the exec's Run() method.
      @param model A pointer to the model being scheduled.
      @param rate The rate at which to execute the model as described above.
      @return Currently returns 0 always. */
  int  Schedule(FGModel* model, int rate);

  /** This executes each scheduled model in succession, as well as running any
      scripts which are loaded.
      @return true if successful, false if sim should be ended  */
  bool Run(void);

  /** Initializes the sim with a set of initial conditions.
      @param fgic A pointer to a filled out initial conditions class which
      describes the desired initial conditions.
      @return true if successful
       */
  bool RunIC(FGInitialCondition *fgic);

  /// Freezes the sim
  void Freeze(void) {frozen = true;}

  /// Resumes the sim
  void Resume(void) {frozen = false;}

  /** Loads an aircraft model.
      @param AircraftPath path to the aircraft directory. For instance:
      "aircraft". Under aircraft, then, would be directories for various
      modeled aircraft such as C172/, x15/, etc.
      @param EnginePath path to the directory under which engine config
      files are kept, for instance "engine"
      @param model the name of the aircraft model itself. This file will
      be looked for in the directory specified in the AircraftPath variable,
      and in turn under the directory with the same name as the model. For
      instance: "aircraft/x15/x15.xml"
      @return true if successful*/
  bool LoadModel(string AircraftPath, string EnginePath, string model);

  /** Loads a script to drive JSBSim (usually in standalone mode).
      The language is the Simple Script Directives for JSBSim (SSDJ).
      @param script the filename (including path name, if any) for the script.
      @return true if successful */
  bool LoadScript(string script);

  /** This function is called each pass through the executive Run() method IF
      scripting is enabled. */
  void RunScript(void);

  bool SetEnginePath(string path)   {EnginePath = path; return true;}
  bool SetAircraftPath(string path) {AircraftPath = path; return true;}
  bool SetScriptPath(string path)   {ScriptPath = path; return true;}

  /// @name Top-level executive State and Model retrieval mechanism
  //@{
  /// Returns the FGState pointer.
  inline FGState* GetState(void)              {return State;}
  /// Returns the FGAtmosphere pointer.
  inline FGAtmosphere* GetAtmosphere(void)    {return Atmosphere;}
  /// Returns the FGFCS pointer.
  inline FGFCS* GetFCS(void)                  {return FCS;}
  /// Returns the FGPropulsion pointer.
  inline FGPropulsion* GetPropulsion(void)    {return Propulsion;}
  /// Returns the FGAircraft pointer.
  inline FGMassBalance* GetMassBalance(void)  {return MassBalance;}
  /// Returns the FGAerodynamics pointer
  inline FGAerodynamics* GetAerodynamics(void){return Aerodynamics;}
  /// Returns the FGInertial pointer.
  inline FGInertial* GetInertial(void)        {return Inertial;}
  /// Returns the FGAircraft pointer.
  inline FGAircraft* GetAircraft(void)        {return Aircraft;}
  /// Returns the FGTranslation pointer.
  inline FGTranslation* GetTranslation(void)  {return Translation;}
  /// Returns the FGRotation pointer.
  inline FGRotation* GetRotation(void)        {return Rotation;}
  /// Returns the FGPosition pointer.
  inline FGPosition* GetPosition(void)        {return Position;}
  /// Returns the FGAuxiliary pointer.
  inline FGAuxiliary* GetAuxiliary(void)      {return Auxiliary;}
  /// Returns the FGOutput pointer.
  inline FGOutput* GetOutput(void)            {return Output;}
  //@}

  /// Retrieves the engine path.
  inline string GetEnginePath(void)          {return EnginePath;}
  /// Retrieves the aircraft path.
  inline string GetAircraftPath(void)        {return AircraftPath;}

private:
  FGModel* FirstModel;

  bool frozen;
  bool terminate;
  int  Error;
  int  Frame;
  bool modelLoaded;
  bool Scripted;

  string AircraftPath;
  string EnginePath;
  string ScriptPath;
  string ScriptName;
  float  StartTime;
  float  EndTime;
  vector <struct condition> Conditions;

  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGPropulsion*   Propulsion;
  FGMassBalance*  MassBalance;
  FGAerodynamics* Aerodynamics;
  FGInertial*     Inertial;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGOutput*       Output;

  bool Allocate(void);
  bool DeAllocate(void);
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

