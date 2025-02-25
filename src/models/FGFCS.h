/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGFCS.h
 Author:       Jon S. Berndt
 Date started: 12/12/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
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

#include <iosfwd>
#include <vector>
#include <string>

#include "FGFDMExec.h"
#include "models/FGModel.h"
#include "models/FGLGear.h"
#include "models/FGGroundReactions.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCSChannel;
typedef enum { ofRad=0, ofDeg, ofNorm, ofMag , NForms} OutputForm;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the Flight Control System (FCS) functionality.
    This class also encapsulates the identical "system" and "autopilot" capability.
    FGFCS owns and contains the list of FGFCSComponents
    that define a system or systems for the modeled aircraft. The config file
    for the aircraft contains a description of the control path that starts at
    an input or command and ends at an effector, e.g. an aerosurface. The FCS
    components which comprise the control laws for an axis are defined
    sequentially in the configuration file. For instance, for the X-15:

    @code
    <flight_control name="X-15 SAS">
      <channel>
        <summer name="Pitch Trim Sum">
           <input> fcs/elevator-cmd-norm </input>
           <input> fcs/pitch-trim-cmd-norm </input>
           <clipto>
             <min>-1</min>
             <max>1</max>
           </clipto>
        </summer>

        <aerosurface_scale name="Pitch Command Scale">
          <input> fcs/pitch-trim-sum </input>
          <range>
            <min> -50 </min>
            <max>  50 </max>
          </range>
        </aerosurface_scale>

        ... etc.
    @endcode

    In the above case we can see the first few components of the pitch channel
    defined. The input to the first component (a summer), as can be seen in the "Pitch trim
    sum" component, is really the sum of two parameters: elevator command (from
    the stick - a pilot input), and pitch trim.
    The next component created is an aerosurface scale component - a type of
    gain (see the LoadFCS() method for insight on how the various types of
    components map into the actual component classes).  This continues until the
    final component for an axis when the
    \<output> element is usually used to specify where the output is supposed to go. See the
    individual components for more information on how they are mechanized.

    Another option for the flight controls portion of the config file is that in
    addition to using the "NAME" attribute in,

    @code
    <flight_control name="X-15 SAS">
    @endcode

    one can also supply a filename:

    @code
    <flight_control name="X-15 SAS" file="X15.xml">
    </flight_control>
    @endcode

    In this case, the FCS would be read in from another file.

    <h2>Properties</h2>
    @property fcs/aileron-cmd-norm normalized aileron command
    @property fcs/elevator-cmd-norm normalized elevator command
    @property fcs/rudder-cmd-norm
    @property fcs/steer-cmd-norm
    @property fcs/flap-cmd-norm
    @property fcs/speedbrake-cmd-norm
    @property fcs/spoiler-cmd-norm
    @property fcs/pitch-trim-cmd-norm
    @property fcs/roll-trim-cmd-norm
    @property fcs/yaw-trim-cmd-norm
    @property gear/gear-cmd-norm
    @property fcs/left-aileron-pos-rad
    @property fcs/left-aileron-pos-deg
    @property fcs/left-aileron-pos-norm
    @property fcs/mag-left-aileron-pos-rad
    @property fcs/right-aileron-pos-rad
    @property fcs/right-aileron-pos-deg
    @property fcs/right-aileron-pos-norm
    @property fcs/mag-right-aileron-pos-rad
    @property fcs/elevator-pos-rad
    @property fcs/elevator-pos-deg
    @property fcs/elevator-pos-norm
    @property fcs/mag-elevator-pos-rad
    @property fcs/rudder-pos-rad
    @property fcs/rudder-pos-deg
    @property fcs/rudder-pos-norm
    @property fcs/mag-rudder-pos-rad
    @property fcs/flap-pos-rad
    @property fcs/flap-pos-deg
    @property fcs/flap-pos-norm
    @property fcs/speedbrake-pos-rad
    @property fcs/speedbrake-pos-deg
    @property fcs/speedbrake-pos-norm
    @property fcs/mag-speedbrake-pos-rad
    @property fcs/spoiler-pos-rad
    @property fcs/spoiler-pos-deg
    @property fcs/spoiler-pos-norm
    @property fcs/mag-spoiler-pos-rad
    @property fcs/wing-fold-pos-norm
    @property gear/gear-pos-norm
    @property gear/tailhook-pos-norm

    @author Jon S. Berndt
    @version $Revision: 1.55 $
    @see FGActuator
    @see FGDeadBand
    @see FGFCSFunction
    @see FGFilter
    @see FGGain
    @see FGKinemat
    @see FGPID
    @see FGSensor
    @see FGSummer
    @see FGSwitch
    @see FGWaypoint
    @see FGAngles
    @see FGFCSComponent
    @see Element
    @see FGDistributor
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGFCS : public FGModel
{

public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGFCS(FGFDMExec*);
  /// Destructor
  ~FGFCS() override;

  bool InitModel(void) override;

  /** Runs the Flight Controls model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;

  /// @name Pilot input command retrieval
  //@{
  /** Gets the aileron command.
      @return aileron command in range from -1.0 - 1.0 */
  double GetDaCmd(void) const { return DaCmd; }

  /** Gets the elevator command.
      @return elevator command in range from -1.0 - 1.0 */
  double GetDeCmd(void) const { return DeCmd; }

  /** Gets the rudder command.
      @return rudder command in range from -1.0 - 1.0 */
  double GetDrCmd(void) const { return DrCmd; }

  /** Gets the steering command.
       @return steering command in range from -1.0 - 1.0 */
   double GetDsCmd(void) const { return FDMExec->GetGroundReactions()->GetDsCmd(); }

  /** Gets the flaps command.
      @return flaps command in range from 0 to 1.0 */
  double GetDfCmd(void) const { return DfCmd; }

  /** Gets the speedbrake command.
      @return speedbrake command in range from 0 to 1.0 */
  double GetDsbCmd(void) const { return DsbCmd; }

  /** Gets the spoiler command.
      @return spoiler command in range from 0 to 1.0 */
  double GetDspCmd(void) const { return DspCmd; }

  /** Gets the throttle command.
      @param engine engine ID number
      @return throttle command in range from 0 - 1.0 for the given engine */
  double GetThrottleCmd(int engine) const;

  const std::vector<double>& GetThrottleCmd() const {return ThrottleCmd;}

/** Gets the mixture command.
    @param engine engine ID number
    @return mixture command for the given engine
    @note The mixture command is generally in range from 0 - 1.0, but some
          models might set it to a value above 1.0.
    @note JSBSim does not check nor enforce the value to be within 0.0-1.0.
*/
  double GetMixtureCmd(int engine) const { return MixtureCmd[engine]; }

  const std::vector<double>& GetMixtureCmd() const {return MixtureCmd;}

  /** Gets the prop pitch command.
      @param engine engine ID number
      @return pitch command in range from 0.0 - 1.0 for the given engine */
  double GetPropAdvanceCmd(int engine) const { return PropAdvanceCmd[engine]; }

  /** Gets the prop feather command.
      @param engine engine ID number
      @return feather command for the given engine (on / off)*/
  bool GetFeatherCmd(int engine) const { return PropFeatherCmd[engine]; }

  /** Gets the pitch trim command.
      @return pitch trim command in range from -1.0 to 1.0 */
  double GetPitchTrimCmd(void) const { return PTrimCmd; }

  /** Gets the rudder trim command.
      @return rudder trim command in range from -1.0 - 1.0 */
  double GetYawTrimCmd(void) const { return YTrimCmd; }

  /** Gets the aileron trim command.
      @return aileron trim command in range from -1.0 - 1.0 */
  double GetRollTrimCmd(void) const { return RTrimCmd; }

  /** Get the gear extend/retract command. 0 commands gear up, 1 down.
      defaults to down.
      @return the current value of the gear extend/retract command*/
  double GetGearCmd(void) const { return GearCmd; }
  //@}

  /// @name Aerosurface position retrieval
  //@{
  /** Gets the left aileron position.
      @return aileron position in radians */
  double GetDaLPos( int form = ofRad )
                         const { return DaLPos[form]; }

  /** Gets the right aileron position.
      @return aileron position in radians */
  double GetDaRPos( int form = ofRad )
                         const { return DaRPos[form]; }

  /** Gets the elevator position.
      @return elevator position in radians */
  double GetDePos( int form = ofRad )
                         const { return DePos[form]; }

  /** Gets the rudder position.
      @return rudder position in radians */
  double GetDrPos( int form = ofRad )
                         const { return DrPos[form]; }

  /** Gets the speedbrake position.
      @return speedbrake position in radians */
  double GetDsbPos( int form = ofRad )
                         const { return DsbPos[form]; }

  /** Gets the spoiler position.
      @return spoiler position in radians */
  double GetDspPos( int form = ofRad )
                         const { return DspPos[form]; }

  /** Gets the flaps position.
      @return flaps position in radians */
  double GetDfPos( int form = ofRad )
                         const { return DfPos[form]; }

  /** Gets the throttle position.
      @param engine engine ID number
      @return throttle position for the given engine in range from 0 - 1.0 */
  double GetThrottlePos(int engine) const;

  const std::vector<double>& GetThrottlePos() const {return ThrottlePos;}

  /** Gets the mixture position.
      @param engine engine ID number
      @return mixture position for the given engine
      @note The mixture position is generally in range from 0 - 1.0, but some
            models might set it to a value above 1.0.
      @note JSBSim does not check nor enforce the value to be within 0.0-1.0.
  */
  double GetMixturePos(int engine) const { return MixturePos[engine]; }

  const std::vector<double>& GetMixturePos() const {return MixturePos;}

  /** Gets the gear position (0 up, 1 down), defaults to down
      @return gear position (0 up, 1 down) */
  double GetGearPos(void) const { return GearPos; }

  /** Gets the tailhook position (0 up, 1 down)
      @return tailhook position (0 up, 1 down) */
  double GetTailhookPos(void) const { return TailhookPos; }

  /** Gets the wing fold position (0 unfolded, 1 folded)
      @return wing fold position (0 unfolded, 1 folded) */
  double GetWingFoldPos(void) const { return WingFoldPos; }

  /** Gets the prop pitch position.
      @param engine engine ID number
      @return prop pitch position for the given engine in range from 0 - 1.0 */
  double GetPropAdvance(int engine) const { return PropAdvance[engine]; }

  const std::vector<double>& GetPropAdvance() const { return PropAdvance; }

  /** Gets the prop feather position.
      @param engine engine ID number
      @return prop fether for the given engine (on / off)*/
  bool GetPropFeather(int engine) const { return PropFeather[engine]; }

  const std::vector<bool>& GetPropFeather() const { return PropFeather; }
  //@}

  /** Retrieves all component names for inclusion in output stream
      @param delimiter either a tab or comma string depending on output type
      @return a string containing the descriptive names for all components */
  std::string GetComponentStrings(const std::string& delimiter) const;

  /** Retrieves all component outputs for inclusion in output stream
      @param delimiter either a tab or comma string depending on output type
      @return a string containing the numeric values for the current set of
      component outputs */
  std::string GetComponentValues(const std::string& delimiter) const;

  /// @name Pilot input command setting
  //@{
  /** Sets the aileron command
      @param cmd aileron command */
  void SetDaCmd( double cmd ) { DaCmd = cmd; }

  /** Sets the elevator command
      @param cmd elevator command in percent*/
  void SetDeCmd(double cmd ) { DeCmd = cmd; }

  /** Sets the rudder command
      @param cmd rudder command in percent*/
  void SetDrCmd(double cmd) { DrCmd = cmd; }

  /** Sets the steering command
       @param cmd steering command in percent*/
   void SetDsCmd(double cmd) { FDMExec->GetGroundReactions()->SetDsCmd( cmd ); }

  /** Sets the flaps command
      @param cmd flaps command in percent*/
  void SetDfCmd(double cmd) { DfCmd = cmd; }

  /** Sets the speedbrake command
      @param cmd speedbrake command in percent*/
  void SetDsbCmd(double cmd) { DsbCmd = cmd; }

  /** Sets the spoilers command
      @param cmd spoilers command in percent*/
  void SetDspCmd(double cmd) { DspCmd = cmd; }

  /** Sets the pitch trim command
      @param cmd pitch trim command in percent*/
  void SetPitchTrimCmd(double cmd) { PTrimCmd = cmd; }

  /** Sets the rudder trim command
      @param cmd rudder trim command in percent*/
  void SetYawTrimCmd(double cmd) { YTrimCmd = cmd; }

  /** Sets the aileron trim command
      @param cmd aileron trim command in percent*/
  void SetRollTrimCmd(double cmd) { RTrimCmd = cmd; }

  /** Sets the throttle command for the specified engine
      @param engine engine ID number
      @param cmd normalized throttle command (0.0 - 1.0)*/
  void SetThrottleCmd(int engine, double cmd);

  /** Sets the mixture command for the specified engine
      @param engine engine ID number
      @param cmd normalized mixture command
      @note The mixture command is generally in range from 0 - 1.0, but some
            models handle values above 1.0.
      @note JSBSim does not check nor enforce the value to be within 0.0-1.0.
  */
  void SetMixtureCmd(int engine, double cmd);

  /** Set the gear extend/retract command, defaults to down
      @param gear command 0 for up, 1 for down */
   void SetGearCmd(double gearcmd) { GearCmd = gearcmd; }

  /** Sets the propeller pitch command for the specified engine
      @param engine engine ID number
      @param cmd pitch command in percent (0.0 - 1.0)*/
  void SetPropAdvanceCmd(int engine, double cmd);

   /** Sets the propeller feather command for the specified engine
      @param engine engine ID number
      @param cmd feather (bool)*/
  void SetFeatherCmd(int engine, bool cmd);
  //@}

  /// @name Aerosurface position setting
  //@{
  /** Sets the left aileron position
      @param cmd left aileron position in radians*/
  void SetDaLPos( int form , double pos );

  /** Sets the right aileron position
      @param cmd right aileron position in radians*/
  void SetDaRPos( int form , double pos );

  /** Sets the elevator position
      @param cmd elevator position in radians*/
  void SetDePos( int form , double pos );

  /** Sets the rudder position
      @param cmd rudder position in radians*/
  void SetDrPos( int form , double pos );

   /** Sets the flaps position
      @param cmd flaps position in radians*/
  void SetDfPos( int form , double pos );

  /** Sets the speedbrake position
      @param cmd speedbrake position in radians*/
  void SetDsbPos( int form , double pos );

  /** Sets the spoiler position
      @param cmd spoiler position in radians*/
  void SetDspPos( int form , double pos );

  /** Sets the actual throttle setting for the specified engine
      @param engine engine ID number
      @param cmd normalized throttle setting (0.0 - 1.0)*/
  void SetThrottlePos(int engine, double cmd);

  /** Sets the actual mixture setting for the specified engine
      @param engine engine ID number
      @param cmd normalized mixture setting
      @note The mixture position is typically within the range of 0 to 1.0, but
            some models can handle values above 1.0. In the default piston
            engine model, a mixture position of 1.0 corresponds to the full
            power setting, and the full rich setting is reached at a value
            higher than 1.0.
      @note JSBSim does not check nor enforce the value to be within 0.0-1.0.
  */
  void SetMixturePos(int engine, double cmd);

  /** Set the gear extend/retract position, defaults to down
      @param gear position 0 up, 1 down       */
   void SetGearPos(double gearpos) { GearPos = gearpos; }

  /** Set the tailhook position
      @param tailhook position 0 up, 1 down       */
   void SetTailhookPos(double hookpos) { TailhookPos = hookpos; }

  /** Set the wing fold position
      @param wing fold position 0 unfolded, 1 folded  */
   void SetWingFoldPos(double foldpos) { WingFoldPos = foldpos; }

  /** Sets the actual prop pitch setting for the specified engine
      @param engine engine ID number
      @param cmd prop pitch setting in percent (0.0 - 1.0)*/
  void SetPropAdvance(int engine, double cmd);

  /** Sets the actual prop feather setting for the specified engine
      @param engine engine ID number
      @param cmd prop fether setting (bool)*/
  void SetPropFeather(int engine, bool cmd);
  //@}

    /// @name Landing Gear brakes
  //@{
  /** Sets the left brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetLBrake(double cmd) {BrakePos[FGLGear::bgLeft] = cmd;}

  /** Sets the right brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetRBrake(double cmd) {BrakePos[FGLGear::bgRight] = cmd;}

  /** Sets the center brake group
      @param cmd brake setting in percent (0.0 - 1.0) */
  void SetCBrake(double cmd) {BrakePos[FGLGear::bgCenter] = cmd;}

  /** Gets the brake for a specified group.
      @param bg which brakegroup to retrieve the command for
      @return the brake setting for the supplied brake group argument */
  double GetBrake(FGLGear::BrakeGroup bg);

  const std::vector<double>& GetBrakePos() const {return BrakePos;}

  /** Gets the left brake.
      @return the left brake setting. */
  double GetLBrake(void) const {return BrakePos[FGLGear::bgLeft];}

  /** Gets the right brake.
      @return the right brake setting. */
  double GetRBrake(void) const {return BrakePos[FGLGear::bgRight];}

  /** Gets the center brake.
      @return the center brake setting. */
  double GetCBrake(void) const {return BrakePos[FGLGear::bgCenter];}
  //@}

  enum SystemType { stFCS, stSystem, stAutoPilot };

  /** Loads the Flight Control System.
      Load() is called from FGFDMExec.
      @param el pointer to the Element instance
      @return true if succesful */
  bool Load(Element* el) override;

  SGPath FindFullPathName(const SGPath& path) const override;

  void AddThrottle(void);
  double GetDt(void) const;

  std::shared_ptr<FGPropertyManager> GetPropertyManager(void) { return PropertyManager; }

  bool GetTrimStatus(void) const { return FDMExec->GetTrimStatus(); }
  double GetChannelDeltaT(void) const { return GetDt() * ChannelRate; }

private:
  double DaCmd, DeCmd, DrCmd, DfCmd, DsbCmd, DspCmd;
  double DePos[NForms], DaLPos[NForms], DaRPos[NForms], DrPos[NForms];
  double DfPos[NForms], DsbPos[NForms], DspPos[NForms];
  double PTrimCmd, YTrimCmd, RTrimCmd;
  std::vector <double> ThrottleCmd;
  std::vector <double> ThrottlePos;
  std::vector <double> MixtureCmd;
  std::vector <double> MixturePos;
  std::vector <double> PropAdvanceCmd;
  std::vector <double> PropAdvance;
  std::vector <bool> PropFeatherCmd;
  std::vector <bool> PropFeather;
  //double LeftBrake, RightBrake, CenterBrake; // Brake settings
  std::vector <double> BrakePos; // left, center, right - defined by FGLGear:: enum
  double GearCmd,GearPos;
  double TailhookPos, WingFoldPos;
  SystemType systype;
  int ChannelRate;

  typedef std::vector <FGFCSChannel*> Channels;
  Channels SystemChannels;
  void bind(void);
  void bindThrottle(unsigned int);
  void Debug(int from) override;
};
}

#endif
