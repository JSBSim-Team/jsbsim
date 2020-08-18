/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Header:       FGFDMExec.h
 Author:       Jon Berndt
 Date started: 11/17/98
 file The header file for the JSBSim executive.

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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

#include <memory>
#include <random>

#include "models/FGPropagate.h"
#include "models/FGOutput.h"
#include "math/FGTemplateFunc.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGScript;
class FGTrim;
class FGAerodynamics;
class FGAircraft;
class FGAtmosphere;
class FGAccelerations;
class FGWinds;
class FGAuxiliary;
class FGBuoyantForces;
class FGExternalReactions;
class FGGroundReactions;
class FGFCS;
class FGInertial;
class FGInput;
class FGPropulsion;
class FGMassBalance;
class FGTrim;

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

    @code{.cpp}
    fdmex = new FGFDMExec( ... );
    result = fdmex->LoadModel( ... );
    @endcode

    When an aircraft model is loaded, the config file is parsed and for each of
    the sections of the config file (propulsion, flight control, etc.) the
    corresponding Load() method is called (e.g. FGFCS::Load()).

    Subsequent to the creation of the executive and loading of the model,
    initialization is performed. Initialization involves copying control inputs
    into the appropriate JSBSim data storage locations, configuring it for the
    set of user supplied initial conditions, and then copying state variables
    from JSBSim. The state variables are used to drive the instrument displays
    and to place the vehicle model in world space for visual rendering:

    @code{.cpp}
    copy_to_JSBsim(); // copy control inputs to JSBSim
    fdmex->RunIC(); // loop JSBSim once w/o integrating
    copy_from_JSBsim(); // update the bus
    @endcode

    Once initialization is complete, cyclic execution proceeds:

    @code{.cpp}
    copy_to_JSBsim(); // copy control inputs to JSBSim
    fdmex->Run(); // execute JSBSim
    copy_from_JSBsim(); // update the bus
    @endcode

    JSBSim can be used in a standalone mode by creating a compact stub program
    that effectively performs the same progression of steps as outlined above
    for the integrated version, but with two exceptions. First, the
    copy_to_JSBSim() and copy_from_JSBSim() functions are not used because the
    control inputs are handled directly by the scripting facilities and outputs
    are handled by the output (data logging) class. Second, the name of a script
    file can be supplied to the stub program. Scripting (see FGScript) provides
    a way to supply command inputs to the simulation:

    @code{.cpp}
    FDMExec = new JSBSim::FGFDMExec();
    FDMExec->LoadScript( ScriptName ); // the script loads the aircraft and ICs
    result = FDMExec->Run();
    while (result) { // cyclic execution
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
    @version $Revision: 1.106 $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFDMExec : public FGJSBBase
{
  struct childData {
    FGFDMExec* exec;
    std::string info;
    FGColumnVector3 Loc;
    FGColumnVector3 Orient;
    bool mated;
    bool internal;

    childData(void) {
      info = "";
      Loc = FGColumnVector3(0,0,0);
      Orient = FGColumnVector3(0,0,0);
      mated = true;
      internal = false;
    }
    
    void Run(void) {exec->Run();}
    void AssignState(FGPropagate* source_prop) {
      exec->GetPropagate()->SetVState(source_prop->GetVState());
    }

    ~childData(void) {
      delete exec;
    }
  };

public:

  /// Default constructor
  FGFDMExec(FGPropertyManager* root = 0, unsigned int* fdmctr = 0);

  /// Default destructor
  ~FGFDMExec();

  // This list of enums is very important! The order in which models are listed
  // here determines the order of execution of the models.
  //
  // There are some conditions that need to be met :
  // 1. FCS can request mass geometry changes via the inertia/pointmass-*
  //    properties so it must be executed before MassBalance
  // 2. MassBalance must be executed before Propulsion, Aerodynamics,
  //    GroundReactions, ExternalReactions and BuoyantForces to ensure that
  //    their moments are computed with the updated CG position.
  enum eModels { ePropagate=0,
                 eInput,
                 eInertial,
                 eAtmosphere,
                 eWinds,
                 eSystems,
                 eMassBalance,
                 eAuxiliary,
                 ePropulsion,
                 eAerodynamics,
                 eGroundReactions,
                 eExternalReactions,
                 eBuoyantForces,
                 eAircraft,
                 eAccelerations,
                 eOutput,
                 eNumStandardModels };

  /** Unbind all tied JSBSim properties. */
  void Unbind(void) {instance->Unbind();}

  /** This function executes each scheduled model in succession.
      @return true if successful, false if sim should be ended  */
  bool Run(void);

  /** Initializes the sim from the initial condition object and executes
      each scheduled model without integrating i.e. dt=0.
      @return true if successful */
  bool RunIC(void);

  /** Loads an aircraft model.
      @param AircraftPath path to the aircraft/ directory. For instance:
      "aircraft". Under aircraft, then, would be directories for various
      modeled aircraft such as C172/, x15/, etc.
      @param EnginePath path to the directory under which engine config
      files are kept, for instance "engine"
      @param SystemsPath path to the directory under which systems config
      files are kept, for instance "systems"
      @param model the name of the aircraft model itself. This file will
      be looked for in the directory specified in the AircraftPath variable,
      and in turn under the directory with the same name as the model. For
      instance: "aircraft/x15/x15.xml"
      @param addModelToPath set to true to add the model name to the
      AircraftPath, defaults to true
      @return true if successful */
  bool LoadModel(const SGPath& AircraftPath, const SGPath& EnginePath,
                 const SGPath& SystemsPath, const std::string& model,
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
  bool LoadModel(const std::string& model, bool addModelToPath = true);

  /** Loads a script
      @param Script The full path name and file name for the script to be loaded.
      @param deltaT The simulation integration step size, if given.  If no value is supplied
                    then 0.0 is used and the value is expected to be supplied in
                    the script file itself.
      @param initfile The initialization file that will override the initialization file
                      specified in the script file. If no file name is given on the command line,
                      the file specified in the script will be used. If an initialization file 
                      is not given in either place, an error will result.
      @return true if successfully loads; false otherwise. */
  bool LoadScript(const SGPath& Script, double deltaT=0.0,
                  const SGPath& initfile=SGPath());

  /** Sets the path to the engine config file directories.
      @param path path to the directory under which engine config
      files are kept, for instance "engine"  */
  bool SetEnginePath(const SGPath& path) {
    EnginePath = GetFullPath(path);
    return true;
  }

  /** Sets the path to the aircraft config file directories.
      @param path path to the aircraft directory. For instance:
      "aircraft". Under aircraft, then, would be directories for various
      modeled aircraft such as C172/, x15/, etc.  */
  bool SetAircraftPath(const SGPath& path) {
    AircraftPath = GetFullPath(path);
    return true;
  }
  
  /** Sets the path to the systems config file directories.
      @param path path to the directory under which systems config
      files are kept, for instance "systems"  */
  bool SetSystemsPath(const SGPath& path) {
    SystemsPath = GetFullPath(path);
    return true;
  }
  
  /// @name Top-level executive State and Model retrieval mechanism
  ///@{
  /// Returns the FGAtmosphere pointer.
  FGAtmosphere* GetAtmosphere(void)    {return (FGAtmosphere*)Models[eAtmosphere];}
  /// Returns the FGAccelerations pointer.
  FGAccelerations* GetAccelerations(void)    {return (FGAccelerations*)Models[eAccelerations];}
  /// Returns the FGWinds pointer.
  FGWinds* GetWinds(void)    {return (FGWinds*)Models[eWinds];}
  /// Returns the FGFCS pointer.
  FGFCS* GetFCS(void)                  {return (FGFCS*)Models[eSystems];}
  /// Returns the FGPropulsion pointer.
  FGPropulsion* GetPropulsion(void)    {return (FGPropulsion*)Models[ePropulsion];}
  /// Returns the FGAircraft pointer.
  FGMassBalance* GetMassBalance(void)  {return (FGMassBalance*)Models[eMassBalance];}
  /// Returns the FGAerodynamics pointer
  FGAerodynamics* GetAerodynamics(void){return (FGAerodynamics*)Models[eAerodynamics];}
  /// Returns the FGInertial pointer.
  FGInertial* GetInertial(void)        {return (FGInertial*)Models[eInertial];}
  /// Returns the FGGroundReactions pointer.
  FGGroundReactions* GetGroundReactions(void) {return (FGGroundReactions*)Models[eGroundReactions];}
  /// Returns the FGExternalReactions pointer.
  FGExternalReactions* GetExternalReactions(void) {return (FGExternalReactions*)Models[eExternalReactions];}
  /// Returns the FGBuoyantForces pointer.
  FGBuoyantForces* GetBuoyantForces(void) {return (FGBuoyantForces*)Models[eBuoyantForces];}
  /// Returns the FGAircraft pointer.
  FGAircraft* GetAircraft(void)        {return (FGAircraft*)Models[eAircraft];}
  /// Returns the FGPropagate pointer.
  FGPropagate* GetPropagate(void)      {return (FGPropagate*)Models[ePropagate];}
  /// Returns the FGAuxiliary pointer.
  FGAuxiliary* GetAuxiliary(void)      {return (FGAuxiliary*)Models[eAuxiliary];}
  /// Returns the FGInput pointer.
  FGInput* GetInput(void)              {return (FGInput*)Models[eInput];}
  /// Returns the FGOutput pointer.
  FGOutput* GetOutput(void)            {return (FGOutput*)Models[eOutput];}
  /// Retrieves the script object
  FGScript* GetScript(void) {return Script;}
  /// Returns a pointer to the FGInitialCondition object
  FGInitialCondition* GetIC(void)      {return IC;}
  /// Returns a pointer to the FGTrim object
  FGTrim* GetTrim(void);
  ///@}

  /// Retrieves the engine path.
  const SGPath& GetEnginePath(void)    {return EnginePath;}
  /// Retrieves the aircraft path.
  const SGPath& GetAircraftPath(void)  {return AircraftPath;}
  /// Retrieves the systems path.
  const SGPath& GetSystemsPath(void)   {return SystemsPath;}
  /// Retrieves the full aircraft path name.
  const SGPath& GetFullAircraftPath(void) {return FullAircraftPath;}

  /** Retrieves the value of a property.
      @param property the name of the property
      @result the value of the specified property */
  inline double GetPropertyValue(const std::string& property)
  { return instance->GetNode()->GetDouble(property); }

  /** Sets a property value.
      @param property the property to be set
      @param value the value to set the property to */
  inline void SetPropertyValue(const std::string& property, double value) {
    instance->GetNode()->SetDouble(property, value);
  }

  /// Returns the model name.
  const std::string& GetModelName(void) const { return modelName; }

  /// Returns a pointer to the property manager object.
  FGPropertyManager* GetPropertyManager(void);
  /// Returns a vector of strings representing the names of all loaded models (future)
  std::vector <std::string> EnumerateFDMs(void);
  /// Gets the number of child FDMs.
  int GetFDMCount(void) const {return (int)ChildFDMList.size();}
  /// Gets a particular child FDM.
  childData* GetChildFDM(int i) const {return ChildFDMList[i];}
  /// Marks this instance of the Exec object as a "child" object.
  void SetChild(bool ch) {IsChild = ch;}

  /** Sets the output (logging) mechanism for this run.
      Calling this function passes the name of an output directives file to
      the FGOutput object associated with this run. The call to this function
      should be made prior to loading an aircraft model. This call results in an
      FGOutput object being built as the first Output object in the FDMExec-managed
      list of Output objects that may be created for an aircraft model. If this call
      is made after an aircraft model is loaded, there is no effect. Any Output
      objects added by the aircraft model itself (in an &lt;output> element) will be
      added after this one. Care should be taken not to refer to the same file
      name.
      An output directives file contains an &lt;output> &lt;/output> element, within
      which should be specified the parameters or parameter groups that should
      be logged.
      @param fname the filename of an output directives file.
    */
  bool SetOutputDirectives(const SGPath& fname)
  { return Output->SetDirectivesFile(GetFullPath(fname)); }

  /** Forces the specified output object to print its items once */
  void ForceOutput(int idx=0) { Output->ForceOutput(idx); }

  /** Sets the logging rate in Hz for all output objects (if any). */
  void SetLoggingRate(double rate) { Output->SetRateHz(rate); }

  /** Sets (or overrides) the output filename
      @param n index of file
      @param fname the name of the file to output data to
      @return true if successful, false if there is no output specified for the flight model */
  bool SetOutputFileName(const int n, const std::string& fname) { return Output->SetOutputName(n, fname); }

  /** Retrieves the current output filename.
      @param n index of file
      @return the name of the output file for the output specified by the flight model.
              If none is specified, the empty string is returned. */
  std::string GetOutputFileName(int n) const { return Output->GetOutputName(n); }

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
  void DisableOutput(void) { Output->Disable(); }
  /// Enables data logging to all outputs.
  void EnableOutput(void) { Output->Enable(); }
  /// Pauses execution by preventing time from incrementing.
  void Hold(void) {holding = true;}
  /// Turn on hold after increment
  void EnableIncrementThenHold(int Timesteps) {TimeStepsUntilHold = Timesteps; IncrementThenHolding = true;}
  /// Checks if required to hold afer increment
  void CheckIncrementalHold(void);
  /// Resumes execution from a "Hold".
  void Resume(void) {holding = false;}
  /// Returns true if the simulation is Holding (i.e. simulation time is not moving).
  bool Holding(void) {return holding;}
  /** Resets the initial conditions object and prepares the simulation to run
      again. If mode is set to 1 the output instances will take special actions
      such as closing the current output file and open a new one with a
      different name.
      @param mode Sets the reset mode.*/
  void ResetToInitialConditions(int mode);
  /// Sets the debug level.
  void SetDebugLevel(int level) {debug_lvl = level;}

  struct PropertyCatalogStructure {
    /// Name of the property.
    std::string base_string;
    /// The node for the property.
    FGPropertyNode_ptr node;
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
  std::string QueryPropertyCatalog(const std::string& check);

  // Print the contents of the property catalog for the loaded aircraft.
  void PrintPropertyCatalog(void);

  // Print the simulation configuration
  void PrintSimulationConfiguration(void) const;

  std::vector<std::string>& GetPropertyCatalog(void) {return PropertyCatalog;}

  void SetTrimStatus(bool status){ trim_status = status; }
  bool GetTrimStatus(void) const { return trim_status; }
  void SetTrimMode(int mode){ ta_mode = mode; }
  int GetTrimMode(void) const { return ta_mode; }

  std::string GetPropulsionTankReport();

  /// Returns the cumulative simulation time in seconds.
  double GetSimTime(void) const { return sim_time; }

  /// Returns the simulation delta T.
  double GetDeltaT(void) const {return dT;}

  /// Suspends the simulation and sets the delta T to zero.
  void SuspendIntegration(void) {saved_dT = dT; dT = 0.0;}

  /// Resumes the simulation by resetting delta T to the correct value.
  void ResumeIntegration(void)  {dT = saved_dT;}

  /** Returns the simulation suspension state.
      @return true if suspended, false if executing  */
  bool IntegrationSuspended(void) const {return dT == 0.0;}

  /** Sets the current sim time.
      @param cur_time the current time
      @return the current simulation time.      */
  double Setsim_time(double cur_time);

  /** Sets the integration time step for the simulation executive.
      @param delta_t the time step in seconds.     */
  void Setdt(double delta_t) { dT = delta_t; }

  /** Sets the root directory where JSBSim starts looking for its system
      directories.
      @param rootDir the string containing the root directory. */
  void SetRootDir(const SGPath& rootDir) {RootDir = rootDir;}

  /** Retrieves the Root Directory.
      @return the string representing the root (base) JSBSim directory. */
  const SGPath& GetRootDir(void) const {return RootDir;}

  /** Increments the simulation time if not in Holding mode. The Frame counter
      is also incremented.
      @return the new simulation time.     */
  double IncrTime(void);

  /** Retrieves the current frame count. */
  unsigned int GetFrame(void) const {return Frame;}

  /** Retrieves the current debug level setting. */
  int GetDebugLevel(void) const {return debug_lvl;};

  /** Initializes the simulation with initial conditions
      @param FGIC The initial conditions that will be passed to the simulation. */
  void Initialize(FGInitialCondition *FGIC);

  /** Sets the property forces/hold-down. This allows to do hard 'hold-down'
      such as for rockets on a launch pad with engines ignited.
      @param hd enables the 'hold-down' function if non-zero
  */
  void SetHoldDown(bool hd);

  /** Gets the value of the property forces/hold-down.
      @result zero if the 'hold-down' function is disabled, non-zero otherwise.
  */
  bool GetHoldDown(void) const {return HoldDown;}

  FGTemplateFunc* GetTemplateFunc(const std::string& name) {
    return TemplateFunctions.count(name) ? TemplateFunctions[name] : nullptr;
  }

  void AddTemplateFunc(const std::string& name, Element* el) {
    TemplateFunctions[name] = new FGTemplateFunc(this, el);
  }

  const std::shared_ptr<std::default_random_engine>& GetRandomEngine(void) const
  { return RandomEngine; }

private:
  unsigned int Frame;
  unsigned int IdFDM;
  int disperse;
  unsigned short Terminate;
  double dT;
  double saved_dT;
  double sim_time;
  bool holding;
  bool IncrementThenHolding;
  int TimeStepsUntilHold;
  bool Constructing;
  bool modelLoaded;
  bool IsChild;
  std::string modelName;
  SGPath AircraftPath;
  SGPath FullAircraftPath;
  SGPath EnginePath;
  SGPath SystemsPath;
  std::string CFGVersion;
  std::string Release;
  SGPath RootDir;

  // Standard Model pointers - shortcuts for internal executive use only.
  FGPropagate* Propagate;
  FGInertial* Inertial;
  FGAtmosphere* Atmosphere;
  FGWinds* Winds;
  FGAuxiliary* Auxiliary;
  FGFCS* FCS;
  FGPropulsion* Propulsion;
  FGAerodynamics* Aerodynamics;
  FGGroundReactions* GroundReactions;
  FGExternalReactions* ExternalReactions;
  FGBuoyantForces* BuoyantForces;
  FGMassBalance* MassBalance;
  FGAircraft* Aircraft;
  FGAccelerations* Accelerations;
  FGOutput* Output;

  bool trim_status;
  int ta_mode;
  unsigned int ResetMode;
  int trim_completed;

  FGScript*           Script;
  FGInitialCondition* IC;
  FGTrim*             Trim;

  FGPropertyManager* Root;
  bool StandAlone;
  FGPropertyManager* instance;

  bool HoldDown;

  int RandomSeed;
  std::shared_ptr<std::default_random_engine> RandomEngine;

  // The FDM counter is used to give each child FDM an unique ID. The root FDM
  // has the ID 0
  unsigned int*      FDMctr;

  std::vector <std::string> PropertyCatalog;
  std::vector <childData*> ChildFDMList;
  std::vector <FGModel*> Models;
  std::map<std::string, FGTemplateFunc_ptr> TemplateFunctions;

  bool ReadFileHeader(Element*);
  bool ReadChild(Element*);
  bool ReadPrologue(Element*);
  void SRand(int sr);
  int  SRand(void) const {return RandomSeed;}
  void LoadInputs(unsigned int idx);
  void LoadPlanetConstants(void);
  void LoadModelConstants(void);
  bool Allocate(void);
  bool DeAllocate(void);
  void InitializeModels(void);
  int GetDisperse(void) const {return disperse;}
  SGPath GetFullPath(const SGPath& name) {
    if (name.isRelative())
      return RootDir/name.utf8Str();
    else
      return name;
  }

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
