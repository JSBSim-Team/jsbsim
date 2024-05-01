/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGForce.h
 Author:       Tony Peden
 Date started: 5/20/00

 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -----

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
5/20/00  TP   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

The purpose of this class is to provide storage for computed forces and
encapsulate all the functionality associated with transforming those
forces from their native coord system to the body system.  This includes
computing the moments due to the difference between the point of application
and the cg.

CAVEAT:  if the custom transform is used for wind-to-body transforms then the
         user *must* always pass this class the negative of beta. This is true
         because sideslip angle does not follow the right hand rule i.e. it is
         positive for aircraft nose left sideslip.  Note that use of the custom
         transform for this purpose shouldn't be necessary as it is already
         provided by SetTransform(tWindBody) and is not subject to the same
         restriction.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFORCE_H
#define FGFORCE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include "math/FGMatrix33.h"
#include "models/FGMassBalance.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFDMExec;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Utility class that aids in the conversion of forces between coordinate
    systems and calculation of moments.
<br><h3>Resolution of Applied Forces into Moments and Body Axes Components</h3>
<br><p>
All forces acting on the aircraft that cannot be considered a change in weight
need to be resolved into body axis components so that the aircraft acceleration
vectors, both translational and rotational, can be computed. Furthermore, the
moments produced by each force that does not act at a location corresponding to
the center of gravity also need to be computed. Unfortunately, the math required
to do this can be a bit messy and errors are easily introduced so the class
FGForce was created to provide these services in a consistent and reusable
manner.<br><br></p>

<h4>Basic usage</h4>

<p>FGForce requires that its users supply it with the location of the applied
force vector in JSBSim structural coordinates, the sense of each axis in that
coordinate system relative to the body system, the orientation of the vector
also relative to body coordinates and, of course, the force vector itself. With
this information it will compute both the body axis force components and the
resulting moments. Any moments inherently produced by the native system can be
supplied as well and they will be summed with those computed.</p>

<p>A good example for demonstrating the use of this class are the aerodynamic
forces: lift, drag, and side force and the aerodynamic moments about the pitch,
roll and yaw axes. These "native" forces and moments are computed and stored
in the FGColumnVector objects vFs and vMoments. Their native coordinate system
is often referred to as the wind system and is defined as a right-handed system
having its x-axis aligned with the relative velocity vector and pointing towards
the rear of the aircraft , the y-axis extending out the right wing, and the
z-axis directed upwards. This is different than body axes; they are defined such
that the x-axis is lies on the aircraft's roll axis and positive forward, the
y-axis is positive out the right wing, and the z-axis is positive downwards. In
this instance, JSBSim already provides the needed transform and FGForce can make
use of it by calling SetTransformType() once an object is created:</p>

<p><tt>FGForce fgf(FDMExec);</tt><br>
<tt>fgf.SetTransformType(tWindBody);</tt><br><br>

This call need only be made once for each object. The available transforms are
defined in the enumerated type TransformType and are tWindBody, tLocalBody,
tCustom, and tNone. The local-to-body transform, like the wind-to-body, also
makes use of that already available in JSBSim. tNone sets FGForce to do no
angular transform at all, and tCustom allows for modeling force vectors at
arbitrary angles relative to the body system such as that produced by propulsion
systems. Setting up and using a custom transform is covered in more detail
below.
Continuing with the example, the point of application of the aerodynamic forces,
the aerodynamic reference point in JSBSim, also needs to be set:</p>
<p><tt>
fgf.SetLocation(x, y, z)</tt></p>

<p>where x, y, and z are in JSBSim structural coordinates.</p>

<p>Initialization is complete and the FGForce object is ready to do its job. As
stated above, the lift, drag, and side force are computed and stored in the
vector vFs and need to be passed to FGForce:</p>

<p><tt>fgf.SetNativeForces(vFs);</tt> </p>

<p>The same applies to the aerodynamic pitching, rolling and yawing moments:</p>

