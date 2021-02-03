/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGWinds.h
 Author:       Jon Berndt, Andreas Gaeb, David Culp
 Date started: 5/2011

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

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
5/2011   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGWINDS_H
#define FGWINDS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "models/FGModel.h"
#include "math/FGMatrix33.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGTable;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models atmospheric disturbances: winds, gusts, turbulence, downbursts, etc.

    <h2>Turbulence</h2>
    Various turbulence models are available. They are specified
    via the property <tt>atmosphere/turb-type</tt>. The following models are
    available:
    - 0: ttNone (turbulence disabled)
    - 1: ttStandard
    - 2: ttCulp
    - 3: ttMilspec (Dryden spectrum)
    - 4: ttTustin (Dryden spectrum)

    The Milspec and Tustin models are described in the Yeager report cited
    below.  They both use a Dryden spectrum model whose parameters (scale
    lengths and intensities) are modelled according to MIL-F-8785C. Parameters
    are modelled differently for altitudes below 1000ft and above 2000ft, for
    altitudes in between they are interpolated linearly.

    The two models differ in the implementation of the transfer functions
    described in the milspec.

    To use one of these two models, set <tt>atmosphere/turb-type</tt> to 4
    resp. 5, and specify values for
    <tt>atmosphere/turbulence/milspec/windspeed_at_20ft_AGL-fps</tt> and
    <tt>atmosphere/turbulence/milspec/severity</tt> (the latter corresponds to
    the probability of exceedence curves from Fig.&nbsp;7 of the milspec,
    allowable range is 0 (disabled) to 7). <tt>atmosphere/psiw-rad</tt> is
    respected as well; note that you have to specify a positive wind magnitude
    to prevent psiw from being reset to zero.

    Reference values (cf. figures 7 and 9 from the milspec):
    <table>
      <tr><td><b>Intensity</b></td>
          <td><b><tt>windspeed_at_20ft_AGL-fps</tt></b></td>
          <td><b><tt>severity</tt></b></td></tr>
      <tr><td>light</td>
          <td>25 (15 knots)</td>
          <td>3</td></tr>
      <tr><td>moderate</td>
          <td>50 (30 knots)</td>
          <td>4</td></tr>
      <tr><td>severe</td>
          <td>75 (45 knots)</td>
          <td>6</td></tr>
    </table>

    <h2>Cosine Gust</h2>
    A one minus cosine gust model is available. This permits a configurable,
    predictable gust to be input to JSBSim for testing handling and
    dynamics. Here is how a gust can be entered in a script:

    ~~~{.xml}
    <event name="Introduce gust">
      <condition> simulation/sim-time-sec ge 10 </condition>
      <set name="atmosphere/cosine-gust/startup-duration-sec" value="5"/>
      <set name="atmosphere/cosine-gust/steady-duration-sec" value="1"/>
      <set name="atmosphere/cosine-gust/end-duration-sec" value="5"/>
      <set name="atmosphere/cosine-gust/magnitude-ft_sec" value="30"/>
      <set name="atmosphere/cosine-gust/frame" value="2"/>
      <set name="atmosphere/cosine-gust/X-velocity-ft_sec" value="-1"/>
      <set name="atmosphere/cosine-gust/Y-velocity-ft_sec" value="0"/>
      <set name="atmosphere/cosine-gust/Z-velocity-ft_sec" value="0"/>
      <set name="atmosphere/cosine-gust/start" value="1"/>
      <notify/>
    </event>
    ~~~

    The x, y, z velocity components are meant to define the direction vector.
    The vector will be normalized by the routine, so it does not need to be a
    unit vector.

    The startup duration is the time it takes to build up to full strength
    (magnitude-ft_sec) from zero. Steady duration is the time the gust stays at
    the specified magnitude. End duration is the time it takes to dwindle to
    zero from the specified magnitude. The start and end transients are in a
    smooth cosine shape.

    The frame is specified from the following enum:

    enum eGustFrame {gfNone=0, gfBody, gfWind, gfLocal};

    That is, if you specify the X, Y, Z gust direction vector in the body frame,
    frame will be "1". If the X, Y, and Z gust direction vector is in the Wind
    frame, use frame = 2. If you specify the gust direction vector in the local
    frame (N-E-D) use frame = 3. Note that an internal local frame direction
    vector is created based on the X, Y, Z direction vector you specify and the
    frame *at the time the gust is begun*. The direction vector is not updated
    after the initial creation. This is to keep the gust at the same direction
    independent of aircraft dynamics.

    The gust is triggered when the property atmosphere/cosine-gust/start is set
    to 1. It can be used repeatedly - the gust resets itself after it has
    completed.

    The cosine gust is global: it affects the whole world not just the vicinity
    of the aircraft.

    @see Yeager, Jessie C.: "Implementation and Testing of Turbulence Models for
         the F18-HARV" (<a
         href="http://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19980028448_1998081596.pdf">
         pdf</a>), NASA CR-1998-206937, 1998

    @see MIL-F-8785C: Military Specification: Flying Qualities of Piloted Aircraft

