/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       LaGrangeMultiplier.h
 Author:       Bertrand Coconnier
 Date started: 07/01/11

 ------------- Copyright (C) 2011  Bertrand Coconnier -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef LAGRANGEMULTIPLIER_H
#define LAGRANGEMULTIPLIER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
  
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Rotational state of a wheel whose spin is resolved together with the
    ground friction multipliers (see the <wheel_inertia> element of FGLGear). */
struct WheelSpinDOF {
  double InvInertia = 0.0; ///< 1 / spin inertia about the axle [1/(slug*ft^2)]
  double Rate = 0.0;       ///< absolute spin rate about the axle [rad/s]
  double Accel = 0.0;      ///< spin acceleration from the last friction solve [rad/s^2]
};

struct LagrangeMultiplier {
  FGColumnVector3 ForceJacobian;
  FGColumnVector3 LeverArm;
  double Min;
  double Max;
  double value;
  /// When true, the moment applied to the airframe per unit multiplier is
  /// MomentJacobian instead of LeverArm * ForceJacobian. Needed for torques
  /// that act between a wheel and the airframe.
  bool UseMomentJacobian = false;
  FGColumnVector3 MomentJacobian;
  /// Wheel spin degree of freedom coupled to this multiplier (or nullptr) and
  /// the multiplier's Jacobian entry on that wheel's spin rate.
  WheelSpinDOF* Wheel = nullptr;
  double WheelCoeff = 0.0;
};

} // namespace

#endif
