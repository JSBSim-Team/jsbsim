/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGFCS.h
 Author:       Jon S. Berndt
 Date started: 12/12/98
 
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
12/12/98   JSB   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFCS_H
#define FGFCS_H

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

#include <string>
#include "filtersjb/FGFCSComponent.h"
#include "FGModel.h"
#include "FGLGear.h"
#include "FGConfigFile.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FCS "$Id: FGFCS.h,v 1.45 2002/02/28 12:16:45 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the Flight Control System (FCS) functionality.
    <ul><li>\URL[Source Code]{FGFCS.cpp.html}</li>
    <li>\URL[Header File]{FGFCS.h.html}</li></ul>
    This class owns and contains the list of \URL[components]{FGFCSComponent.html}
    that define the control system for this aircraft. The config file for the
    aircraft contains a description of the control path that starts at an input
    or command and ends at an effector, e.g. an aerosurface. The FCS components
    which comprise the control laws for an axis are defined sequentially in
    the configuration file. For instance, for the X-15:
    
    <pre>
    &lt FLIGHT_CONTROL NAME="X-15 SAS"&gt

    &lt COMPONENT NAME="Pitch Trim Sum" TYPE="SUMMER"&gt
      ID            0
      INPUT        FG_ELEVATOR_CMD
      INPUT        FG_PITCH_TRIM_CMD
      CLIPTO       -1 1
    &lt/COMPONENT&gt

    &lt COMPONENT NAME="Pitch Command Scale" TYPE="AEROSURFACE_SCALE"&gt
      ID           1
      INPUT        0
      MIN         -50
      MAX          50
    &lt/COMPONENT&gt

    &lt COMPONENT NAME="Pitch Gain 1" TYPE="PURE_GAIN"&gt
      ID           2
      INPUT        1
      GAIN         -0.36
    &lt/COMPONENT&gt

    &lt COMPONENT NAME="Pitch Scheduled Gain 1" TYPE="SCHEDULED_GAIN"&gt
      ID           3
      INPUT        2
      GAIN         0.017
      SCHEDULED_BY FG_ELEVATOR_POS
      -0.35  -6.0
      -0.17  -3.0
       0.00  -2.0
       0.09  -3.0
       0.17  -5.0
       0.60 -12.0
    &lt/COMPONENT&gt

    ... etc.
    </pre>
    
    In the above case we can see the first few components of the pitch channel
    defined. The input to the first component, as can be seen in the "Pitch trim
    sum" component, is really the sum of two parameters: elevator command (from
    the stick - a pilot input), and pitch trim. The type of this component is
    "Summer". Its ID is 0 - the ID is used by other components to reference it.
    The next component created is an aerosurface scale component - a type of
    gain (see the LoadFCS() method for insight on how the various types of
    components map into the actual component classes). You can see the input of
    the "Pitch Command Scale" component takes "0" as input. When a number is
    specified as an input, it refers to the ID of another FCS component. In this
    case, ID 0 refers to the previously defined and discussed "Pitch Trim Sum"
    component. This continues until the final component for an axis when the
    OUTPUT keyword specifies where the output is supposed to go. See the
    individual components for more information on how they are mechanized.
    
    @author Jon S. Berndt
    @version $Id: FGFCS.h,v 1.45 2002/02/28 12:16:45 apeden Exp $
    @see FGFCSComponent
    @see FGConfigFile
    @see FGGain
    @see FGSummer
    @see FGSwitch
    @see FGGradient
    @see FGFilter
    @see FGDeadBand
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

typedef enum { iNDe=0, iNDaL, iNDaR, iNDr, iNDsb, iNDsp, iNDf } NormalizeIdx;

class FGFCS : public FGModel {

public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGFCS(FGFDMExec*);
  /// Destructor
  ~FGFCS();

  /** Runs the Flight Controls model; called by the Executive
      @return false if no error */
  bool Run(void);

  /// @name Pilot input command retrieval
  //@{
  /** Gets the aileron command.
      @return aileron command in percent */
  inline double GetDaCmd(void) { return DaCmd; }
  
  /** Gets the elevator command.
      @return elevator command in percent */
  inline double GetDeCmd(void) { return DeCmd; }

