/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGPropagate.h
 Author:       Jon S. Berndt
 Date started: 1/5/99

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
01/05/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGPROPAGATE_H
#define FGPROPAGATE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <models/FGModel.h>
#include <math/FGColumnVector3.h>
#include <initialization/FGInitialCondition.h>
#include <math/FGLocation.h>
#include <math/FGQuaternion.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPAGATE "$Id: FGPropagate.h,v 1.11 2007/12/30 23:18:51 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the EOM and integration/propagation of state
    @author Jon S. Berndt, Mathias Froehlich
    @version $Id: FGPropagate.h,v 1.11 2007/12/30 23:18:51 jberndt Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// state vector

struct VehicleState {
  FGLocation vLocation;
  FGColumnVector3 vUVW;
  FGColumnVector3 vPQR;
  FGQuaternion vQtrn;
};

class FGPropagate : public FGModel {
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGPropagate(FGFDMExec* Executive);

  /// Destructor
  ~FGPropagate();
  
  enum eIntegrateType {eNone = 0, eRectEuler, eTrapezoidal, eAdamsBashforth2, eAdamsBashforth3};

  bool InitModel(void);

  /** Runs the Propagate model; called by the Executive
      @return false if no error */
  bool Run(void);

  const FGColumnVector3& GetVel(void) const { return vVel; }
  const FGColumnVector3& GetUVW(void) const { return VState.vUVW; }
  const FGColumnVector3& GetUVWdot(void) const { return vUVWdot; }
  const FGColumnVector3& GetPQR(void) const {return VState.vPQR;}
  const FGColumnVector3& GetPQRdot(void) const {return vPQRdot;}
  const FGColumnVector3& GetEuler(void) const { return VState.vQtrn.GetEuler(); }

  double GetUVW   (int idx) const { return VState.vUVW(idx); }
  double GetUVWdot(int idx) const { return vUVWdot(idx); }
  double GetVel(int idx) const { return vVel(idx); }
  double Geth(void)   const { return VState.vLocation.GetRadius() - SeaLevelRadius; }
  double Gethmeters(void) const { return Geth()*fttom;}
  double GetPQR(int axis) const {return VState.vPQR(axis);}
  double GetPQRdot(int idx) const {return vPQRdot(idx);}
  double GetEuler(int axis) const { return VState.vQtrn.GetEuler(axis); }
  double GetCosEuler(int idx) const { return VState.vQtrn.GetCosEuler(idx); }
  double GetSinEuler(int idx) const { return VState.vQtrn.GetSinEuler(idx); }
  double Gethdot(void) const { return -vVel(eDown); }

  /** Returns the "constant" RunwayRadius.
      The RunwayRadius parameter is set by the calling application or set to
      zero if JSBSim is running in standalone mode.
      @return distance of the runway from the center of the earth.
      @units feet */
  double GetRunwayRadius(void) const;
  double GetSeaLevelRadius(void) const { return SeaLevelRadius; }
  double GetTerrainElevationASL(void) const;
  double GetDistanceAGL(void)  const;
  double GetRadius(void) const { return VState.vLocation.GetRadius(); }
  double GetLongitude(void) const { return VState.vLocation.GetLongitude(); }
  double GetLatitude(void) const { return VState.vLocation.GetLatitude(); }
  double GetLongitudeDeg(void) const { return VState.vLocation.GetLongitudeDeg(); }
  double GetLatitudeDeg(void) const { return VState.vLocation.GetLatitudeDeg(); }
  const FGLocation& GetLocation(void) const { return VState.vLocation; }

  /** Retrieves the local-to-body transformation matrix.
      @return a reference to the local-to-body transformation matrix.  */
  const FGMatrix33& GetTl2b(void) const { return VState.vQtrn.GetT(); }

  /** Retrieves the body-to-local transformation matrix.
      @return a reference to the body-to-local matrix.  */
  const FGMatrix33& GetTb2l(void) const { return VState.vQtrn.GetTInv(); }

  const VehicleState GetVState(void) const { return VState; }

  void SetVState(VehicleState vstate) {
      VState.vLocation = vstate.vLocation;
      VState.vUVW = vstate.vUVW;
      VState.vPQR = vstate.vPQR;
      VState.vQtrn = vstate.vQtrn; // ... mmh
  }

  const FGQuaternion GetQuaternion(void) const { return VState.vQtrn; }

  void SetPQR(unsigned int i, double val) {
      if ((i>=1) && (i<=3) )
          VState.vPQR(i) = val;
  }

  void SetUVW(unsigned int i, double val) {
      if ((i>=1) && (i<=3) )
          VState.vUVW(i) = val;
  }

// SET functions

  void SetLongitude(double lon) { VState.vLocation.SetLongitude(lon); }
  void SetLongitudeDeg(double lon) {SetLongitude(lon*degtorad);}
  void SetLatitude(double lat) { VState.vLocation.SetLatitude(lat); }
  void SetLatitudeDeg(double lat) {SetLatitude(lat*degtorad);}
  void SetRadius(double r) { VState.vLocation.SetRadius(r); }
  void SetLocation(const FGLocation& l) { VState.vLocation = l; }
  void Seth(double tt);
  void Sethmeters(double tt) {Seth(tt/fttom);}
  void SetSeaLevelRadius(double tt) { SeaLevelRadius = tt; }
  void SetTerrainElevationASL(double tt);
  void SetDistanceAGL(double tt);
  void SetInitialState(const FGInitialCondition *);
  void RecomputeRunwayRadius(void);

  void bind(void);
  void unbind(void);

private:

// state vector

  struct VehicleState VState;

  FGColumnVector3 vVel;
  FGColumnVector3 vPQRdot, last_vPQRdot, last2_vPQRdot;
  FGColumnVector3 vUVWdot, last_vUVWdot, last2_vUVWdot;
  FGColumnVector3 vLocationDot, last_vLocationDot, last2_vLocationDot;
  FGQuaternion vQtrndot, last_vQtrndot, last2_vQtrndot;
  
  double RunwayRadius, SeaLevelRadius;
  int integrator_rotational_rate;
  int integrator_translational_rate;
  int integrator_rotational_position;
  int integrator_translational_position;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
