/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGInertial.h
 Author:       Jon S. Berndt
 Date started: 09/13/00

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
09/13/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGINERTIAL_H
#define FGINERTIAL_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>

#include "FGModel.h"
#include "math/FGLocation.h"
#include "input_output/FGGroundCallback.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models inertial forces (e.g. centripetal and coriolis accelerations).
    Starting conversion to WGS84.
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGInertial : public FGModel {

public:
  explicit FGInertial(FGFDMExec*);
  ~FGInertial(void);

  /** Runs the Inertial model; called by the Executive
      Can pass in a value indicating if the executive is directing the
      simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim
                     from advancing time. Some models may ignore this flag, such
                     as the Input model, which may need to be active to listen
                     on a socket for the "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;
  static constexpr double GetStandardGravity(void) { return gAccelReference; }
  const FGColumnVector3& GetGravity(void) const {return vGravAccel;}
  const FGColumnVector3& GetOmegaPlanet() const {return vOmegaPlanet;}
  void SetOmegaPlanet(double rate) {
    vOmegaPlanet = FGColumnVector3(0.0, 0.0, rate);
  }
  double GetSemimajor(void) const {return a;}
  double GetSemiminor(void) const {return b;}

  /** @name Functions that rely on the ground callback
      The following functions allow to set and get the vehicle position above
      the ground. The ground level is obtained by interrogating an instance of
      FGGroundCallback. A ground callback must therefore be set with
      SetGroundCallback() before calling any of these functions. */
  ///@{
  /** Get terrain contact point information below the current location.
      @param location     Location at which the contact point is evaluated.
      @param contact      Contact point location
      @param normal       Terrain normal vector in contact point    (ECEF frame)
      @param velocity     Terrain linear velocity in contact point  (ECEF frame)
      @param ang_velocity Terrain angular velocity in contact point (ECEF frame)
      @return Location altitude above contact point (AGL) in feet.
      @see SetGroundCallback */
  double GetContactPoint(const FGLocation& location, FGLocation& contact,
                         FGColumnVector3& normal, FGColumnVector3& velocity,
                         FGColumnVector3& ang_velocity) const
  {
    return GroundCallback->GetAGLevel(location, contact, normal, velocity,
                                      ang_velocity); }

  /** Get the altitude above ground level.
      @return the altitude AGL in feet.
      @param location Location at which the AGL is evaluated.
      @see SetGroundCallback */
  double GetAltitudeAGL(const FGLocation& location) const {
    FGLocation lDummy;
    FGColumnVector3 vDummy;
    return GroundCallback->GetAGLevel(location, lDummy, vDummy, vDummy,
                                      vDummy);
  }

  /** Set the altitude above ground level.
      @param location    Location at which the AGL is set.
      @param altitudeAGL Altitude above Ground Level in feet.
      @see SetGroundCallback */
  void SetAltitudeAGL(FGLocation& location, double altitudeAGL);

  /** Set the terrain elevation above sea level.
      @param h Terrain elevation in ft.
      @see SetGroundcallback */
  void SetTerrainElevation(double h) {
    GroundCallback->SetTerrainElevation(h);
  }

  /** Set the simulation time.
      The elapsed time can be used by the ground callbck to assess the planet
      rotation or the movement of objects.
      @param time elapsed time in seconds since the simulation started.
  */
  void SetTime(double time) {
    GroundCallback->SetTime(time);
  }
  ///@}

  /** Sets the ground callback pointer.
      FGInertial will take ownership of the pointer which must therefore be
      located in the heap.
      @param gc A pointer to a ground callback object
      @see FGGroundCallback
  */
  void SetGroundCallback(FGGroundCallback* gc) { GroundCallback.reset(gc); }

  /// These define the indices use to select the gravitation models.
  enum eGravType {
    /// Evaluate gravity using Newton's classical formula assuming the Earth is
    /// spherical
    gtStandard,
    /// Evaluate gravity using WGS84 formulas that take the Earth oblateness
    /// into account
    gtWGS84
  };

  /// Get the gravity type.
  int GetGravityType(void) const { return gravType; }

  /// Set the gravity type.
  void SetGravityType(int gt);

  /** Transform matrix from the local horizontal frame to earth centered.
      The local frame is the NED (North-East-Down) frame. Since the Down
      direction depends on the gravity so is the local frame.
      The East direction is tangent to the spheroid with a null component along
      the Z axis.
      The North direction is obtained from the cross product between East and
      Down.
      @param location The location at which the transform matrix must be
                      evaluated.
      @return a rotation matrix of the transform from the earth centered frame
              to the local horizontal frame.
  */
  FGMatrix33 GetTl2ec(const FGLocation& location) const;

  /** Transform matrix from the earth centered to local horizontal frame.
      The local frame is the NED (North-East-Down) frame. Since the Down
      direction depends on the gravity so is the local frame.
      The East direction is tangent to the spheroid with a null component along
      the Z axis.
      The North direction is obtained from the cross product between East and
      Down.
      @param location The location at which the transform matrix must be
                      evaluated.
      @return a rotation matrix of the transform from the earth centered frame
              to the local horizontal frame.
  */
  FGMatrix33 GetTec2l(const FGLocation& location) const
  { return GetTl2ec(location).Transposed(); }

  struct Inputs {
    FGLocation Position;
  } in;

  bool Load(Element* el) override;

private:
  // Standard gravity (9.80665 m/s^2) in ft/s^2 which is the gravity at 45 deg.
  // of latitude (see ISA 1976 and Steven & Lewis)
  // It includes the centripetal acceleration.
  static constexpr double gAccelReference = 9.80665 / fttom;

  FGColumnVector3 vOmegaPlanet;
  FGColumnVector3 vGravAccel;
  double GM;
  double J2;   // WGS84 value for J2
  double a;    // WGS84 semimajor axis length in feet 
  double b;    // WGS84 semiminor axis length in feet
  int gravType;
  std::unique_ptr<FGGroundCallback> GroundCallback;

  double GetGAccel(double r) const;
  FGColumnVector3 GetGravityJ2(const FGLocation& position) const;
  void bind(void);
  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