  /** Gets the rudder command.
      @return rudder command in percent */
  inline double GetDrCmd(void) { return DrCmd; }

  /** Gets the flaps command.
      @return flaps command in percent */
  inline double GetDfCmd(void) { return DfCmd; }

  /** Gets the speedbrake command.
      @return speedbrake command in percent */
  inline double GetDsbCmd(void) { return DsbCmd; }

  /** Gets the spoiler command.
      @return spoiler command in percent */
  inline double GetDspCmd(void) { return DspCmd; }

  /** Gets the throttle command.
      @param engine engine ID number
      @return throttle command in percent ( 0 - 100) for the given engine */
  double GetThrottleCmd(int engine);

  /** Gets the mixture command.
      @param engine engine ID number
      @return mixture command in percent ( 0 - 100) for the given engine */
  inline double GetMixtureCmd(int engine) { return MixtureCmd[engine]; }

  /** Gets the prop pitch command.
      @param engine engine ID number
      @return pitch command in percent ( 0.0 - 1.0) for the given engine */
  inline double GetPropAdvanceCmd(int engine) { return PropAdvanceCmd[engine]; }

  /** Gets the pitch trim command.
      @return pitch trim command in percent */
  inline double GetPitchTrimCmd(void) { return PTrimCmd; }
  
  /** Gets the rudder trim command.
      @return rudder trim command in percent */
  inline double GetYawTrimCmd(void) { return YTrimCmd; }
  
  /** Gets the aileron trim command.
      @return aileron trim command in percent */
  inline double GetRollTrimCmd(void) { return RTrimCmd; }
  
  /** Get the gear extend/retract command. 0 commands gear up, 1 down.
      defaults to down.
      @return the current value of the gear extend/retract command*/
  inline double GetGearCmd(void) { return GearCmd; }    
  //@}

  /// @name Aerosurface position retrieval
  //@{
  /** Gets the left aileron position.
      @return aileron position in radians */
  inline double GetDaLPos(void) { return DaLPos; }

  /// @name Aerosurface position retrieval
  //@{
  /** Gets the normalized left aileron position.
      @return aileron position in radians */
  inline double GetDaLPosN(void) { return DaLPosN; }

  /// @name Aerosurface position retrieval
  //@{
  /** Gets the right aileron position.
      @return aileron position in radians */
  inline double GetDaRPos(void) { return DaRPos; }

  /// @name Aerosurface position retrieval
  //@{
  /** Gets the normalized right aileron position.
      @return right aileron position in percent (-1..1) */
  inline double GetDaRPosN(void) { return DaRPosN; }

  /** Gets the elevator position.
      @return elevator position in radians */
  inline double GetDePos(void) { return DePos; }
 
  /** Gets the normalized elevator position.
      @return  elevator position in percent (-1..1) */
  inline double GetDePosN(void) { return DePosN; }

  /** Gets the rudder position.
      @return rudder position in radians */
  inline double GetDrPos(void) { return DrPos; }

  /** Gets the normalized rudder position.
      @return rudder position in percent (-1..1) */
  inline double GetDrPosN(void) { return DrPosN; }

  /** Gets the flaps position.
      @return flaps position in radians */
  inline double GetDfPos(void) { return DfPos; }

  /** Gets the normalized flaps position.
      @return flaps position in percent (-1..1) */
  inline double GetDfPosN(void) { return DfPosN; }

  /** Gets the speedbrake position.
      @return speedbrake position in radians */
  inline double GetDsbPos(void) { return DsbPos; }

  /** Gets the normalized speedbrake position.
      @return speedbrake position in percent (-1..1) */
  inline double GetDsbPosN(void) { return DsbPosN; }

  /** Gets the spoiler position.
      @return spoiler position in radians */
  inline double GetDspPos(void) { return DspPos; }
  
  /** Gets the normalized spoiler position.
      @return spoiler position in percent (-1..1) */
  inline double GetDspPosN(void) { return DspPosN; }

  /** Gets the throttle position.
      @param engine engine ID number
      @return throttle position for the given engine in percent ( 0 - 100)*/
  double GetThrottlePos(int engine);