<p><tt>fgf.SetNativeMoments(vMoments);</tt></p>

<p>Note that storing the native forces and moments outside of this class is not
strictly necessary, overloaded SetNativeForces() and SetNativeMoments() methods
which each accept three doubles (rather than a vector) are provided and can be
repeatedly called without incurring undue overhead. The body axes force vector
can now be retrieved by calling:</p>

<p><tt>vFb=fgf.GetBodyForces();</tt></p>

<p>This method is where the bulk of the work gets done so calling it more than
once for the same set of native forces and moments should probably be avoided.
Note that the moment calculations are done here as well so they should be
retrieved after calling the GetBodyForces() method:</p>

<p><tt>vM=fgf.GetMoments();</tt> </p>

<p>As an aside, the native moments are not needed to perform the computations
correctly so, if the FGForce object is not being used to store them then an
alternate approach is to avoid the SetNativeMoments call and perform the sum</p>

<p><tt>vMoments+=fgf.GetMoments();</tt> <br><br>

after the forces have been retrieved. </p>

<h4>Use of the Custom Transform Type</h4>

<p>In cases where the native force vector is not aligned with the body, wind, or
local coordinate systems a custom transform type is provided. A vectorable
engine nozzle will be used to demonstrate its usage. Initialization is much the
same:</p>

<p><tt>FGForce fgf(FDMExec);</tt> <br>
<tt>fgf.SetTransformType(tCustom);</tt> <br>
<tt>fgf.SetLocation(x,y,z);</tt> </p>

<p>Except that here the tCustom transform type is specified and the location of
the thrust vector is used rather than the aerodynamic reference point. Thrust is
typically considered to be positive when directed aft while the body x-axis is
positive forward and, if the native system is right handed, the z-axis will be
reversed as well. These differences in sense need to be specified using by the
call: </p>

<p><tt>fgf.SetSense(-1,1,-1);</tt></p>

<p>The angles are specified by calling the method: </p>

<p><tt>fgf.SetAnglesToBody(pitch, roll, yaw);</tt> </p>

<p>in which the transform matrix is computed. Note that these angles should be
taken relative to the body system and not the local as the names might suggest.
For an aircraft with vectorable thrust, this method will need to be called
every time the nozzle angle changes, a fixed engine/nozzle installation, on the
other hand, will require it to be be called only once.</p>

<p>Retrieval of the computed forces and moments is done as detailed above.</p>
<br>
<pre>
    <p><i>CAVEAT: If the custom system is used to compute
    the wind-to-body transform, then the sign of the sideslip
    angle must be reversed when calling SetAnglesToBody().
    This is true because sideslip angle does not follow the right
    hand rule. Using the custom transform type this way
    should not be necessary, as it is already provided as a built
    in type (and the sign differences are correctly accounted for).</i>
    </p>
</pre>

<h4>Use as a Base Type</h4>

