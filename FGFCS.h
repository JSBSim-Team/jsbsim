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
#  ifdef FG_HAVE_STD_INCLUDES
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

#define ID_FCS "$Id: FGFCS.h,v 1.27 2001/03/22 14:10:24 jberndt Exp $"

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
    @version $Id: FGFCS.h,v 1.27 2001/03/22 14:10:24 jberndt Exp $
    @see FGFCSComponent
    @see FGConfigFile
    @see FGGain
    @see FGSummer
    @see FGSwitch
    @see FGGradient
    @see FGFilter
    @see FGFlaps
    @see FGDeadBand
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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
      @return aileron command in radians */
  inline float GetDaCmd(void) { return DaCmd; }
  /** Gets the elevator command.
      @return elevator command in radians */
  inline float GetDeCmd(void) { return DeCmd; }
  /** Gets the rudder command.
      @return rudder command in radians */
  inline float GetDrCmd(void) { return DrCmd; }
  /** Gets the flaps command.
      @return flaps command in radians */
  inline float GetDfCmd(void) { return DfCmd; }
  /** Gets the speedbrake command.
      @return speedbrake command in radians */
  inline float GetDsbCmd(void) { return DsbCmd; }
  /** Gets the spoiler command.
      @return spoiler command in radians */
  inline float GetDspCmd(void) { return DspCmd; }
  /** Gets the throttle command.
      @param engine engine ID number
      @return throttle command in percent ( 0 - 100) for the given engine */
  inline float GetThrottleCmd(int engine) { return ThrottleCmd[engine]; }
  /** Gets the pitch trim command.
      @return pitch trim command in radians */
  inline float GetPitchTrimCmd(void) { return PTrimCmd; }
  //@}
  
  /// @name Aerosurface position retrieval
  //@{
  /** Gets the aileron position.
      @return aileron position in radians */
  inline float GetDaPos(void) { return DaPos; }
  /** Gets the elevator position.
      @return elevator position in radians */
  inline float GetDePos(void) { return DePos; }
  /** Gets the rudder position.
      @return rudder position in radians */
  inline float GetDrPos(void) { return DrPos; }
  /** Gets the flaps position.
      @return flaps position in radians */
  inline float GetDfPos(void) { return DfPos; }
  /** Gets the speedbrake position.
      @return speedbrake position in radians */
  inline float GetDsbPos(void) { return DsbPos; }
  /** Gets the spoiler position.
      @return spoiler position in radians */
  inline float GetDspPos(void) { return DspPos; }
  /** Gets the throttle position.
      @param engine engine ID number
      @return throttle position for the given engine in percent ( 0 - 100)*/
  inline float GetThrottlePos(int engine) { return ThrottlePos[engine]; }
  //@}

  /** Retrieves the State object pointer.
      This is used by the FGFCS-owned components.
      @return pointer to the State object */
  inline FGState* GetState(void) { return State; }
  /** Retrieves a components output value
      @param idx the index of the component (the component ID)
      @return output value from the component */
  float GetComponentOutput(eParam idx);
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
      @param cmd aileron command in radians*/
  inline void SetDaCmd(float cmd) { DaCmd = cmd; }
  /** Sets the elevator command
      @param cmd elevator command in radians*/
  inline void SetDeCmd(float cmd) { DeCmd = cmd; }
  /** Sets the rudder command
      @param cmd rudder command in radians*/
  inline void SetDrCmd(float cmd) { DrCmd = cmd; }
  /** Sets the flaps command
      @param cmd flaps command in radians*/
  inline void SetDfCmd(float cmd) { DfCmd = cmd; }
  /** Sets the speedbrake command
      @param cmd speedbrake command in radians*/
  inline void SetDsbCmd(float cmd) { DsbCmd = cmd; }
  /** Sets the spoilers command
      @param cmd spoilers command in radians*/
  inline void SetDspCmd(float cmd) { DspCmd = cmd; }
  /** Sets the pitch trim command
      @param cmd pitch trim command in radians*/
  inline void SetPitchTrimCmd(float cmd) { PTrimCmd = cmd; }
  /** Sets the throttle command for the specified engine
      @param engine engine ID number 
      @param cmd throttle command in percent (0 - 100)*/
  inline void SetThrottleCmd(int engine, float cmd);
  //@}

  /// @name Aerosurface position setting
  //@{
  /** Sets the aileron position
      @param cmd aileron position in radians*/
  inline void SetDaPos(float cmd) { DaPos = cmd; }
  /** Sets the elevator position
      @param cmd elevator position in radians*/
  inline void SetDePos(float cmd) { DePos = cmd; }
  /** Sets the rudder position
      @param cmd rudder position in radians*/
  inline void SetDrPos(float cmd) { DrPos = cmd; }
  /** Sets the flaps position
      @param cmd flaps position in radians*/
  inline void SetDfPos(float cmd) { DfPos = cmd; }
  /** Sets the speedbrake position
      @param cmd speedbrake position in radians*/
  inline void SetDsbPos(float cmd) { DsbPos = cmd; }
  /** Sets the spoiler position
      @param cmd spoiler position in radians*/
  inline void SetDspPos(float cmd) { DspPos = cmd; }
  /** Sets the actual throttle setting for the specified engine
      @param engine engine ID number 
      @param cmd throttle setting in percent (0 - 100)*/
  inline void SetThrottlePos(int engine, float cmd);
  //@}

  /// @name Landing Gear brakes
  //@{
  /** Sets the left brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetLBrake(float cmd) {LeftBrake = cmd;}
  /** Sets the right brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetRBrake(float cmd) {RightBrake = cmd;}
  /** Sets the center brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetCBrake(float cmd) {CenterBrake = cmd;}
  /** Gets the brake for a specified group.
      @param bg which brakegroup to retrieve the command for
      @return the brake setting for the supplied brake group argument */
  float GetBrake(FGLGear::BrakeGroup bg);
  //@}

  /** Loads the Flight Control System.
      The FGAircraft instance is actually responsible for reading the config file
      and calling the various Loadxx() methods of the other systems, passing in
      the config file instance pointer. LoadFCS() is called from FGAircraft.
      @param AC_cfg pointer to the config file instance
      @return true if succesful */
  bool LoadFCS(FGConfigFile* AC_cfg);

  /** The name of the flight control laws for this aircraft.
      This is given in the config file, and is not used for anything currently.*/
  string FCSName;

  void AddThrottle(void);

private:
  float DaCmd, DeCmd, DrCmd, DfCmd, DsbCmd, DspCmd;
  float DaPos, DePos, DrPos, DfPos, DsbPos, DspPos;
  float PTrimCmd;
  vector <float> ThrottleCmd;
  vector <float> ThrottlePos;
  float LeftBrake, RightBrake, CenterBrake; // Brake settings

  vector <FGFCSComponent*> Components;
  void Debug(void);
};

#include "FGState.h"

#endif

