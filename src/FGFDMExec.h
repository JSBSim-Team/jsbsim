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

#include <models/FGModel.h>
#include <models/FGOutput.h>
#include <models/FGInput.h>
#include <initialization/FGTrim.h>
#include <initialization/FGInitialCondition.h>
#include <FGJSBBase.h>
#include <input_output/FGPropertyManager.h>
#include <input_output/FGXMLParse.h>
#include <input_output/FGGroundCallback.h>
#include <models/FGPropagate.h>

#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FDMEXEC "$Id: FGFDMExec.h,v 1.17 2006/01/23 11:29:56 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the JSBSim simulation executive.
    This class is the executive class through which all other simulation classes
    are instantiated, initialized, and run. When integrated with FlightGear (or
    other flight simulator) this class is typically instantiated by an interface
    class on the simulator side.

    At the time of simulation initialization, the interface
    class creates an instance of this executive class. The
    executive is subsequently directed to load the chosen aircraft specification
    file:

    @code
    fdmex = new FGFDMExec( … );
    result = fdmex->LoadModel( … );
    @endcode

    When an aircraft model is loaded, the config file is parsed and for each of the
    sections of the config file (propulsion, flight control, etc.) the
    corresponding Load() method is called (e.g. FGFCS::Load()).

    Subsequent to the creation of the executive and loading of the model,
    initialization is performed. Initialization involves copying control inputs
    into the appropriate JSBSim data storage locations, configuring it for the set
    of user supplied initial conditions, and then copying state variables from
    JSBSim. The state variables are used to drive the instrument displays and to
    place the vehicle model in world space for visual rendering:

    @code
    copy_to_JSBsim(); // copy control inputs to JSBSim
    fdmex->RunIC(); // loop JSBSim once w/o integrating
    copy_from_JSBsim(); // update the bus
    @endcode

    Once initialization is complete, cyclic execution proceeds:

    @code
    copy_to_JSBsim(); // copy control inputs to JSBSim
    fdmex->Run(); // execute JSBSim
    copy_from_JSBsim(); // update the bus
    @endcode

    JSBSim can be used in a standalone mode by creating a compact stub program
    that effectively performs the same progression of steps as outlined above for
    the integrated version, but with two exceptions. First, the copy_to_JSBSim()
    and copy_from_JSBSim() functions are not used because the control inputs are
    handled directly by the scripting facilities and outputs are handled by the
    output (data logging) class. Second, the name of a script file can be supplied
    to the stub program. Scripting (see FGScript) provides a way to supply command
    inputs to the simulation:

    @code
    FDMExec = new JSBSim::FGFDMExec();
    Script = new JSBSim::FGScript( … );
    Script->LoadScript( ScriptName ); // the script loads the aircraft and ICs
    result = FDMExec->Run();
    while (result) { // cyclic execution
      if (Scripted) if (!Script->RunScript()) break; // execute script
      result = FDMExec->Run(); // execute JSBSim
    }
    @endcode

    The standalone mode has been useful for verifying changes before committing
    updates to the source code repository. It is also useful for running sets of
    tests that reveal some aspects of simulated aircraft performance, such as
    range, time-to-climb, takeoff distance, etc.

    <h3>JSBSim Debugging Directives</h3>

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

    <h3>Properties</h3>
    @property simulator/do_trim (write only) Can be set to the integer equivalent to one of
                                tLongitudinal (0), tFull (1), tGround (2), tPullup (3),
                                tCustom (4), tTurn (5). Setting this to a legal value
                                (such as by a script) causes a trim to be performed. This
                                property actually maps toa function call of DoTrim().

    @author Jon S. Berndt
    @version $Revision: 1.17 $
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
      executed every 5th call to the exec's Run() method. Use of a rate other than
      one is at this time not recommended.
      @param model A pointer to the model being scheduled.
      @param rate The rate at which to execute the model as described above.
      @return Currently returns 0 always. */
  int  Schedule(FGModel* model, int rate);

  /** This function executes each scheduled model in succession.
      @return true if successful, false if sim should be ended  */
  bool Run(void);

  /** Initializes the sim from the initial condition object and executes
      each scheduled model without integrating i.e. dt=0.
      @return true if successful */
  bool RunIC(void);

  /** Sets the ground callback pointer.
      @param gc A pointer to a ground callback object.  */
  void SetGroundCallback(FGGroundCallback* gc);

  /** Loads an aircraft model.
      @param AircraftPath path to the aircraft/ directory. For instance:
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
      @return true if successful */
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
      files are kept, for instance "engine"  */
  bool SetEnginePath(string path)   { EnginePath = path; return true; }

  /** Sets the path to the aircraft config file directories.
      @param path path to the aircraft directory. For instance:
      "aircraft". Under aircraft, then, would be directories for various
      modeled aircraft such as C172/, x15/, etc.  */
  bool SetAircraftPath(string path) { AircraftPath = path; return true; }

  /// @name Top-level executive State and Model retrieval mechanism
  //@{
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
  inline FGPropagate* GetPropagate(void)      {return Propagate;}
  /// Returns the FGAuxiliary pointer.
  inline FGAuxiliary* GetAuxiliary(void)      {return Auxiliary;}
  /// Returns the FGInput pointer.
  inline FGInput* GetInput(void)              {return Input;}
  /// Returns the FGGroundCallback pointer.
  inline FGGroundCallback* GetGroundCallback(void) {return GroundCallback;}
  /// Returns the FGState pointer.
  inline FGState* GetState(void)              {return State;}
  // Returns a pointer to the FGInitialCondition object
  inline FGInitialCondition* GetIC(void)      {return IC;}
  // Returns a pointer to the FGTrim object
  inline FGTrim* GetTrim(void);
  //@}

  /// Retrieves the engine path.
  inline string GetEnginePath(void)          {return EnginePath;}
  /// Retrieves the aircraft path.
  inline string GetAircraftPath(void)        {return AircraftPath;}

  /// Returns the model name.
  string GetModelName(void) { return modelName; }

  /// Returns a pointer to the property manager object.
  FGPropertyManager* GetPropertyManager(void);
  /// Returns a vector of strings representing the names of all loaded models (future)
  vector <string> EnumerateFDMs(void);
  /// Marks this instance of the Exec object as a "slave" object.
  void SetSlave(void) {IsSlave = true;}

  /** Executes trimming in the selected mode.
  *   @param mode Specifies how to trim:
  * - tLongitudinal=0
  * - tFull
  * - tGround
  * - tPullup
  * - tCustom
  * - tTurn
  * - tNone  */
  void DoTrim(int mode);

  /// Disables data logging to all outputs.
  void DisableOutput(void);
  /// Enables data logging to all outputs.
  void EnableOutput(void);
  /// Pauses execution by preventing time from incrementing.
  void Hold(void) {holding = true;}
  /// Resumes execution from a "Hold".
  void Resume(void) {holding = false;}
  /// Returns true if the simulation is Holding (i.e. simulation time is not moving).
  bool Holding(void) {return holding;}

  struct PropertyCatalogStructure {
    /// Name of the property.
    string base_string;
    /// The node for the property.
    FGPropertyManager *node;
  };

  /** Builds a catalog of properties.
  *   This function descends the property tree and creates a list (an STL vector)
  *   containing the name and node for all properties.
  *   @param pcs The "root" property catalog structure pointer.  */
  void BuildPropertyCatalog(struct PropertyCatalogStructure* pcs);

  /** Retrieves property or properties matching the supplied string.
  *   A string is returned that contains a carriage return delimited list of all
  *   strings in the property catalog that matches the supplied check string.
  *   @param check The string to search for in the property catalog.
  *   @return the carriage-return-delimited string containing all matching strings
  *               in the catalog.  */
  string QueryPropertyCatalog(string check);

  /// Use the MSIS atmosphere model.
  void UseAtmosphereMSIS(void);

  /// Use the Mars atmosphere model. (Not operative yet.)
  void UseAtmosphereMars(void);

private:
  FGModel* FirstModel;

  bool terminate;
  bool holding;
  bool Constructing;
  int  Error;
  unsigned int Frame;
  unsigned int IdFDM;
  FGPropertyManager* Root;
  static unsigned int FDMctr;
  bool modelLoaded;
  string modelName;
  bool IsSlave;
  static FGPropertyManager *master;
  FGPropertyManager *instance;
  vector <string> PropertyCatalog;

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

  string CFGVersion;
  string Release;

  FGGroundCallback*  GroundCallback;
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
  FGInput*           Input;
  vector <FGOutput*> Outputs;

  FGInitialCondition* IC;
  FGTrim *Trim;

  vector <slaveData*> SlaveFDMList;

  bool ReadFileHeader(Element*);
  bool ReadSlave(Element*);
  bool ReadPrologue(Element*);

  bool Allocate(void);
  bool DeAllocate(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

