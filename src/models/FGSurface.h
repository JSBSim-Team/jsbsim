/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGSurface.h
 Author:       Erik Hofman
 Date started: 01/15/14

 ------------- Copyright (C) 2014  Jon S. Berndt (jon@jsbsim.org) -------------

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
01/15/14   EMH   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSURFACE_H
#define FGSURFACE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFDMExec;
class FGPropertyManager;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for all surface properties
    @author Erik M. Hofman
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGSurface
{
public:
  /// Constructor
  FGSurface(FGFDMExec* fdmex);

  void bind(FGPropertyManager* pm);

  /// Reset all surface values to a default
  void resetValues(void);

  /// Sets the static friction factor of the surface area
  void SetStaticFFactor(double friction) { staticFFactor = friction; }

  /// Sets the rolling friction factor of the surface area
  void SetRollingFFactor(double friction) { rollingFFactor = friction; }

  /// Sets the maximum force for the surface area
  void SetMaximumForce(double force) { maximumForce = force; }

  /// Sets the normalized bumpiness factor associated with the surface
  void SetBumpiness(double bump) { bumpiness = bump; }

  /// Sets the surface is a solid flag value
  void SetSolid(bool solid) { isSolid = solid; }

  /// Set the currect position for bumpiness calulcation
  void SetPosition(const double pt[3]) {
      pos[0] = pt[0]; pos[1] = pt[1]; pos[2] = pt[2];
  }


  /// Gets the static friction factor of the surface area
  double GetStaticFFactor(void) { return staticFFactor; }

  /// Gets the rolling friction factor of the surface area
  double GetRollingFFactor(void) { return rollingFFactor; }

  /// Gets the maximum force of the surface area
  double GetMaximumForce(void) { return maximumForce; }

  /// Gets the normalized bumpiness factor associated with the surface
  double GetBumpiness(void) { return bumpiness; }

  /// Gets the surface is a solid flag value
  bool GetSolid(void) { return isSolid; }

  /// Returns the height of the bump at the provided offset
  double  GetBumpHeight();

protected:
  double staticFFactor, rollingFFactor;
  double maximumForce;
  double bumpiness;
  bool isSolid;

private:
  double pos[3];
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
