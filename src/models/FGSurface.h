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

#include "FGFDMExec.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_SURFACE "$Id: FGSurface.h,v 1.2 2014/01/16 12:31:50 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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
  FGSurface();

  /// Constructor
  FGSurface(FGFDMExec* fdmex);
  /// Destructor
  ~FGSurface();

  /// Sets the friction factor of the surface area
  void SetFrictionFactor(double friction) { frictionFactor = friction; }

  /// Sets the rolling friction of the surface area
  void SetRollingFriction(double friction) { rollingFriction = friction; }

  /// Sets the load capacity of the surface area
  void SetLoadCapacity(double capacity ) { loadCapacity = capacity; }

  /// Sets the load resistance of the surface area
  void SetLoadResistance(double resistance) { loadResistance = resistance; }

  /// Sets the bumpiness factor associated with the surface
  void SetBumpiness(double bump) { bumpiness = bump; }

  /// Sets the surface is a solid flag value
  void SetSolid(bool solid) { isSolid = solid; }

  /// Set the currect position for bumpiness calulcation
  void SetPosition(const double pt[3]) {
      pos[0] = pt[0]; pos[1] = pt[1]; pos[2] = pt[2];
  }


  /// Gets the friction factor of the surface area
  double GetFrictionFactor(void) const { return frictionFactor; }

  /// Gets the rolling friction of the surface area
  double GetRollingFriction(void) const { return rollingFriction; }

  /// Gets the load capacity of the surface area
  double GetLoadCapacity(void) const { return loadCapacity; }

  /// Gets the load resistance of the surface area
  double GetLoadResistance(void) const { return loadResistance; }

  /// Gets the bumpiness factor associated with the surface
  double GetBumpiness(void) const { return bumpiness; }

  /// Gets the surface is a solid flag value
  bool   GetSolid(void) const { return isSolid; }

  /// Returns the height of the bump at the provided offset
  float  GetBumpHeight();

  std::string GetSurfaceStrings(std::string delimeter) const;
  std::string GetSurfaceValues(std::string delimeter) const;

protected:
  double frictionFactor;
  double rollingFriction;
  double loadCapacity;
  double loadResistance;
  double bumpiness;
  bool isSolid;

private:
  double pos[3];
};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

