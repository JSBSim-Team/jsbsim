/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropagate.h
 Author:       Jon S. Berndt
 Date started: 1/5/99

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
01/05/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPAGATE_H
#define FGPROPAGATE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "FGColumnVector3.h"
#include "FGQuaternion.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPAGATE "$Id: FGPropagate.h,v 1.9 2004/04/24 17:12:58 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the EOM and integration/propagation of state
    @author Jon S. Berndt, Mathias Froehlich
    @version $Id: FGPropagate.h,v 1.9 2004/04/24 17:12:58 jberndt Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGPropagate : public FGModel {
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGPropagate(FGFDMExec* Executive);

  /// Destructor
  ~FGPropagate();

  bool InitModel(void);

  /** Runs the Propagate model; called by the Executive
      @return false if no error */
  bool Run(void);

  const FGColumnVector3& GetVel(void) { return vVel; }
  const FGColumnVector3& GetUVW   (void)    { return vUVW; }
  const FGColumnVector3& GetUVWdot(void)    { return vUVWdot; }
  const FGColumnVector3& GetPQR(void) {return vPQR;}
  const FGColumnVector3& GetPQRdot(void) {return vPQRdot;}
  const FGColumnVector3& GetEuler(void) const { return vQtrn.GetEuler(); }

  double GetUVW   (int idx) const { return vUVW(idx); }
  double GetUVWdot(int idx) const { return vUVWdot(idx); }
  double GetVn(void)  const { return vVel(eNorth); }
  double GetVe(void)  const { return vVel(eEast); }
  double GetVd(void)  const { return vVel(eDown); }
  double Geth(void)   const { return vLocation(eRad) - SeaLevelRadius; }
  double GetPQR(int axis) const {return vPQR(axis);}
  double GetPQRdot(int idx) const {return vPQRdot(idx);}
  double GetEuler(int axis) const { return vQtrn.GetEuler()(axis); }
  double Gethdot(void) const { return vLocationDot(eRad); }
//  double GetLatitude(void) const { return vLocation(eLat); }
//  double GetLatitudeDot(void) const { return vLocationDot(eLat); }
//  double GetLongitude(void) const { return vLocation(eLong); }
//  double GetLongitudeDot(void) const { return vLocationDot(eLong); }

  /** Returns the "constant" RunwayRadius.
      The RunwayRadius parameter is set by the calling application or set to
      zero if JSBSim is running in standalone mode.
      @return distance of the runway from the center of the earth.
      @units feet */
  double GetRunwayRadius(void) const { return RunwayRadius; }
  double GetDistanceAGL(void)  const { return vLocation(eRad)-RunwayRadius; }
  double GetRadius(void) const { return vLocation(eRad); }
  double GetLocation (int idx) const { return vLocation(idx);}
  double GetLocationDot (int idx) const { return vLocationDot(idx);}
  const  FGColumnVector3& GetLocation(void) const { return vLocation;}
  const  FGColumnVector3& GetLocationDot(void) const { return vLocationDot;}

  double Getphi(void) const { return vQtrn.GetEulerPhi(); }
  double Gettht(void) const { return vQtrn.GetEulerTheta(); }
  double Getpsi(void) const { return vQtrn.GetEulerPsi(); }

  double GetCosphi(void) const { return vQtrn.GetCosEulerPhi(); }
  double GetCostht(void) const { return vQtrn.GetCosEulerTheta(); }
  double GetCospsi(void) const { return vQtrn.GetCosEulerPsi(); }

  double GetSinphi(void) const { return vQtrn.GetSinEulerPhi(); }
  double GetSintht(void) const { return vQtrn.GetSinEulerTheta(); }
  double GetSinpsi(void) const { return vQtrn.GetSinEulerPsi(); }

  /** Retrieves the local-to-body transformation matrix.
      @return a reference to the local-to-body transformation matrix.  */
  const FGMatrix33& GetTl2b(void) { return vQtrn.GetT(); }

  /** Retrieves the body-to-local transformation matrix.
      @return a reference to the body-to-local matrix.  */
  const FGMatrix33& GetTb2l(void) { return vQtrn.GetTInv(); }

  double GetHOverBCG(void) const { return hoverbcg; }
  double GetHOverBMAC(void) const { return hoverbmac; }

// SET functions

  void SetvVel(const FGColumnVector3& v) { vVel = v; }
//  void SetLatitude(double tt) { vLocation(eLat) = tt; }
//  void SetLongitude(double tt) { vLocation(eLong) = tt; }
  void SetLocation(int idx, double val) { vLocation(idx) = val; }
  void Seth(double tt);
  void SetRunwayRadius(double tt) { RunwayRadius = tt; }
  void SetSeaLevelRadius(double tt) { SeaLevelRadius = tt;}
  void SetDistanceAGL(double tt);
  void SetEuler(FGColumnVector3 tt) {vQtrn = FGQuaternion(tt(ePhi), tt(eTht), tt(ePsi));}
  void SetUVW(FGColumnVector3 tt) { vUVW = tt; }
  void SetPQR(FGColumnVector3 tt) {vPQR = tt;}
  void SetPQR(double p, double q, double r) {vPQR(eP)=p; vPQR(eQ)=q; vPQR(eR)=r;}

  void bind(void);
  void unbind(void);

private:
  FGColumnVector3 vVel;
  FGColumnVector3 vMac;
  FGColumnVector3 vLocation;
  FGColumnVector3 vLocationDot;
  FGColumnVector3 vLocationDot_prev[4];
  FGColumnVector3 vPQR;
  FGColumnVector3 vPQRdot;
  FGColumnVector3 vPQRdot_prev[4];
  FGColumnVector3 vUVW;
  FGColumnVector3 vUVWdot;
  FGColumnVector3 vUVWdot_prev[4];
  FGQuaternion vQtrn;
  FGQuaternion vQtrndot_prev[4];

  double dt;
  double RunwayRadius, SeaLevelRadius;
  double hoverbcg,hoverbmac,b;

  FGColumnVector3& toGlobe(FGColumnVector3&);

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
