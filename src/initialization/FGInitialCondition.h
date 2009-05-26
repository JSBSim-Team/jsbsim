/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGInitialCondition.h
 Author:       Tony Peden
 Date started: 7/1/99

 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------

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
7/1/99   TP   Created

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

The purpose of this class is to take a set of initial conditions and provide
a kinematically consistent set of body axis velocity components, euler
angles, and altitude.  This class does not attempt to trim the model i.e.
the sim will most likely start in a very dynamic state (unless, of course,
you have chosen your IC's wisely) even after setting it up with this class.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGINITIALCONDITION_H
#define FGINITIALCONDITION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <FGFDMExec.h>
#include <FGJSBBase.h>
#include <math/FGColumnVector3.h>
#include <input_output/FGXMLFileRead.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_INITIALCONDITION "$Id: FGInitialCondition.h,v 1.17 2009/05/26 05:35:42 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

typedef enum { setvt, setvc, setve, setmach, setuvw, setned, setvg } speedset;
typedef enum { setwned, setwmd, setwhc } windset;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Initializes the simulation run.
    Takes a set of initial conditions (IC) and provide a kinematically consistent set
    of body axis velocity components, euler angles, and altitude.  This class
    does not attempt to trim the model i.e. the sim will most likely start in a
    very dynamic state (unless, of course, you have chosen your IC's wisely, or
    started on the ground) even after setting it up with this class.

   <h3>Usage Notes</h3>

   With a valid object of FGFDMExec and an aircraft model loaded:

   @code
   FGInitialCondition fgic=new FGInitialCondition(FDMExec);
   fgic->SetVcalibratedKtsIC()
   fgic->SetAltitudeAGLFtIC();

   // directly into Run
   FDMExec->GetState()->Initialize(fgic)
   delete fgic;
   FDMExec->Run()

   //or to loop the sim w/o integrating
   FDMExec->RunIC(fgic)
   @endcode

   <h3>Speed</h3>

   Since vc, ve, vt, and mach all represent speed, the remaining
   three are recalculated each time one of them is set (using the
   current altitude).  The most recent speed set is remembered so
   that if and when altitude is reset, the last set speed is used
   to recalculate the remaining three. Setting any of the body
   components forces a recalculation of vt and vt then becomes the
   most recent speed set.

   <h3>Alpha,Gamma, and Theta</h3>

   This class assumes that it will be used to set up the sim for a
   steady, zero pitch rate condition. Since any two of those angles
   specifies the third gamma (flight path angle) is favored when setting
   alpha and theta and alpha is favored when setting gamma. i.e.

   - set alpha : recalculate theta using gamma as currently set
   - set theta : recalculate alpha using gamma as currently set
   - set gamma : recalculate theta using alpha as currently set

   The idea being that gamma is most interesting to pilots (since it
   is indicative of climb rate).

   Setting climb rate is, for the purpose of this discussion,
   considered equivalent to setting gamma.

   These are the items that can be set in an initialization file:

   - ubody (velocity, ft/sec)
   - vbody (velocity, ft/sec)
   - wbody (velocity, ft/sec)
   - vnorth (velocity, ft/sec)
   - veast (velocity, ft/sec)
   - vdown (velocity, ft/sec)
   - latitude (position, degrees)
   - longitude (position, degrees)
   - phi (orientation, degrees)
   - theta (orientation, degrees)
   - psi (orientation, degrees)
   - alpha (angle, degrees)
   - beta (angle, degrees)
   - gamma (angle, degrees)
   - roc (vertical velocity, ft/sec)
   - altitude (altitude AGL, ft)
   - winddir (wind from-angle, degrees)
   - vwind (magnitude wind speed, ft/sec)
   - hwind (headwind speed, knots)
   - xwind (crosswind speed, knots)
   - vc (calibrated airspeed, ft/sec)
   - mach (mach)
   - vground (ground speed, ft/sec)
   - running (0 or 1)

   <h3>Properties</h3>
   @property ic/vc-kts (read/write) Calibrated airspeed initial condition in knots
   @property ic/ve-kts (read/write) Knots equivalent airspeed initial condition
   @property ic/vg-kts (read/write) Ground speed initial condition in knots
   @property ic/vt-kts (read/write) True airspeed initial condition in knots
   @property ic/mach (read/write) Mach initial condition
   @property ic/roc-fpm (read/write) Rate of climb initial condition in feet/minute
   @property ic/gamma-deg (read/write) Flightpath angle initial condition in degrees
   @property ic/alpha-deg (read/write) Angle of attack initial condition in degrees
   @property ic/beta-deg (read/write) Angle of sideslip initial condition in degrees
   @property ic/theta-deg (read/write) Pitch angle initial condition in degrees
   @property ic/phi-deg (read/write) Roll angle initial condition in degrees
   @property ic/psi-true-deg (read/write) Heading angle initial condition in degrees
   @property ic/lat-gc-deg (read/write) Latitude initial condition in degrees
   @property ic/long-gc-deg (read/write) Longitude initial condition in degrees
   @property ic/h-sl-ft (read/write) Height above sea level initial condition in feet
   @property ic/h-agl-ft (read/write) Height above ground level initial condition in feet
   @property ic/sea-level-radius-ft (read/write) Radius of planet at sea level in feet
   @property ic/terrain-elevation-ft (read/write) Terrain elevation above sea level in feet
   @property ic/vg-fps (read/write) Ground speed initial condition in feet/second
   @property ic/vt-fps (read/write) True airspeed initial condition in feet/second
   @property ic/vw-bx-fps (read/write) Wind velocity initial condition in Body X frame in feet/second
   @property ic/vw-by-fps (read/write) Wind velocity initial condition in Body Y frame in feet/second
   @property ic/vw-bz-fps (read/write) Wind velocity initial condition in Body Z frame in feet/second
   @property ic/vw-north-fps (read/write) Wind northward velocity initial condition in feet/second
   @property ic/vw-east-fps (read/write) Wind eastward velocity initial condition in feet/second
   @property ic/vw-down-fps (read/write) Wind downward velocity initial condition in feet/second
   @property ic/vw-mag-fps (read/write) Wind velocity magnitude initial condition in feet/sec.
   @property ic/vw-dir-deg (read/write) Wind direction initial condition, in degrees from north
   @property ic/roc-fps (read/write) Rate of climb initial condition, in feet/second
   @property ic/u-fps (read/write) Body frame x-axis velocity initial condition in feet/second
   @property ic/v-fps (read/write) Body frame y-axis velocity initial condition in feet/second
   @property ic/w-fps (read/write) Body frame z-axis velocity initial condition in feet/second
   @property ic/vn-fps (read/write) Local frame x-axis (north) velocity initial condition in feet/second
   @property ic/ve-fps (read/write) Local frame y-axis (east) velocity initial condition in feet/second
   @property ic/vd-fps (read/write) Local frame z-axis (down) velocity initial condition in feet/second
   @property ic/gamma-rad (read/write) Flight path angle initial condition in radians
   @property ic/alpha-rad (read/write) Angle of attack initial condition in radians
   @property ic/theta-rad (read/write) Pitch angle initial condition in radians
   @property ic/beta-rad (read/write) Angle of sideslip initial condition in radians
   @property ic/phi-rad (read/write) Roll angle initial condition in radians
   @property ic/psi-true-rad (read/write) Heading angle initial condition in radians
   @property ic/lat-gc-rad (read/write) Geocentric latitude initial condition in radians
   @property ic/long-gc-rad (read/write) Longitude initial condition in radians
   @property ic/p-rad_sec (read/write) Roll rate initial condition in radians/second
   @property ic/q-rad_sec (read/write) Pitch rate initial condition in radians/second
   @property ic/r-rad_sec (read/write) Yaw rate initial condition in radians/second

   @author Tony Peden
   @version "$Id: FGInitialCondition.h,v 1.17 2009/05/26 05:35:42 jberndt Exp $"
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGInitialCondition : public FGJSBBase, public FGXMLFileRead
{
public:
  /// Constructor
  FGInitialCondition(FGFDMExec *fdmex);
  /// Destructor
  ~FGInitialCondition();

  /** Set calibrated airspeed initial condition in knots.
      @param vc Calibrated airspeed in knots  */
  void SetVcalibratedKtsIC(double vc);

  /** Set equivalent airspeed initial condition in knots.
      @param ve Equivalent airspeed in knots  */
  void SetVequivalentKtsIC(double ve);

  /** Set true airspeed initial condition in knots.
      @param vt True airspeed in knots  */
  inline void SetVtrueKtsIC(double vt)   { SetVtrueFpsIC(vt*ktstofps);   }

  /** Set ground speed initial condition in knots.
      @param vg Ground speed in knots  */
  inline void SetVgroundKtsIC(double vg) { SetVgroundFpsIC(vg*ktstofps); }

  /** Set mach initial condition.
      @param mach Mach number  */
  void SetMachIC(double mach);

  /** Sets angle of attack initial condition in degrees.
      @param a Alpha in degrees */
  inline void SetAlphaDegIC(double a)      { SetAlphaRadIC(a*degtorad); }

  /** Sets angle of sideslip initial condition in degrees.
      @param B Beta in degrees */
  inline void SetBetaDegIC(double B)       { SetBetaRadIC(B*degtorad);}

  /** Sets pitch angle initial condition in degrees.
      @param theta Theta (pitch) angle in degrees */
  inline void SetThetaDegIC(double theta) { SetThetaRadIC(theta*degtorad); }

  /** Resets the IC data structure to new values
      @param u, v, w, ... **/
  void ResetIC(double u0, double v0, double w0, double p0, double q0, double r0,
               double alpha0, double beta0, double phi0, double theta0, double psi0,
               double latitudeRad0, double longitudeRad0, double altitudeAGL0,
               double gamma0);

  /** Sets the roll angle initial condition in degrees.
      @param phi roll angle in degrees */
  inline void SetPhiDegIC(double phi)  { SetPhiRadIC(phi*degtorad);}

  /** Sets the heading angle initial condition in degrees.
      @param psi Heading angle in degrees */
  inline void SetPsiDegIC(double psi){ SetPsiRadIC(psi*degtorad); }

  /** Sets the climb rate initial condition in feet/minute.
      @param roc Rate of Climb in feet/minute  */
  void SetClimbRateFpmIC(double roc);

  /** Sets the flight path angle initial condition in degrees.
      @param gamma Flight path angle in degrees  */
  inline void SetFlightPathAngleDegIC(double gamma) { SetFlightPathAngleRadIC(gamma*degtorad); }

  /** Sets the altitude above sea level initial condition in feet.
      @param altitudeASL Altitude above sea level in feet */
  void SetAltitudeASLFtIC(double altitudeASL);

  /** Sets the initial Altitude above ground level.
      @param agl Altitude above ground level in feet */
  void SetAltitudeAGLFtIC(double agl);

  /** Sets the initial sea level radius from planet center
      @param sl_rad sea level radius in feet */
  void SetSeaLevelRadiusFtIC(double sl_rad);

  /** Sets the initial terrain elevation.
      @param elev Initial terrain elevation in feet */
  void SetTerrainElevationFtIC(double elev);

  /** Sets the initial latitude.
      @param lat Initial latitude in degrees */
  inline void SetLatitudeDegIC(double lat)  { latitude=lat*degtorad; }

  /** Sets the initial longitude.
      @param lon Initial longitude in degrees */
  inline void SetLongitudeDegIC(double lon) { longitude=lon*degtorad; }

  /** Gets the initial calibrated airspeed.
      @return Initial calibrated airspeed in knots */
  inline double GetVcalibratedKtsIC(void) const { return vc*fpstokts; }

  /** Gets the initial equivalent airspeed.
      @return Initial equivalent airspeed in knots */
  inline double GetVequivalentKtsIC(void) const { return ve*fpstokts; }

  /** Gets the initial ground speed.
      @return Initial ground speed in knots */
  inline double GetVgroundKtsIC(void) const { return vg*fpstokts; }

  /** Gets the initial true velocity.
      @return Initial true airspeed in knots. */
  inline double GetVtrueKtsIC(void) const { return vt*fpstokts; }

  /** Gets the initial mach.
      @return Initial mach number */
  inline double GetMachIC(void) const { return mach; }

  /** Gets the initial climb rate.
      @return Initial climb rate in feet/minute */
  inline double GetClimbRateFpmIC(void) const { return hdot*60; }

  /** Gets the initial flight path angle.
      @return Initial flight path angle in degrees */
  inline double GetFlightPathAngleDegIC(void)const  { return gamma*radtodeg; }

  /** Gets the initial angle of attack.
      @return Initial alpha in degrees */
  inline double GetAlphaDegIC(void) const { return alpha*radtodeg; }

  /** Gets the initial sideslip angle.
      @return Initial beta in degrees */
  inline double GetBetaDegIC(void) const  { return beta*radtodeg; }

  /** Gets the initial pitch angle.
      @return Initial pitch angle in degrees */
  inline double GetThetaDegIC(void) const { return theta*radtodeg; }

  /** Gets the initial roll angle.
      @return Initial phi in degrees */
  inline double GetPhiDegIC(void) const { return phi*radtodeg; }

  /** Gets the initial heading angle.
      @return Initial psi in degrees */
  inline double GetPsiDegIC(void) const { return psi*radtodeg; }

  /** Gets the initial latitude.
      @return Initial geocentric latitude in degrees */
  inline double GetLatitudeDegIC(void) const { return latitude*radtodeg; }

  /** Gets the initial longitude.
      @return Initial longitude in degrees */
  inline double GetLongitudeDegIC(void) const { return longitude*radtodeg; }

  /** Gets the initial altitude above sea level.
      @return Initial altitude in feet. */
  inline double GetAltitudeASLFtIC(void) const { return altitudeASL; }

  /** Gets the initial altitude above ground level.
      @return Initial altitude AGL in feet */
  inline double GetAltitudeAGLFtIC(void) const { return altitudeASL - terrain_elevation; }

  /** Gets the initial sea level radius.
      @return Initial sea level radius */
  inline double GetSeaLevelRadiusFtIC(void) const { return sea_level_radius; }

  /** Gets the initial terrain elevation.
      @return Initial terrain elevation in feet */
  inline double GetTerrainElevationFtIC(void) const { return terrain_elevation; }

  /** Sets the initial ground speed.
      @param vg Initial ground speed in feet/second */
  void SetVgroundFpsIC(double vg);

  /** Sets the initial true airspeed.
      @param vt Initial true airspeed in feet/second */
  void SetVtrueFpsIC(double vt);

  /** Sets the initial body axis X velocity.
      @param ubody Initial X velocity in feet/second */
  void SetUBodyFpsIC(double ubody);

  /** Sets the initial body axis Y velocity.
      @param vbody Initial Y velocity in feet/second */
  void SetVBodyFpsIC(double vbody);

  /** Sets the initial body axis Z velocity.
      @param wbody Initial Z velocity in feet/second */
  void SetWBodyFpsIC(double wbody);

  /** Sets the initial local axis north velocity.
      @param vn Initial north velocity in feet/second */
  void SetVNorthFpsIC(double vn);

  /** Sets the initial local axis east velocity.
      @param ve Initial east velocity in feet/second */
  void SetVEastFpsIC(double ve);

  /** Sets the initial local axis down velocity.
      @param vd Initial down velocity in feet/second */
  void SetVDownFpsIC(double vd);

  /** Sets the initial roll rate.
      @param P Initial roll rate in radians/second */
  void SetPRadpsIC(double P)  { p = P; }

  /** Sets the initial pitch rate.
      @param Q Initial pitch rate in radians/second */
  void SetQRadpsIC(double Q) { q = Q; }

  /** Sets the initial yaw rate.
      @param R initial yaw rate in radians/second */
  void SetRRadpsIC(double R) { r = R; }

  /** Sets the initial wind velocity.
      @param wN Initial wind velocity in local north direction, feet/second
      @param wE Initial wind velocity in local east direction, feet/second
      @param wD Initial wind velocity in local down direction, feet/second   */
  void SetWindNEDFpsIC(double wN, double wE, double wD);

  /** Sets the initial total wind speed.
      @param mag Initial wind velocity magnitude in knots */
  void SetWindMagKtsIC(double mag);

  /** Sets the initial wind direction.
      @param dir Initial direction wind is coming from in degrees */
  void SetWindDirDegIC(double dir);

  /** Sets the initial headwind velocity.
      @param head Initial headwind speed in knots */
  void SetHeadWindKtsIC(double head);

  /** Sets the initial crosswind speed.
      @param cross Initial crosswind speed, positive from left to right */
  void SetCrossWindKtsIC(double cross);

  /** Sets the initial wind downward speed.
      @param wD Initial downward wind speed in knots*/
  void SetWindDownKtsIC(double wD);

  /** Sets the initial climb rate.
      @param roc Initial Rate of climb in feet/second */
  void SetClimbRateFpsIC(double roc);

  /** Gets the initial ground velocity.
      @return Initial ground velocity in feet/second */
  inline double GetVgroundFpsIC(void) const  { return vg; }

  /** Gets the initial true velocity.
      @return Initial true velocity in feet/second */
  inline double GetVtrueFpsIC(void) const { return vt; }

  /** Gets the initial body axis X wind velocity.
      @return Initial body axis X wind velocity in feet/second */
  inline double GetWindUFpsIC(void) const { return uw; }

  /** Gets the initial body axis Y wind velocity.
      @return Initial body axis Y wind velocity in feet/second */
  inline double GetWindVFpsIC(void) const { return vw; }

  /** Gets the initial body axis Z wind velocity.
      @return Initial body axis Z wind velocity in feet/second */
  inline double GetWindWFpsIC(void) const { return ww; }

  /** Gets the initial wind velocity in local frame.
      @return Initial wind velocity toward north in feet/second */
  inline double GetWindNFpsIC(void) const { return wnorth; }

  /** Gets the initial wind velocity in local frame.
      @return Initial wind velocity eastwards in feet/second */
  inline double GetWindEFpsIC(void) const { return weast; }

  /** Gets the initial wind velocity in local frame.
      @return Initial wind velocity downwards in feet/second */
  inline double GetWindDFpsIC(void) const { return wdown; }

  /** Gets the initial total wind velocity in feet/sec.
      @return Initial wind velocity in feet/second */
  inline double GetWindFpsIC(void)  const { return sqrt(wnorth*wnorth + weast*weast); }

  /** Gets the initial wind direction.
      @return Initial wind direction in feet/second */
  double GetWindDirDegIC(void) const;

  /** Gets the initial climb rate.
      @return Initial rate of climb in feet/second */
  inline double GetClimbRateFpsIC(void) const { return hdot; }

  /** Gets the initial body axis X velocity.
      @return Initial body axis X velocity in feet/second. */
  double GetUBodyFpsIC(void) const;

  /** Gets the initial body axis Y velocity.
      @return Initial body axis Y velocity in feet/second. */
  double GetVBodyFpsIC(void) const;

  /** Gets the initial body axis Z velocity.
      @return Initial body axis Z velocity in feet/second. */
  double GetWBodyFpsIC(void) const;

  /** Gets the initial local frame X (North) velocity.
      @return Initial local frame X (North) axis velocity in feet/second. */
  double GetVNorthFpsIC(void) const {return vnorth;};

  /** Gets the initial local frame Y (East) velocity.
      @return Initial local frame Y (East) axis velocity in feet/second. */
  double GetVEastFpsIC(void) const {return veast;};

  /** Gets the initial local frame Z (Down) velocity.
      @return Initial local frame Z (Down) axis velocity in feet/second. */
  double GetVDownFpsIC(void) const {return vdown;};

  /** Gets the initial body axis roll rate.
      @return Initial body axis roll rate in radians/second */
  double GetPRadpsIC() const { return p; }

  /** Gets the initial body axis pitch rate.
      @return Initial body axis pitch rate in radians/second */
  double GetQRadpsIC() const { return q; }

  /** Gets the initial body axis yaw rate.
      @return Initial body axis yaw rate in radians/second */
  double GetRRadpsIC() const { return r; }

  /** Sets the initial flight path angle.
      @param gamma Initial flight path angle in radians */
  void SetFlightPathAngleRadIC(double gamma);

  /** Sets the initial angle of attack.
      @param alpha Initial angle of attack in radians */
  void SetAlphaRadIC(double alpha);

  /** Sets the initial pitch angle.
      @param theta Initial pitch angle in radians */
  void SetThetaRadIC(double theta);

  /** Sets the initial sideslip angle.
      @param beta Initial angle of sideslip in radians. */
  void SetBetaRadIC(double beta);

  /** Sets the initial roll angle.
      @param phi Initial roll angle in radians */
  void SetPhiRadIC(double phi);

  /** Sets the initial heading angle.
      @param psi Initial heading angle in radians */
  void SetPsiRadIC(double psi);

  /** Sets the initial latitude.
      @param lat Initial latitude in radians */
  inline void SetLatitudeRadIC(double lat) { latitude=lat; }

  /** Sets the initial longitude.
      @param lon Initial longitude in radians */
  inline void SetLongitudeRadIC(double lon) { longitude=lon; }

  /** Sets the target normal load factor.
      @param nlf Normal load factor*/
  inline void SetTargetNlfIC(double nlf) { targetNlfIC=nlf; }

  /** Gets the initial flight path angle.
      @return Initial flight path angle in radians */
  inline double GetFlightPathAngleRadIC(void) const { return gamma; }

  /** Gets the initial angle of attack.
      @return Initial alpha in radians */
  inline double GetAlphaRadIC(void) const      { return alpha; }

  /** Gets the initial angle of sideslip.
      @return Initial sideslip angle in radians */
  inline double GetBetaRadIC(void) const       { return beta; }

  /** Gets the initial roll angle.
      @return Initial roll angle in radians */
  inline double GetPhiRadIC(void) const  { return phi; }

  /** Gets the initial latitude.
      @return Initial latitude in radians */
  inline double GetLatitudeRadIC(void) const { return latitude; }

  /** Gets the initial longitude.
      @return Initial longitude in radians */
  inline double GetLongitudeRadIC(void) const { return longitude; }

  /** Gets the initial pitch angle.
      @return Initial pitch angle in radians */
  inline double GetThetaRadIC(void) const { return theta; }

  /** Gets the initial heading angle.
      @return Initial heading angle in radians */
  inline double GetPsiRadIC(void) const   { return psi; }

  /** Gets the initial speedset.
      @return Initial speedset */
  inline speedset GetSpeedSet(void) { return lastSpeedSet; }

  /** Gets the initial windset.
      @return Initial windset */
  inline windset GetWindSet(void) { return lastWindSet; }

  /** Gets the target normal load factor set from IC.
      @return target normal load factor set from IC*/
  inline double GetTargetNlfIC(void) { return targetNlfIC; }

  /** Loads the initial conditions.
      @param rstname The name of an initial conditions file
      @param useStoredPath true if the stored path to the IC file should be used
      @return true if successful */
  bool Load(string rstname, bool useStoredPath = true );

  /** Get init-file name
  */
  string GetInitFile(void) {return init_file_name;}
  /** Set init-file name
  */
  void SetInitFile(string f) { init_file_name = f;}
  void WriteStateFile(int num);

private:
  double vt,vc,ve,vg;
  double mach;
  double altitudeASL,hdot;
  double latitude,longitude;
  double u,v,w;
  double p,q,r;
  double uw,vw,ww;
  double vnorth,veast,vdown;
  double wnorth,weast,wdown;
  double whead, wcross, wdir, wmag;
  double sea_level_radius;
  double terrain_elevation;
  double radius_to_vehicle;
  double targetNlfIC;

  double  alpha, beta, theta, phi, psi, gamma;
  double salpha,sbeta,stheta,sphi,spsi,sgamma;
  double calpha,cbeta,ctheta,cphi,cpsi,cgamma;

  double xlo, xhi,xmin,xmax;

  typedef double (FGInitialCondition::*fp)(double x);
  fp sfunc;

  speedset lastSpeedSet;
  windset lastWindSet;

  FGFDMExec *fdmex;
  FGPropertyManager *PropertyManager;

  bool Constructing;
  bool getAlpha(void);
  bool getTheta(void);
  bool getMachFromVcas(double *Mach,double vcas);

  double GammaEqOfTheta(double Theta);
  void InitializeIC(void);
  double GammaEqOfAlpha(double Alpha);
  double calcVcas(double Mach);
  void calcUVWfromNED(void);
  void calcWindUVW(void);

  bool findInterval(double x,double guess);
  bool solve(double *y, double x);
  void bind(void);
  void Debug(int from);

  string init_file_name;
};
}
#endif