<p>For use as a base type, the native force and moment vector data members are
defined as protected. In this case the SetNativeForces() and SetNativeMoments()
methods need not be used and, instead, the assignments to vFn, the force vector,
and vMn, the moments, can be made directly. Otherwise, the usage is similar.<br>
<br><br></p>

    @author Tony Peden
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class JSBSIM_API FGForce : public FGJSBBase
{
public:
  /// Constructor
  explicit FGForce(FGFDMExec *FDMExec);

  /// Destructor
  virtual ~FGForce();

  enum TransformType { tNone, tWindBody, tLocalBody, tInertialBody, tCustom };

  virtual const FGColumnVector3& GetBodyForces(void);

  inline double GetBodyXForce(void) const { return vFb(eX); }
  inline double GetBodyYForce(void) const { return vFb(eY); }
  inline double GetBodyZForce(void) const { return vFb(eZ); }
  inline const FGColumnVector3& GetMoments(void) const { return vM; }

  // Normal point of application, JSBsim structural coords
  // (inches, x +back, y +right, z +up)
  inline void SetLocation(double x, double y, double z) {
    vXYZn(eX) = x;
    vXYZn(eY) = y;
    vXYZn(eZ) = z;
    SetActingLocation(x, y, z);
  }

  /** Acting point of application.
      JSBsim structural coords used (inches, x +back, y +right, z +up).
      This function sets the point at which the force acts - this may
      not be the same as where the object resides. One area where this
      is true is P-Factor modeling.
      @param x acting location of force
      @param y acting location of force
      @param z acting location of force    */
  inline void SetActingLocation(double x, double y, double z) {
    vActingXYZn(eX) = x;
    vActingXYZn(eY) = y;
    vActingXYZn(eZ) = z;
  }
  inline void SetLocationX(double x) {vXYZn(eX) = x; vActingXYZn(eX) = x;}
  inline void SetLocationY(double y) {vXYZn(eY) = y; vActingXYZn(eY) = y;}
  inline void SetLocationZ(double z) {vXYZn(eZ) = z; vActingXYZn(eZ) = z;}
  inline void SetActingLocationX(double x) {vActingXYZn(eX) = x;}
  inline void SetActingLocationY(double y) {vActingXYZn(eY) = y;}
  inline void SetActingLocationZ(double z) {vActingXYZn(eZ) = z;}
  inline void SetLocation(const FGColumnVector3& vv) { vXYZn = vv; SetActingLocation(vv);}
  inline void SetActingLocation(const FGColumnVector3& vv) { vActingXYZn = vv; }

  inline double GetLocationX( void ) const { return vXYZn(eX);}
  inline double GetLocationY( void ) const { return vXYZn(eY);}
  inline double GetLocationZ( void ) const { return vXYZn(eZ);}
  inline double GetActingLocationX( void ) const { return vActingXYZn(eX);}
  inline double GetActingLocationY( void ) const { return vActingXYZn(eY);}
  inline double GetActingLocationZ( void ) const { return vActingXYZn(eZ);}
  const FGColumnVector3& GetLocation(void) const { return vXYZn; }
  const FGColumnVector3& GetActingLocation(void) const { return vActingXYZn; }

  //these angles are relative to body axes, not earth!!!!!
  //I'm using these because pitch, roll, and yaw are easy to visualize,
  //there's no equivalent to roll in wind axes i.e. alpha, ? , beta
  //making up new names or using these is a toss-up: either way people
  //are going to get confused.
  //They are in radians.

  void SetAnglesToBody(double broll, double bpitch, double byaw);
  inline void  SetAnglesToBody(const FGColumnVector3& vv) {
    SetAnglesToBody(vv(eRoll), vv(ePitch), vv(eYaw));
  }

  void UpdateCustomTransformMatrix(void);
  void SetPitch(double pitch) {vOrient(ePitch) = pitch; UpdateCustomTransformMatrix();}
  void SetYaw(double yaw) {vOrient(eYaw) = yaw; UpdateCustomTransformMatrix();}

  double GetPitch(void) const {return vOrient(ePitch);}
  double GetYaw(void) const {return vOrient(eYaw);}

  inline const FGColumnVector3& GetAnglesToBody(void) const {return vOrient;}
  inline double GetAnglesToBody(int axis) const {return vOrient(axis);}

  inline void SetTransformType(TransformType ii) { ttype=ii; }
  inline TransformType GetTransformType(void) const { return ttype; }

  const FGMatrix33& Transform(void) const;

protected:
  FGFDMExec *fdmex;
  std::shared_ptr<FGMassBalance> MassBalance;
  FGColumnVector3 vFn;
  FGColumnVector3 vMn;
  FGColumnVector3 vOrient;
  TransformType ttype;
  FGColumnVector3 vXYZn;
  FGColumnVector3 vActingXYZn;
  FGMatrix33 mT;

private:
  FGColumnVector3 vFb;
  FGColumnVector3 vM;

  void Debug(int from);
};
}
#endif