  /** Gets the mixture position.
      @param engine engine ID number
      @return mixture position for the given engine in percent ( 0 - 100)*/
  inline double GetMixturePos(int engine) { return MixturePos[engine]; }
  
  /** Gets the gear position (0 up, 1 down), defaults to down
      @return gear position (0 up, 1 down) */
  inline double GetGearPos(void) { return GearPos; }    

  /** Gets the prop pitch position.
      @param engine engine ID number
      @return prop pitch position for the given engine in percent ( 0.0-1.0)*/
  inline double GetPropAdvance(int engine) { return PropAdvance[engine]; }
  //@}

  /** Retrieves the State object pointer.
      This is used by the FGFCS-owned components.
      @return pointer to the State object */
  inline FGState* GetState(void) { return State; }

  /** Retrieves a components output value
      @param idx the index of the component (the component ID)
      @return output value from the component */
  double GetComponentOutput(eParam idx);

  /** Retrieves the component name
      @param idx the index of the component (the component ID)
      @return name of the component */
  string GetComponentName(int idx);

  /** Retrieves all component names for inclusion in output stream */
  string GetComponentStrings(void);

  /** Retrieves all component outputs for inclusion in output stream */
  string GetComponentValues(void);

  /// @name Pilot input command setting
  //@{
  /** Sets the aileron command
      @param cmd aileron command in percent*/
  inline void SetDaCmd(double cmd) { DaCmd = cmd; }

  /** Sets the elevator command
      @param cmd elevator command in percent*/
  inline void SetDeCmd(double cmd) { DeCmd = cmd; }

  /** Sets the rudder command
      @param cmd rudder command in percent*/
  inline void SetDrCmd(double cmd) { DrCmd = cmd; }

  /** Sets the flaps command
      @param cmd flaps command in percent*/
  inline void SetDfCmd(double cmd) { DfCmd = cmd; }

  /** Sets the speedbrake command
      @param cmd speedbrake command in percent*/
  inline void SetDsbCmd(double cmd) { DsbCmd = cmd; }

  /** Sets the spoilers command
      @param cmd spoilers command in percent*/
  inline void SetDspCmd(double cmd) { DspCmd = cmd; }

  /** Sets the pitch trim command
      @param cmd pitch trim command in percent*/
  inline void SetPitchTrimCmd(double cmd) { PTrimCmd = cmd; }

  /** Sets the rudder trim command
      @param cmd rudder trim command in percent*/
  inline void SetYawTrimCmd(double cmd) { YTrimCmd = cmd; }

  /** Sets the aileron trim command
      @param cmd aileron trim command in percent*/
  inline void SetRollTrimCmd(double cmd) { RTrimCmd = cmd; }

  /** Sets the throttle command for the specified engine
      @param engine engine ID number
      @param cmd throttle command in percent (0 - 100)*/
  void SetThrottleCmd(int engine, double cmd);

  /** Sets the mixture command for the specified engine
      @param engine engine ID number
      @param cmd mixture command in percent (0 - 100)*/
  void SetMixtureCmd(int engine, double cmd);
  
  /** Set the gear extend/retract command, defaults to down
      @param gear command 0 for up, 1 for down */
   void SetGearCmd(double gearcmd) { GearCmd = gearcmd; }   

  /** Sets the propeller pitch command for the specified engine
      @param engine engine ID number
      @param cmd mixture command in percent (0.0 - 1.0)*/
  void SetPropAdvanceCmd(int engine, double cmd);
  //@}

  /// @name Aerosurface position setting
  //@{
  /** Sets the left aileron position
      @param cmd left aileron position in radians*/
  inline void SetDaLPos(double cmd) { DaLPos = cmd; }

  /** Sets the normalized left aileron position
      @param cmd left aileron position in percent (-1..1)*/
  inline void SetDaLPosN(double cmd) { DaLPosN = cmd; }

  /** Sets the right aileron position
      @param cmd right aileron position in radians*/
  inline void SetDaRPos(double cmd) { DaRPos = cmd; }

  /** Sets the normalized right aileron position
      @param cmd right aileron position in percent (-1..1)*/
  inline void SetDaRPosN(double cmd) { DaRPosN = cmd; }