*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGWinds : public FGModel {
public:

  /// Constructor
  explicit FGWinds(FGFDMExec*);
  /// Destructor
  ~FGWinds();
  /** Runs the winds model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim
                     from advancing time. Some models may ignore this flag, such
                     as the Input model, which may need to be active to listen
                     on a socket for the "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;
  bool InitModel(void) override;
  enum tType {ttNone, ttStandard, ttCulp, ttMilspec, ttTustin} turbType;

  // TOTAL WIND access functions (wind + gust + turbulence)

  /// Retrieves the total wind components in NED frame.
  virtual const FGColumnVector3& GetTotalWindNED(void) const { return vTotalWindNED; }

  /// Retrieves a total wind component in NED frame.
  virtual double GetTotalWindNED(int idx) const {return vTotalWindNED(idx);}

  // WIND access functions

  /// Sets the wind components in NED frame.
  virtual void SetWindNED(double wN, double wE, double wD) { vWindNED(1)=wN; vWindNED(2)=wE; vWindNED(3)=wD;}

  /// Sets a wind component in NED frame.
  virtual void SetWindNED(int idx, double wind) { vWindNED(idx)=wind;}

  /// Sets the wind components in NED frame.
  virtual void SetWindNED(const FGColumnVector3& wind) { vWindNED=wind; }

  /// Retrieves the wind components in NED frame.
  virtual const FGColumnVector3& GetWindNED(void) const { return vWindNED; }

  /// Retrieves a wind component in NED frame.
  virtual double GetWindNED(int idx) const {return vWindNED(idx);}

  /** Retrieves the direction that the wind is coming from.
      The direction is defined as north=0 and increases counterclockwise.
      The wind heading is returned in radians.*/
  virtual double GetWindPsi(void) const { return psiw; }

  /** Sets the direction that the wind is coming from.
      The direction is defined as north=0 and increases counterclockwise to 2*pi
      (radians). The vertical component of wind is assumed to be zero - and is
      forcibly set to zero. This function sets the vWindNED vector components
      based on the supplied direction. The magnitude of the wind set in the
      vector is preserved (assuming the vertical component is non-zero).
      @param dir wind direction in the horizontal plane, in radians.*/
  virtual void SetWindPsi(double dir);

  virtual void SetWindspeed(double speed);

  virtual double GetWindspeed(void) const;

  // GUST access functions

  /// Sets a gust component in NED frame.
  virtual void SetGustNED(int idx, double gust) { vGustNED(idx)=gust;}

  /// Sets a turbulence component in NED frame.
  virtual void SetTurbNED(int idx, double turb) { vTurbulenceNED(idx)=turb;}

  /// Sets the gust components in NED frame.
  virtual void SetGustNED(double gN, double gE, double gD) { vGustNED(eNorth)=gN; vGustNED(eEast)=gE; vGustNED(eDown)=gD;}

  /// Retrieves a gust component in NED frame.
  virtual double GetGustNED(int idx) const {return vGustNED(idx);}

  /// Retrieves a turbulence component in NED frame.
  virtual double GetTurbNED(int idx) const {return vTurbulenceNED(idx);}

  /// Retrieves the gust components in NED frame.
  virtual const FGColumnVector3& GetGustNED(void) const {return vGustNED;}

  /** Turbulence models available: ttNone, ttStandard, ttBerndt, ttCulp,
      ttMilspec, ttTustin */
  virtual void   SetTurbType(tType tt) {turbType = tt;}
  virtual tType  GetTurbType() const {return turbType;}

  virtual void   SetTurbGain(double tg) {TurbGain = tg;}
  virtual double GetTurbGain() const {return TurbGain;}

  virtual void   SetTurbRate(double tr) {TurbRate = tr;}
  virtual double GetTurbRate() const {return TurbRate;}

  virtual void   SetRhythmicity(double r) {Rhythmicity=r;}
  virtual double GetRhythmicity() const {return Rhythmicity;}

  virtual double GetTurbPQR(int idx) const {return vTurbPQR(idx);}
  virtual double GetTurbMagnitude(void) const {return vTurbulenceNED.Magnitude();}
  virtual double GetTurbDirection(void) const {return TurbDirection;}
  virtual const FGColumnVector3& GetTurbPQR(void) const {return vTurbPQR;}

  virtual void   SetWindspeed20ft(double ws) { windspeed_at_20ft = ws;}
  virtual double GetWindspeed20ft() const { return windspeed_at_20ft;}

  /// allowable range: 0-7, 3=light, 4=moderate, 6=severe turbulence
  virtual void   SetProbabilityOfExceedence( int idx) {probability_of_exceedence_index = idx;}
  virtual int    GetProbabilityOfExceedence() const { return probability_of_exceedence_index;}

  // Stores data defining a 1 - cosine gust profile that builds up, holds steady
  // and fades out over specified durations.
  struct OneMinusCosineProfile {
    bool Running;           ///<- This flag is set true through FGWinds::StartGust().
    double elapsedTime;     ///<- Stores the elapsed time for the ongoing gust.
    double startupDuration; ///<- Specifies the time it takes for the gust startup transient.
    double steadyDuration;  ///<- Specifies the duration of the steady gust.
    double endDuration;     ///<- Specifies the time it takes for the gust to subsude.
    OneMinusCosineProfile() ///<- The constructor.
    {
      elapsedTime = 0.0;
      Running = false;
      startupDuration = 2;
      steadyDuration = 4;
      endDuration = 2;
    }
  };

  enum eGustFrame {gfNone=0, gfBody, gfWind, gfLocal};

  /// Stores the information about a single one minus cosine gust instance.
  struct OneMinusCosineGust {
    FGColumnVector3 vWind;                    ///<- The input normalized wind vector.
    FGColumnVector3 vWindTransformed;         ///<- The transformed normal vector at the time the gust is started.
    double magnitude;                         ///<- The magnitude of the wind vector.
    eGustFrame gustFrame;                     ///<- The frame that the wind vector is specified in.
    struct OneMinusCosineProfile gustProfile; ///<- The gust shape (profile) data for this gust.
    OneMinusCosineGust()                      ///<- Constructor.
    {
      vWind.InitMatrix(0.0);
      gustFrame = gfLocal;
      magnitude = 1.0;
    };
  };

  /// Stores information about a specified Up- or Down-burst.
  struct UpDownBurst {
    double ringLatitude;                           ///<- The latitude of the downburst run (radians)
    double ringLongitude;                          ///<- The longitude of the downburst run (radians)
    double ringAltitude;                           ///<- The altitude of the ring (feet).
    double ringRadius;                             ///<- The radius of the ring (feet).
    double ringCoreRadius;                         ///<- The cross-section "core" radius of the ring (feet).
    double circulation;                            ///<- The circulation (gamma) (feet-squared per second).
    struct OneMinusCosineProfile oneMCosineProfile;///<- A gust profile structure.
    UpDownBurst() {                                ///<- Constructor
      ringLatitude = ringLongitude = 0.0;
      ringAltitude = 1000.0;
      ringRadius = 2000.0;
      ringCoreRadius = 100.0;
      circulation = 100000.0;
    }
  };

  // 1 - Cosine gust setters
  /// Initiates the execution of the gust.
  virtual void StartGust(bool running) {oneMinusCosineGust.gustProfile.Running = running;}
  ///Specifies the duration of the startup portion of the gust.
  virtual void StartupGustDuration(double dur) {oneMinusCosineGust.gustProfile.startupDuration = dur;}
  ///Specifies the length of time that the gust is at a steady, full strength.
  virtual void SteadyGustDuration(double dur) {oneMinusCosineGust.gustProfile.steadyDuration = dur;}
  /// Specifies the length of time it takes for the gust to return to zero velocity.
  virtual void EndGustDuration(double dur) {oneMinusCosineGust.gustProfile.endDuration = dur;}
  /// Specifies the magnitude of the gust in feet/second.
  virtual void GustMagnitude(double mag) {oneMinusCosineGust.magnitude = mag;}
  /** Specifies the frame that the gust direction vector components are specified in. The 
      body frame is defined with the X direction forward, and the Y direction positive out
      the right wing. The wind frame is defined with the X axis pointing into the velocity
      vector, the Z axis perpendicular to the X axis, in the aircraft XZ plane, and the Y
      axis completing the system. The local axis is a navigational frame with X pointing north,
      Y pointing east, and Z pointing down. This is a locally vertical, locally horizontal
      frame, with the XY plane tangent to the geocentric surface. */
  virtual void GustFrame(eGustFrame gFrame) {oneMinusCosineGust.gustFrame = gFrame;}
  /// Specifies the X component of velocity in the specified gust frame (ft/sec).
  virtual void GustXComponent(double x) {oneMinusCosineGust.vWind(eX) = x;}
  /// Specifies the Y component of velocity in the specified gust frame (ft/sec).
  virtual void GustYComponent(double y) {oneMinusCosineGust.vWind(eY) = y;}
  /// Specifies the Z component of velocity in the specified gust frame (ft/sec).
  virtual void GustZComponent(double z) {oneMinusCosineGust.vWind(eZ) = z;}

  // Up- Down-burst functions
  void NumberOfUpDownburstCells(int num);

  struct Inputs {
    double V;
    double wingspan;
    double DistanceAGL;
    double AltitudeASL;
    double longitude;
    double latitude;
    double planetRadius;
    FGMatrix33 Tl2b;
    FGMatrix33 Tw2b;
    double totalDeltaT;
  } in;

private:

  double MagnitudedAccelDt, MagnitudeAccel, Magnitude, TurbDirection;
  //double h;
  double TurbGain;
  double TurbRate;
  double Rhythmicity;
  double wind_from_clockwise;
  double spike, target_time, strength;
  FGColumnVector3 vTurbulenceGrad;
  FGColumnVector3 vBodyTurbGrad;
  FGColumnVector3 vTurbPQR;

  struct OneMinusCosineGust oneMinusCosineGust;
  std::vector <struct UpDownBurst*> UpDownBurstCells;

  // Dryden turbulence model
  double windspeed_at_20ft; ///< in ft/s
  int probability_of_exceedence_index; ///< this is bound as the severity property
  FGTable *POE_Table; ///< probability of exceedence table

  double psiw;
  FGColumnVector3 vTotalWindNED;
  FGColumnVector3 vWindNED;
  FGColumnVector3 vGustNED;
  FGColumnVector3 vCosineGust;
  FGColumnVector3 vBurstGust;
  FGColumnVector3 vTurbulenceNED;

  void Turbulence(double h);
  void UpDownBurst();

  void CosineGust();
  double CosineGustProfile( double startDuration, double steadyDuration,
                            double endDuration, double elapsedTime);
  double DistanceFromRingCenter(double lat, double lon);

  virtual void bind(void);
  void Debug(int from) override;
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

