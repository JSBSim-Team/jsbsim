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

#define ID_FCS "$Id: FGFCS.h,v 1.55 2002/09/07 21:40:12 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

typedef enum { iDe=0, iDaL, iDaR, iDr, iDsb, iDsp, iDf, NNorm } FcIdx;
typedef enum { ofRad=0, ofNorm, ofMag , NForms} OutputForm;

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
    @version $Id: FGFCS.h,v 1.55 2002/09/07 21:40:12 apeden Exp $
    @see FGFCSComponent
    @see FGConfigFile
    @see FGGain
    @see FGSummer
    @see FGSwitch
    @see FGGradient
    @see FGFilter
    @see FGDeadBand
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGFCS.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGFCS.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
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
      @return aileron command in percent */
  inline double GetDaCmd(void) const { return DaCmd; }
  
  /** Gets the elevator command.
      @return elevator command in percent */
  inline double GetDeCmd(void) const { return DeCmd; }

  /** Gets the rudder command.
      @return rudder command in percent */
  inline double GetDrCmd(void) const { return DrCmd; }

  /** Gets the flaps command.
      @return flaps command in percent */
  inline double GetDfCmd(void) const { return DfCmd; }

  /** Gets the speedbrake command.
      @return speedbrake command in percent */
  inline double GetDsbCmd(void) const { return DsbCmd; }

  /** Gets the spoiler command.
      @return spoiler command in percent */
  inline double GetDspCmd(void) const { return DspCmd; }

  /** Gets the throttle command.
      @param engine engine ID number
      @return throttle command in percent ( 0 - 100) for the given engine */
  double GetThrottleCmd(int engine) const;

  /** Gets the mixture command.
      @param engine engine ID number
      @return mixture command in percent ( 0 - 100) for the given engine */
  inline double GetMixtureCmd(int engine) const { return MixtureCmd[engine]; }

  /** Gets the prop pitch command.
      @param engine engine ID number
      @return pitch command in percent ( 0.0 - 1.0) for the given engine */
  inline double GetPropAdvanceCmd(int engine) const { return PropAdvanceCmd[engine]; }

  /** Gets the pitch trim command.
      @return pitch trim command in percent */
  inline double GetPitchTrimCmd(void) const { return PTrimCmd; }
  
  /** Gets the rudder trim command.
      @return rudder trim command in percent */
  inline double GetYawTrimCmd(void) const { return YTrimCmd; }
  
  /** Gets the aileron trim command.
      @return aileron trim command in percent */
  inline double GetRollTrimCmd(void) const { return RTrimCmd; }
  
  /** Get the gear extend/retract command. 0 commands gear up, 1 down.
      defaults to down.
      @return the current value of the gear extend/retract command*/
  inline double GetGearCmd(void) const { return GearCmd; }    
  //@}

  /// @name AUTOPilot -> FCS effectors command retrieval
  //@{
  /** Gets the AUTOPilot aileron command.
      @return aileron command in radians */
  inline double GetAPDaCmd(void) const { return AP_DaCmd; }
  
  /** Gets the AUTOPilot elevator command.
      @return elevator command in radians */
  inline double GetAPDeCmd(void) const { return AP_DeCmd; }

  /** Gets the AUTOPilot rudder command.
      @return rudder command in radians */
  inline double GetAPDrCmd(void) const { return AP_DrCmd; }
  
  /** Gets the AUTOPilot throttle (all engines) command.
      @return throttle command in percent */
  inline double GetAPThrottleCmd(void) const { return AP_ThrottleCmd; }
  //@}

  /// @name AUTOPilot setpoint retrieval
  //@{
  /** Gets the autopilot pitch attitude setpoint
      @return Pitch attitude setpoint in radians */
  inline double GetAPAttitudeSetPt(void) const {return APAttitudeSetPt;}

  /** Gets the autopilot altitude setpoint
      @return Altitude setpoint in feet */
  inline double GetAPAltitudeSetPt(void) const {return APAltitudeSetPt;}

  /** Gets the autopilot heading setpoint
      @return Heading setpoint in radians */
  inline double GetAPHeadingSetPt(void) const {return APHeadingSetPt;}

  /** Gets the autopilot airspeed setpoint
      @return Airspeed setpoint in fps */
  inline double GetAPAirspeedSetPt(void) const {return APAirspeedSetPt;}
  //@}

  /// @name AUTOPilot setpoint setting
  //@{
  /// Sets the autopilot pitch attitude setpoint
  inline void SetAPAttitudeSetPt(double set) {APAttitudeSetPt = set;}

  /// Sets the autopilot altitude setpoint
  inline void SetAPAltitudeSetPt(double set) {APAltitudeSetPt = set;}

  /// Sets the autopilot heading setpoint
  inline void SetAPHeadingSetPt(double set) {APHeadingSetPt = set;}

  /// Sets the autopilot airspeed setpoint
  inline void SetAPAirspeedSetPt(double set) {APAirspeedSetPt = set;}
  //@}


    /// @name AUTOPilot mode setting
  //@{
  /** Turns on/off the attitude-seeking autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPAcquireAttitude(bool set) {APAcquireAttitude = set;}
  
  /** Turns on/off the altitude-seeking autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPAcquireAltitude(bool set) {APAcquireAltitude = set;}
  
  /** Turns on/off the heading-seeking autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPAcquireHeading(bool set) {APAcquireHeading = set;}
  
  /** Turns on/off the airspeed-seeking autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPAcquireAirspeed(bool set) {APAcquireAirspeed = set;}
  
  /** Turns on/off the attitude-holding autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPAttitudeHold(bool set) {APAttitudeHold = set;}

  /** Turns on/off the altitude-holding autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPAltitudeHold(bool set) {APAltitudeHold = set;}

  /** Turns on/off the heading-holding autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPHeadingHold(bool set) {APHeadingHold = set;}

  /** Turns on/off the airspeed-holding autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPAirspeedHold(bool set) {APAirspeedHold = set;}

  /** Turns on/off the wing-leveler autopilot.
      @param set true turns the mode on, false turns it off  **/
  inline void SetAPWingsLevelHold(bool set) {APWingsLevelHold = set;}
  //@}
  
  /// @name AUTOPilot mode retrieval
  //@{
  /** Retrieves the on/off mode of the autopilot AcquireAttitude mode
      @return true if on, false if off */
  inline bool GetAPAcquireAttitude(void) const {return APAcquireAttitude;}

  /** Retrieves the on/off mode of the autopilot AcquireAltitude mode
      @return true if on, false if off */
  inline bool GetAPAcquireAltitude(void) const {return APAcquireAltitude;}

  /** Retrieves the on/off mode of the autopilot AcquireHeading mode
      @return true if on, false if off */
  inline bool GetAPAcquireHeading(void) const {return APAcquireHeading;}

  /** Retrieves the on/off mode of the autopilot AcquireAirspeed mode
      @return true if on, false if off */
  inline bool GetAPAcquireAirspeed(void) const {return APAcquireAirspeed;}

  /** Retrieves the on/off mode of the autopilot AttitudeHold mode
      @return true if on, false if off */
  inline bool GetAPAttitudeHold(void) const {return APAttitudeHold;}

  /** Retrieves the on/off mode of the autopilot AltitudeHold mode
      @return true if on, false if off */
  inline bool GetAPAltitudeHold(void) const {return APAltitudeHold;}

  /** Retrieves the on/off mode of the autopilot HeadingHold mode
      @return true if on, false if off */
  inline bool GetAPHeadingHold(void) const {return APHeadingHold;}

  /** Retrieves the on/off mode of the autopilot AirspeedHold mode
      @return true if on, false if off */
  inline bool GetAPAirspeedHold(void) const {return APAirspeedHold;}

  /** Retrieves the on/off mode of the autopilot WingsLevelHold mode
      @return true if on, false if off */
  inline bool GetAPWingsLevelHold(void) const {return APWingsLevelHold;}
  //@}

  /// @name Aerosurface position retrieval
  //@{
  /** Gets the left aileron position.
      @return aileron position in radians */
  inline double GetDaLPos( int form = ofRad ) 
                         const { return DaLPos[form]; }

  /// @name Aerosurface position retrieval
  //@{
  /** Gets the right aileron position.
      @return aileron position in radians */
  inline double GetDaRPos( int form = ofRad ) 
                         const { return DaRPos[form]; }

  /** Gets the elevator position.
      @return elevator position in radians */
  inline double GetDePos( int form = ofRad ) 
                         const { return DePos[form]; }
 
  /** Gets the rudder position.
      @return rudder position in radians */
  inline double GetDrPos( int form = ofRad ) 
                         const { return DrPos[form]; }

  /** Gets the speedbrake position.
      @return speedbrake position in radians */
  inline double GetDsbPos( int form = ofRad ) 
                         const { return DsbPos[form]; }

  /** Gets the spoiler position.
      @return spoiler position in radians */
  inline double GetDspPos( int form = ofRad ) 
                         const { return DspPos[form]; }
  
  /** Gets the flaps position.
      @return flaps position in radians */
  inline double GetDfPos( int form = ofRad ) 
                         const { return DfPos[form]; }
                         
  /** Gets the throttle position.
      @param engine engine ID number
      @return throttle position for the given engine in percent ( 0 - 100)*/
  double GetThrottlePos(int engine) const;

  /** Gets the mixture position.
      @param engine engine ID number
      @return mixture position for the given engine in percent ( 0 - 100)*/
  inline double GetMixturePos(int engine) const { return MixturePos[engine]; }
  
  /** Gets the gear position (0 up, 1 down), defaults to down
      @return gear position (0 up, 1 down) */
  inline double GetGearPos(void) const { return GearPos; }    

  /** Gets the prop pitch position.
      @param engine engine ID number
      @return prop pitch position for the given engine in percent ( 0.0-1.0)*/
  inline double GetPropAdvance(int engine) const { return PropAdvance[engine]; }
  //@}

  /** Retrieves the State object pointer.
      This is used by the FGFCS-owned components.
      @return pointer to the State object */
  inline FGState* GetState(void) { return State; }

  /** Retrieves a components output value
      @param idx the index of the component (the component ID)
      @return output value from the component */
  double GetComponentOutput(int idx);

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
  inline void SetDaCmd( double cmd ) { DaCmd = cmd; }

  /** Sets the elevator command
      @param cmd elevator command in percent*/
  inline void SetDeCmd(double cmd ) { DeCmd = cmd; }

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

  /// @name AUTOPilot -> FCS effector command setting
  //@{
  /** Sets the AUTOPilot aileron command
      @param cmd AUTOPilot aileron command in radians*/
  inline void SetAPDaCmd( double cmd ) { AP_DaCmd = cmd; }

  /** Sets the AUTOPilot elevator command
      @param cmd AUTOPilot elevator command in radians*/
  inline void SetAPDeCmd(double cmd ) { AP_DeCmd = cmd; }

  /** Sets the AUTOPilot rudder command
      @param cmd AUTOPilot rudder command in radians*/
  inline void SetAPDrCmd(double cmd) { AP_DrCmd = cmd; }

  /** Sets the AUTOPilot throttle command
      @param cmd AUTOPilot throttle command in percent*/
  inline void SetAPThrottleCmd(double cmd) { AP_ThrottleCmd = cmd; }
  //@}

  /// @name Aerosurface position setting
  //@{
  /** Sets the left aileron position
      @param cmd left aileron position in radians*/
  inline void SetDaLPos( int form , double pos ) 
                                      { DaLPos[form] = pos; }

  /** Sets the right aileron position
      @param cmd right aileron position in radians*/
  inline void SetDaRPos( int form , double pos ) 
                                      { DaRPos[form] = pos; }

  /** Sets the elevator position
      @param cmd elevator position in radians*/
  inline void SetDePos( int form , double pos ) 
                                      { DePos[form] = pos; }

  /** Sets the rudder position
      @param cmd rudder position in radians*/
  inline void SetDrPos( int form , double pos ) 
                                      { DrPos[form] = pos; }
 
   /** Sets the flaps position
      @param cmd flaps position in radians*/
  inline void SetDfPos( int form , double pos ) 
                                      { DfPos[form] = pos; }
  
  /** Sets the speedbrake position
      @param cmd speedbrake position in radians*/
  inline void SetDsbPos( int form , double pos ) 
                                      { DsbPos[form] = pos; }

  /** Sets the spoiler position
      @param cmd spoiler position in radians*/
  inline void SetDspPos( int form , double pos ) 
                                      { DspPos[form] = pos; }
 
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
  
  FGPropertyManager* GetPropertyManager(void) { return PropertyManager; }
  
  void bind(void);
  void bindModel(void);
  void unbind(void);
  
