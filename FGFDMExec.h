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

#include "FGModel.h"
#include "FGTrim.h"
#include "FGInitialCondition.h"
#include "FGJSBBase.h"
#include "FGPropertyManager.h"

#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FDMEXEC "$Id: FGFDMExec.h,v 1.71 2004/06/27 08:35:32 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the JSBSim simulation executive.
    This class is the interface class through which all other simulation classes
    are instantiated, initialized, and run. When integrated with FlightGear (or
    other flight simulator) this class is typically instantiated by an interface
    class on the simulator side.

    When an aircraft model is loaded the config file is parsed and for each of the
    sections of the config file (propulsion, flight control, etc.) the
    corresponding "ReadXXX()" method is called. From within this method the
    "Load()" method of that system is called (e.g. LoadFCS).

    <h4>JSBSim Debugging Directives</h4>

    This describes to any interested entity the debug level
    requested by setting the JSBSIM_DEBUG environment variable.
    The bitmasked value choices are as follows:
    - <b>unset</b>: In this case (the default) JSBSim would only print
       out the normally expected messages, essentially echoing
       the config files as they are read. If the environment
       variable is not set, debug_lvl is set to 1 internally
    - <b>0</b>: This requests JSBSim not to output any messages
       whatsoever
    - <b>1</b>: This value explicity requests the normal JSBSim
       startup messages
    - <b>2</b>: This value asks for a message to be printed out when
       a class is instantiated
    - <b>4</b>: When this value is set, a message is displayed when a
       FGModel object executes its Run() method
    - <b>8</b>: When this value is set, various runtime state variables
       are printed out periodically
    - <b>16</b>: When set various parameters are sanity checked and
       a message is printed out when they go out of bounds

    @author Jon S. Berndt
    @version $Id: FGFDMExec.h,v 1.71 2004/06/27 08:35:32 ehofman Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFDMExec : public FGJSBBase
{
public:

  /// Default constructor
  FGFDMExec(FGPropertyManager* root = 0);

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

  /** This executes each scheduled model in succession.
      @return true if successful, false if sim should be ended  */
  bool Run(void);

  /** Initializes the sim from the initial condition object and executes
      each scheduled model without integrating i.e. dt=0.
      @return true if successful
       */
  bool RunIC(void);

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
      @param addModelToPath set to true to add the model name to the
      AircraftPath, defaults to true
      @return true if successful*/
  bool LoadModel(string AircraftPath, string EnginePath, string model,
                 bool addModelToPath = true);


  /** Loads an aircraft model.  The paths to the aircraft and engine
      config file directories must be set prior to calling this.  See
      below.
      @param model the name of the aircraft model itself. This file will
      be looked for in the directory specified in the AircraftPath variable,
      and in turn under the directory with the same name as the model. For
      instance: "aircraft/x15/x15.xml"
      @param addModelToPath set to true to add the model name to the
      AircraftPath, defaults to true
      @return true if successful*/
  bool LoadModel(string model, bool addModelToPath = true);


  /** Sets the path to the engine config file directories.
      @param path path to the directory under which engine config
      files are kept, for instance "engine"
  */
  bool SetEnginePath(string path)   { EnginePath = path; return true; }

  /** Sets the path to the aircraft config file directories.
      @param path path to the aircraft directory. For instance:
      "aircraft". Under aircraft, then, would be directories for various
      modeled aircraft such as C172/, x15/, etc.
  */
  bool SetAircraftPath(string path) { AircraftPath = path; return true; }

  /** Sets the path to the autopilot config file directories.
      @param path path to the control directory. For instance:
      "control".
  */
//  bool SetControlPath(string path) { ControlPath = path; return true; }


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
  /// Returns the FGGroundReactions pointer.
  inline FGGroundReactions* GetGroundReactions(void) {return GroundReactions;}
  /// Returns the FGAircraft pointer.
  inline FGAircraft* GetAircraft(void)        {return Aircraft;}
  /// Returns the FGPropagate pointer.
  inline FGPropagate* GetPropagate(void)        {return Propagate;}
  /// Returns the FGAuxiliary pointer.
  inline FGAuxiliary* GetAuxiliary(void)      {return Auxiliary;}
  /// Returns the FGOutput pointer.
  inline FGOutput* GetOutput(void)            {return Output;}
  // Returns a pointer to the FGInitialCondition object
  inline FGInitialCondition* GetIC(void)      {return IC;}
  // Returns a pointer to the FGTrim object
  FGTrim* GetTrim(void);
  //@}

  /// Retrieves the engine path.
  inline string GetEnginePath(void)          {return EnginePath;}
  /// Retrieves the aircraft path.
  inline string GetAircraftPath(void)        {return AircraftPath;}
//  /// Retrieves the control path.
//  inline string GetControlPath(void)        {return ControlPath;}

  string GetModelName(void) { return modelName; }

  FGPropertyManager* GetPropertyManager(void);
  vector <string> EnumerateFDMs(void);
  void SetSlave(void) {IsSlave = true;}

private:
  FGModel* FirstModel;

  bool frozen;
  bool terminate;
  int  Error;
  unsigned int Frame;
  unsigned int IdFDM;
  static unsigned int FDMctr;
  bool modelLoaded;
  string modelName;
  bool IsSlave;
  static FGPropertyManager *master;
  FGPropertyManager *instance;

  struct slaveData {
    FGFDMExec* exec;
    string info;
    double x, y, z;
    double roll, pitch, yaw;
    bool mated;

    slaveData(void) {
      info = "";
      x = y = z = 0.0;
      roll = pitch = yaw = 0.0;
      mated = true;
    }

    ~slaveData(void) {
      delete exec;
    }
  };

  string AircraftPath;
  string EnginePath;
//  string ControlPath;

  string CFGVersion;
  string Release;

  FGState*           State;
  FGAtmosphere*      Atmosphere;
  FGFCS*             FCS;
  FGPropulsion*      Propulsion;
  FGMassBalance*     MassBalance;
  FGAerodynamics*    Aerodynamics;
  FGInertial*        Inertial;
  FGGroundReactions* GroundReactions;
  FGAircraft*        Aircraft;
  FGPropagate*       Propagate;
  FGAuxiliary*       Auxiliary;
  FGOutput*          Output;

  FGInitialCondition* IC;
  FGTrim *Trim;

  vector <slaveData*> SlaveFDMList;

  bool ReadMetrics(FGConfigFile*);
  bool ReadSlave(FGConfigFile*);
  bool ReadPropulsion(FGConfigFile*);
  bool ReadFlightControls(FGConfigFile*);
  bool ReadAerodynamics(FGConfigFile*);
  bool ReadUndercarriage(FGConfigFile*);
  bool ReadPrologue(FGConfigFile*);
  bool ReadOutput(FGConfigFile*);

  bool Allocate(void);
  bool DeAllocate(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

