/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAuxiliary.h
 Author:       Jon Berndt
 Date started: 01/26/99

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
11/22/98   JSB   Created
  1/1/00   TP    Added calcs and getters for VTAS, VCAS, VEAS, Vground, in knots

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGAUXILIARY_H
#define FGAUXILIARY_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGColumnVector3.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_AUXILIARY "$Id: FGAuxiliary.h,v 1.38 2004/03/23 12:32:53 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates various uncategorized scheduled functions.
    Pilot sensed accelerations are calculated here. This is used
    for the coordinated turn ball instrument. Motion base platforms sometimes
    use the derivative of pilot sensed accelerations as the driving parameter,
    rather than straight accelerations.

    The theory behind pilot-sensed calculations is presented:

    For purposes of discussion and calculation, assume for a minute that the
    pilot is in space and motionless in inertial space. She will feel
    no accelerations. If the aircraft begins to accelerate along any axis or
    axes (without rotating), the pilot will sense those accelerations. If
    any rotational moment is applied, the pilot will sense an acceleration
    due to that motion in the amount:

    [wdot X R]  +  [w X (w X R)]
    Term I          Term II

    where:

    wdot = omegadot, the rotational acceleration rate vector
    w    = omega, the rotational rate vector
    R    = the vector from the aircraft CG to the pilot eyepoint

    The sum total of these two terms plus the acceleration of the aircraft
    body axis gives the acceleration the pilot senses in inertial space.
    In the presence of a large body such as a planet, a gravity field also
    provides an accelerating attraction. This acceleration can be transformed
    from the reference frame of the planet so as to be expressed in the frame
    of reference of the aircraft. This gravity field accelerating attraction
    is felt by the pilot as a force on her tushie as she sits in her aircraft
    on the runway awaiting takeoff clearance.

    In JSBSim the acceleration of the body frame in inertial space is given
    by the F = ma relation. If the vForces vector is divided by the aircraft
    mass, the acceleration vector is calculated. The term wdot is equivalent
    to the JSBSim vPQRdot vector, and the w parameter is equivalent to vPQR.
    The radius R is calculated below in the vector vToEyePt.

    @author Tony Peden, Jon Berndt
    @version $Id: FGAuxiliary.h,v 1.38 2004/03/23 12:32:53 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAuxiliary : public FGModel {
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGAuxiliary(FGFDMExec* Executive);
  /// Destructor
  ~FGAuxiliary();

  /** Runs the Auxiliary routines; called by the Executive
      @return false if no error */
  bool Run(void);

  // Use FGInitialCondition to set these speeds
  inline double GetVcalibratedFPS(void) const { return vcas; }
  inline double GetVcalibratedKTS(void) const { return vcas*fpstokts; }
  inline double GetVequivalentFPS(void) const { return veas; }
  inline double GetVequivalentKTS(void) const { return veas*fpstokts; }

  inline double GetTotalTemperature(void) const { return tat; }
  inline double GetTAT_C(void) const { return tatc; }

  // total pressure above is freestream total pressure for subsonic only
  // for supersonic it is the 1D total pressure behind a normal shock
  inline double GetTotalPressure(void) const { return pt; }

  inline FGColumnVector3& GetPilotAccel(void) { return vPilotAccel; }
  inline double GetPilotAccel(int idx) const { return vPilotAccel(idx); }
  FGColumnVector3 GetNpilot(void) const { return vPilotAccelN; }
  double GetNpilot(int idx) const { return vPilotAccelN(idx); }
  inline FGColumnVector3& GetAeroPQR(void) {return vAeroPQR;}
  inline double GetAeroPQR(int axis) const {return vAeroPQR(axis);}
  inline double Getphi(void) const {return vEuler(ePhi);}
  inline double Gettht(void) const {return vEuler(eTht);}
  inline double Getpsi(void) const {return vEuler(ePsi);}
  inline FGColumnVector3& GetEuler(void) {return vEuler;}
  inline double GetEuler(int axis) const {return vEuler(axis);}
  inline FGColumnVector3& GetEulerRates(void) { return vEulerRates; }
  inline double GetEulerRates(int axis) const { return vEulerRates(axis); }
  inline void SetEuler(FGColumnVector3 tt) {vEuler = tt;}

  inline FGColumnVector3& GetAeroUVW (void)    { return vAeroUVW; }
  inline double           GetAeroUVW (int idx) const { return vAeroUVW(idx); }

  double Getalpha(void) const { return alpha; }
  double Getbeta (void) const { return beta; }
  inline double GetMagBeta(void) const { return fabs(beta); }
  double Getqbar (void) const { return qbar; }
  double GetqbarUW (void) const { return qbarUW; }
  double GetqbarUV (void) const { return qbarUV; }
  inline double GetVt   (void) const { return Vt; }
  double GetMach (void) const { return Mach; }
  inline double GetMachU(void) const { return MachU; }
  double Getadot (void) const { return adot; }
  double Getbdot (void) const { return bdot; }

  void SetAeroUVW(FGColumnVector3 tt) { vAeroUVW = tt; }

  inline void Setalpha(double tt) { alpha = tt; }
  inline void Setbeta (double tt) { beta  = tt; }
  inline void Setqbar (double tt) { qbar = tt; }
  inline void SetqbarUW (double tt) { qbarUW = tt; }
  inline void SetqbarUV (double tt) { qbarUV = tt; }
  inline void SetVt   (double tt) { Vt = tt; }
  inline void SetMach (double tt) { Mach=tt; }
  inline void Setadot (double tt) { adot = tt; }
  inline void Setbdot (double tt) { bdot = tt; }

  inline void SetAB(double t1, double t2) { alpha=t1; beta=t2; }
  inline double GetEarthPositionAngle(void) const { return earthPosAngle; }

  double GetHeadWind(void);
  double GetCrossWind(void);

  void bind(void);
  void unbind(void);

private:
  double vcas, veas;
  double rhosl, rho, p, psl, pt, tat, sat, tatc;

  // Don't add a getter for pt!

  FGColumnVector3 vPilotAccel;
  FGColumnVector3 vPilotAccelN;
  FGColumnVector3 vToEyePt;
  FGColumnVector3 vAeroPQR;
  FGColumnVector3 vAeroUVW;
  FGColumnVector3 vEuler;
  FGColumnVector3 vEulerRates;
  FGColumnVector3 vMachUVW;

  double Vt, Mach, MachU;
  double qbar, qbarUW, qbarUV;
  double alpha, beta;
  double adot,bdot;

  double earthPosAngle;

  void Debug(int from);
};

} // namespace JSBSim

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

