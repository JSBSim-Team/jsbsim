/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGroundCallback.h
 Author:       Mathias Froehlich
 Date started: 05/21/04

 ------ Copyright (C) 2004 Mathias Froehlich (Mathias.Froehlich@web.de) -------

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
-------------------------------------------------------------------------------
05/21/00   MF   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGGROUNDCALLBACK_H
#define FGGROUNDCALLBACK_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "simgear/structure/SGSharedPtr.hxx"

namespace JSBSim {

class FGLocation;
class FGColumnVector3;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** This class provides callback slots to get ground specific data.

    The default implementation returns values for a
    ball formed earth with an adjustable terrain elevation.

    @author Mathias Froehlich
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGGroundCallback : public SGReferenced
{
public:

  FGGroundCallback() : time(0.0) {}
  virtual ~FGGroundCallback() {}

  /** Compute the altitude above ground.
      The altitude depends on time t and location l.
      @param t simulation time
      @param l location
      @param contact Contact point location below the location l
      @param normal Normal vector at the contact point
      @param v Linear velocity at the contact point
      @param w Angular velocity at the contact point
      @return altitude above ground
   */
  virtual double GetAGLevel(double t, const FGLocation& location,
                            FGLocation& contact,
                            FGColumnVector3& normal, FGColumnVector3& v,
                            FGColumnVector3& w) const = 0;

  /** Compute the altitude above ground.
      The altitude depends on location l.
      @param l location
      @param contact Contact point location below the location l
      @param normal Normal vector at the contact point
      @param v Linear velocity at the contact point
      @param w Angular velocity at the contact point
      @return altitude above ground
   */
  virtual double GetAGLevel(const FGLocation& location, FGLocation& contact,
                            FGColumnVector3& normal, FGColumnVector3& v,
                            FGColumnVector3& w) const
  { return GetAGLevel(time, location, contact, normal, v, w); }

  /** Compute the local terrain radius
      @param t simulation time
      @param location location
   */
  virtual double GetTerrainGeoCentRadius(double t, const FGLocation& location) const = 0;

  /** Compute the local terrain radius
      @param location location
   */
  virtual double GetTerrainGeoCentRadius(const FGLocation& location) const
  { return GetTerrainGeoCentRadius(time, location); }

  /** Return the sea level radius
      @param location location
   */
  virtual double GetSeaLevelRadius(const FGLocation& location) const = 0;

  /** Set the local terrain radius.
      Only needs to be implemented if JSBSim should be allowed
      to modify the local terrain radius (see the default implementation)
   */
  virtual void SetTerrainGeoCentRadius(double radius)  {}

  void SetTime(double _time) { time = _time; }

protected:
  double time;
};

typedef SGSharedPtr<FGGroundCallback> FGGroundCallback_ptr;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// The default sphere earth implementation:
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

class FGDefaultGroundCallback : public FGGroundCallback
{
public:
  explicit FGDefaultGroundCallback(double referenceRadius) :
    mSeaLevelRadius(referenceRadius), mTerrainLevelRadius(referenceRadius) {}

  double GetAGLevel(double t, const FGLocation& location,
                    FGLocation& contact,
                    FGColumnVector3& normal, FGColumnVector3& v,
                    FGColumnVector3& w) const override;

  void SetTerrainGeoCentRadius(double radius) override
  {  mTerrainLevelRadius = radius;}
  double GetTerrainGeoCentRadius(double t, const FGLocation& location) const override
  { return mTerrainLevelRadius; }

  double GetSeaLevelRadius(const FGLocation& location) const override
  {return mSeaLevelRadius; }

private:
   double mSeaLevelRadius;
   double mTerrainLevelRadius;
};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
