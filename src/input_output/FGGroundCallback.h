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

class JSBSIM_API FGGroundCallback
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

  /** Set the terrain elevation.
      Only needs to be implemented if JSBSim should be allowed
      to modify the local terrain radius (see the default implementation)
   */
  virtual void SetTerrainElevation(double h) {}

  /** Set the planet semimajor and semiminor axes.
      Only needs to be implemented if JSBSim should be allowed to modify
      the planet dimensions.
   */
  virtual void SetEllipse(double semimajor, double semiminor) {}

  /** Set the simulation time.
      The elapsed time can be used by the ground callbck to assess the planet
      rotation or the movement of objects.
      @param _time elapsed time in seconds since the simulation started.
   */
  void SetTime(double _time) { time = _time; }

protected:
  double time;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// The default sphere earth implementation:
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

class JSBSIM_API FGDefaultGroundCallback : public FGGroundCallback
{
public:
  explicit FGDefaultGroundCallback(double semiMajor, double semiMinor) :
    a(semiMajor), b(semiMinor) {}

  double GetAGLevel(double t, const FGLocation& location,
                    FGLocation& contact,
                    FGColumnVector3& normal, FGColumnVector3& v,
                    FGColumnVector3& w) const override;

  void SetTerrainElevation(double h) override
  { mTerrainElevation = h; }

  void SetEllipse(double semimajor, double semiminor) override
  { a = semimajor; b = semiminor; }

private:
  double a, b;
  double mTerrainElevation = 0.0;
};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