private:
  double DaCmd, DeCmd, DrCmd, DfCmd, DsbCmd, DspCmd;
  double AP_DaCmd, AP_DeCmd, AP_DrCmd, AP_ThrottleCmd;
  double DePos[NForms], DaLPos[NForms], DaRPos[NForms], DrPos[NForms];  
  double DfPos[NForms], DsbPos[NForms], DspPos[NForms];
  double PTrimCmd, YTrimCmd, RTrimCmd;
  vector <double> ThrottleCmd;
  vector <double> ThrottlePos;
  vector <double> MixtureCmd;
  vector <double> MixturePos;
  vector <double> PropAdvanceCmd;
  vector <double> PropAdvance;
  double LeftBrake, RightBrake, CenterBrake; // Brake settings
  double GearCmd,GearPos;

  enum Mode {mAP, mFCS, mNone} eMode;

  double APAttitudeSetPt, APAltitudeSetPt, APHeadingSetPt, APAirspeedSetPt;
  bool APAcquireAttitude, APAcquireAltitude, APAcquireHeading, APAcquireAirspeed;
  bool APAttitudeHold, APAltitudeHold, APHeadingHold, APAirspeedHold, APWingsLevelHold;

  bool DoNormalize;
  void Normalize(void);

  vector <FGFCSComponent*> FCSComponents;
  vector <FGFCSComponent*> APComponents;
  int ToNormalize[NNorm];
  void Debug(int from);
};


#endif

