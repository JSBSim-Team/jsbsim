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

#include "FGModel.h"
#include "math/FGLocation.h"

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
  double SLgravity(void) const {return gAccelReference;}
  const FGColumnVector3& GetGravity(void) const {return vGravAccel;}
  const FGColumnVector3& GetOmegaPlanet() const {return vOmegaPlanet;}
  void SetOmegaPlanet(double rate) {
    vOmegaPlanet = FGColumnVector3(0.0, 0.0, rate);
  }
  double GetRefRadius(void) const {return RadiusReference;}
  double GetSemimajor(void) const {return a;}
  double GetSemiminor(void) const {return b;}

  double GetAltitudeASL(const FGLocation& l) const
  { return l.GetRadius() - RadiusReference; }
  void SetAltitudeASL(FGLocation& l, double altitude) const
  { l.SetRadius(altitude + RadiusReference); }

  struct Inputs {
    FGLocation Position;
  } in;

private:
  /// These define the indices use to select the gravitation models.
  enum eGravType {
    /// Evaluate gravity using Newton's classical formula assuming the Earth is
    /// spherical
    gtStandard,
    /// Evaluate gravity using WGS84 formulas that take the Earth oblateness
    /// into account
    gtWGS84
  };

  FGColumnVector3 vOmegaPlanet;
  FGColumnVector3 vGravAccel;
  double gAccelReference;
  double RadiusReference;
  double GM;
  double C2_0; // WGS84 value for the C2,0 coefficient
  double J2;   // WGS84 value for J2
  double a;    // WGS84 semimajor axis length in feet 
  double b;    // WGS84 semiminor axis length in feet
  int gravType;

  double GetGAccel(double r) const;
  FGColumnVector3 GetGravityJ2(const FGLocation& position) const;
  void bind(void);
  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
