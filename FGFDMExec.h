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
#  ifdef FG_HAVE_STD_INCLUDES
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

#define ID_FDMEXEC "$Id: FGFDMExec.h,v 1.24 2001/03/02 16:35:37 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGState;
class FGAtmosphere;
class FGFCS;
class FGPropulsion;
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
    This class is the interface class through which all other simulation classes
    are instantiated, initialized, and run. When intesgrated with FlightGear (or
    another flight simulator) this class is typically instantiated by an interface
    class on the simulator side.
    @author Jon S. Berndt
    @version $Id: FGFDMExec.h,v 1.24 2001/03/02 16:35:37 jberndt Exp $
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
  ~FGFDMExec(void);

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
  
  inline string GetEnginePath(void)          {return EnginePath;}
  inline string GetAircraftPath(void)        {return AircraftPath;}

private:
  FGModel* FirstModel;

  bool frozen;
  bool terminate;
  int  Error;
  bool modelLoaded;
  bool Scripted;

  string AircraftPath;
  string EnginePath;
  string ScriptPath;
  string ScriptName;
  float  StartTime;
  float  EndTime;
  vector <struct condition> Conditions;

  FGState*       State;
  FGAtmosphere*  Atmosphere;
  FGFCS*         FCS;
  FGPropulsion*  Propulsion;
  FGAircraft*    Aircraft;
  FGTranslation* Translation;
  FGRotation*    Rotation;
  FGPosition*    Position;
  FGAuxiliary*   Auxiliary;
  FGOutput*      Output;
  
  bool Allocate(void);
  bool DeAllocate(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
