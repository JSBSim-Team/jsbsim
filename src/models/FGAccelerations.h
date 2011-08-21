/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAccelerations.h
 Author:       Jon S. Berndt
 Date started: 07/12/11

 ------------- Copyright (C) 2011  Jon S. Berndt (jon@jsbsim.org) -------------

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
07/12/11   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGACCELERATIONS_H
#define FGACCELERATIONS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "models/FGModel.h"
#include "math/FGColumnVector3.h"
#include "math/LagrangeMultiplier.h"
#include "math/FGMatrix33.h"
#include "math/FGQuaternion.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_ACCELERATIONS "$Id: FGAccelerations.h,v 1.5 2011/08/21 15:13:22 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Handles the calculation of accelerations.

    -Calculate the angular accelerations
    -Calculate the translational accelerations
    -Calculate the angular rate
    -Calculate the translational velocity

    @author Jon S. Berndt, Mathias Froehlich, Bertrand Coconnier
    @version $Id: FGAccelerations.h,v 1.5 2011/08/21 15:13:22 bcoconni Exp $
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAccelerations : public FGModel {
public:
  /** Constructor.
      @param Executive a pointer to the parent executive object */
  FGAccelerations(FGFDMExec* Executive);

  /// Destructor
  ~FGAccelerations();
  
  /// These define the indices use to select the gravitation models.
  enum eGravType {gtStandard, gtWGS84}; 

  /** Initializes the FGAccelerations class after instantiation and prior to first execution.
      The base class FGModel::InitModel is called first, initializing pointers to the 
      other FGModel objects (and others).  */
  bool InitModel(void);

  /** Runs the state propagation model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);

  const FGQuaternion& GetQuaterniondot(void) const {return vQtrndot;}

  /** Retrieves the body axis acceleration.
      Retrieves the computed body axis accelerations based on the
      applied forces and accounting for a rotating body frame.
      The vector returned is represented by an FGColumnVector reference. The vector
      for the acceleration in Body frame is organized (Ax, Ay, Az). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vUVWdot(1) is Ax. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eX=1, eY=2, eZ=3.
      units ft/sec^2
      @return Body axis translational acceleration in ft/sec^2.
  */
  const FGColumnVector3& GetUVWdot(void) const { return vUVWdot; }

  const FGColumnVector3& GetUVWidot(void) const { return vUVWidot; }

  /** Retrieves the body axis angular acceleration vector.
      Retrieves the body axis angular acceleration vector in rad/sec^2. The
      angular acceleration vector is determined from the applied forces and
      accounts for a rotating frame.
      The vector returned is represented by an FGColumnVector reference. The vector
      for the angular acceleration in Body frame is organized (Pdot, Qdot, Rdot). The vector
      is 1-based, so that the first element can be retrieved using the "()" operator.
      In other words, vPQRdot(1) is Pdot. Various convenience enumerators are defined
      in FGJSBBase. The relevant enumerators for the vector returned by this call are,
      eP=1, eQ=2, eR=3.
      units rad/sec^2
      @return The angular acceleration vector.
  */
  const FGColumnVector3& GetPQRdot(void) const {return vPQRdot;}
  
  const FGColumnVector3& GetPQRidot(void) const {return vPQRidot;}

  /** Retrieves a body frame acceleration component.
      Retrieves a body frame acceleration component. The acceleration returned
      is extracted from the vUVWdot vector (an FGColumnVector). The vector for
      the acceleration in Body frame is organized (Ax, Ay, Az). The vector is
      1-based. In other words, GetUVWdot(1) returns Ax. Various convenience
      enumerators are defined in FGJSBBase. The relevant enumerators for the
      acceleration returned by this call are, eX=1, eY=2, eZ=3.
      units ft/sec^2
      @param idx the index of the acceleration component desired (1-based).
      @return The body frame acceleration component.
  */
  double GetUVWdot(int idx) const { return vUVWdot(idx); }

  FGColumnVector3& GetBodyAccel(void) { return vBodyAccel; }

  double GetBodyAccel(int idx) const { return vBodyAccel(idx); }

  /** Retrieves a body frame angular acceleration component.
      Retrieves a body frame angular acceleration component. The angular
      acceleration returned is extracted from the vPQRdot vector (an
      FGColumnVector). The vector for the angular acceleration in Body frame
      is organized (Pdot, Qdot, Rdot). The vector is 1-based. In other words,
      GetPQRdot(1) returns Pdot (roll acceleration). Various convenience
      enumerators are defined in FGJSBBase. The relevant enumerators for the
      angular acceleration returned by this call are, eP=1, eQ=2, eR=3.
      units rad/sec^2
      @param axis the index of the angular acceleration component desired (1-based).
      @return The body frame angular acceleration component.
  */
  double GetPQRdot(int axis) const {return vPQRdot(axis);}

  double GetMoments(int idx) const { return in.Moment(idx) + vFrictionMoments(idx); }
  double GetForces(int idx) const { return in.Force(idx) + vFrictionForces(idx); }
  double GetGroundMoments(int idx) const { return in.GroundMoment(idx) + vFrictionMoments(idx); }
  double GetGroundForces(int idx) const { return in.GroundForce(idx) + vFrictionForces(idx); }

  void InitializeDerivatives(void);

  void DumpState(void);

  struct Inputs {
    FGMatrix33 J;
    FGMatrix33 Jinv;
    FGMatrix33 Ti2b;
    FGMatrix33 Tb2i;
    FGMatrix33 Tec2b;
    FGMatrix33 Tl2b;
    FGQuaternion qAttitudeECI;
    FGColumnVector3 Moment;
    FGColumnVector3 GroundMoment;
    FGColumnVector3 Force;
    FGColumnVector3 GroundForce;
    FGColumnVector3 J2Grav;
    FGColumnVector3 vPQRi;
    FGColumnVector3 vPQR;
    FGColumnVector3 vUVW;
    FGColumnVector3 vInertialPosition;
    FGColumnVector3 vOmegaPlanet;
    double DeltaT;
    double Mass;
    double GAccel;
    vector<LagrangeMultiplier*> *MultipliersList;
  } in;

private:

  FGColumnVector3 vPQRdot, vPQRidot;
  FGColumnVector3 vUVWdot, vUVWidot;
  FGQuaternion vQtrndot;
  FGColumnVector3 vBodyAccel;
  FGColumnVector3 vGravAccel;
  FGColumnVector3 vFrictionForces;
  FGColumnVector3 vFrictionMoments;

  int gravType;

  void CalculatePQRdot(void);
  void CalculateQuatdot(void);
  void CalculateUVWdot(void);

  void ResolveFrictionForces(double dt);

  void bind(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
