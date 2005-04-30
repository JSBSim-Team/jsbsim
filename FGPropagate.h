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
#include "FGInitialCondition.h"
#include "FGLocation.h"
#include "FGQuaternion.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_PROPAGATE "$Id: FGPropagate.h,v 1.17 2005/04/30 15:49:51 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the EOM and integration/propagation of state
    @author Jon S. Berndt, Mathias Froehlich
    @version $Id: FGPropagate.h,v 1.17 2005/04/30 15:49:51 jberndt Exp $
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
  double GetDistanceAGL(void)  const;
  double GetRadius(void) const { return VState.vLocation.GetRadius(); }
  double GetLongitude(void) const { return VState.vLocation.GetLongitude(); }
  double GetLatitude(void) const { return VState.vLocation.GetLatitude(); }
  const FGLocation& GetLocation(void) const { return VState.vLocation; }

  /** Retrieves the local-to-body transformation matrix.
      @return a reference to the local-to-body transformation matrix.  */
  const FGMatrix33& GetTl2b(void) const { return VState.vQtrn.GetT(); }

  /** Retrieves the body-to-local transformation matrix.
      @return a reference to the body-to-local matrix.  */
  const FGMatrix33& GetTb2l(void) const { return VState.vQtrn.GetTInv(); }

// SET functions

  void SetLongitude(double lon) { VState.vLocation.SetLongitude(lon); }
  void SetLatitude(double lat) { VState.vLocation.SetLatitude(lat); }
  void SetRadius(double r) { VState.vLocation.SetRadius(r); }
  void SetLocation(const FGLocation& l) { VState.vLocation = l; }
  void Seth(double tt);
  void SetSeaLevelRadius(double tt) { SeaLevelRadius = tt; }
  void SetDistanceAGL(double tt);
  void SetInitialState(const FGInitialCondition *);

  void RecomputeRunwayRadius(void);

  void bind(void);
  void unbind(void);

private:

// state vector

  struct VehicleState VState;

  FGColumnVector3 vVel;
  FGColumnVector3 vPQRdot;
  FGColumnVector3 vUVWdot;

  double RunwayRadius, SeaLevelRadius;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
