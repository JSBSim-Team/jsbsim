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
#include "FGInitialCondition.h"
#include "FGJSBBase.h"
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FDMEXEC "$Id: FGFDMExec.h,v 1.51 2001/12/22 16:18:36 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGInitialCondition;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the JSBSim simulation executive.
    @author Jon S. Berndt
    @version $Id: FGFDMExec.h,v 1.51 2001/12/22 16:18:36 jberndt Exp $

    @doc This class is the interface class through which all other simulation classes
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

class FGFDMExec : public FGJSBBase
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

  /** This executes each scheduled model in succession.
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

  bool SetEnginePath(string path)   {EnginePath = path; return true;}
  bool SetAircraftPath(string path) {AircraftPath = path; return true;}

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
  unsigned int Frame;
  unsigned int IdFDM;
  static unsigned int FDMctr;
  bool modelLoaded;

  string AircraftPath;
  string EnginePath;
  string CFGVersion;

  FGState*           State;
  FGAtmosphere*      Atmosphere;
  FGFCS*             FCS;
  FGPropulsion*      Propulsion;
  FGMassBalance*     MassBalance;
  FGAerodynamics*    Aerodynamics;
  FGInertial*        Inertial;
  FGGroundReactions* GroundReactions;
  FGAircraft*        Aircraft;
  FGTranslation*     Translation;
  FGRotation*        Rotation;
  FGPosition*        Position;
  FGAuxiliary*       Auxiliary;
  FGOutput*          Output;

  bool ReadMetrics(FGConfigFile*);
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

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