  /** Sets the elevator position
      @param cmd elevator position in radians*/
  inline void SetDePos(double cmd) { DePos = cmd; }

  /** Sets the normalized elevator position
      @param cmd elevator position in percent (-1..1) */
  inline void SetDePosN(double cmd) { DePosN = cmd; }

  /** Sets the rudder position
      @param cmd rudder position in radians*/
  inline void SetDrPos(double cmd) { DrPos = cmd; }
 
  /** Sets the normalized rudder position
      @param cmd rudder position in percent (-1..1)*/
  inline void SetDrPosN(double cmd) { DrPosN = cmd; }

  /** Sets the flaps position
      @param cmd flaps position in radians*/
  inline void SetDfPos(double cmd) { DfPos = cmd; }
  
  /** Sets the flaps position
      @param cmd flaps position in radians*/
  inline void SetDfPosN(double cmd) { DfPosN = cmd; }

  /** Sets the speedbrake position
      @param cmd speedbrake position in radians*/
  inline void SetDsbPos(double cmd) { DsbPos = cmd; }
 
  /** Sets the normalized speedbrake position
      @param cmd normalized speedbrake position in percent (-1..1)*/
  inline void SetDsbPosN(double cmd) { DsbPosN = cmd; }

  /** Sets the spoiler position
      @param cmd spoiler position in radians*/
  inline void SetDspPos(double cmd) { DspPos = cmd; }
 
  /** Sets the normalized spoiler position
      @param cmd normalized spoiler position in percent (-1..1)*/
  inline void SetDspPosN(double cmd) { DspPosN = cmd; }

  /** Sets the actual throttle setting for the specified engine
      @param engine engine ID number
      @param cmd throttle setting in percent (0 - 100)*/
  void SetThrottlePos(int engine, double cmd);

  /** Sets the actual mixture setting for the specified engine
      @param engine engine ID number
      @param cmd mixture setting in percent (0 - 100)*/
  void SetMixturePos(int engine, double cmd);
  
  /** Set the gear extend/retract position, defaults to down
      @param gear position 0 up, 1 down       */
   void SetGearPos(double gearpos) { GearPos = gearpos; }   


  /** Sets the actual prop pitch setting for the specified engine
      @param engine engine ID number
      @param cmd prop pitch setting in percent (0.0 - 1.0)*/
  void SetPropAdvance(int engine, double cmd);
  //@}

  /// @name Landing Gear brakes
  //@{
  /** Sets the left brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetLBrake(double cmd) {LeftBrake = cmd;}

  /** Sets the right brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetRBrake(double cmd) {RightBrake = cmd;}

  /** Sets the center brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetCBrake(double cmd) {CenterBrake = cmd;}

  /** Gets the brake for a specified group.
      @param bg which brakegroup to retrieve the command for
      @return the brake setting for the supplied brake group argument */
  double GetBrake(FGLGear::BrakeGroup bg);
  //@}

  /** Loads the Flight Control System.
      The FGAircraft instance is actually responsible for reading the config file
      and calling the various Loadxx() methods of the other systems, passing in
      the config file instance pointer. LoadFCS() is called from FGAircraft.
      @param AC_cfg pointer to the config file instance
      @return true if succesful */
  bool Load(FGConfigFile* AC_cfg);

  void AddThrottle(void);

private:
  double DaCmd,   DeCmd,   DrCmd,  DfCmd,  DsbCmd, DspCmd;
  double DaLPos,  DaRPos,  DePos,  DrPos,  DfPos,  DsbPos,  DspPos;
  double DaLPosN, DaRPosN, DePosN, DrPosN, DfPosN, DsbPosN, DspPosN;
  double PTrimCmd, YTrimCmd, RTrimCmd;
  vector <double> ThrottleCmd;
  vector <double> ThrottlePos;
  vector <double> MixtureCmd;
  vector <double> MixturePos;
  vector <double> PropAdvanceCmd;
  vector <double> PropAdvance;
  double LeftBrake, RightBrake, CenterBrake; // Brake settings
  double GearCmd,GearPos;
  
  bool DoNormalize;
  void Normalize(void);

  vector <FGFCSComponent*> Components;
  int ToNormalize[7];
  void Debug(int from);
};

#include "FGState.h"

#endif

