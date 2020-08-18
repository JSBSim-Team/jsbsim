/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGAircraft.h
 Author:       Jon S. Berndt
 Date started: 12/12/98

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
12/12/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGAIRCRAFT_H
#define FGAIRCRAFT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <string>

#include "FGModel.h"
#include "math/FGMatrix33.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates an Aircraft and its systems.
<p> Owns all the parts (other classes) which make up this aircraft. This includes
    the Engines, Tanks, Propellers, Nozzles, Aerodynamic and Mass properties,
    landing gear, etc. These constituent parts may actually run as separate
    JSBSim models themselves, but the responsibility for initializing them and
    for retrieving their force and moment contributions falls to FGAircraft.
<p> The \<metrics> section of the aircraft configuration file is read here, and
    the metrical information is held by this class.
<h3>Configuration File Format for \<metrics> Section:</h3>
@code{.xml}
    <metrics>
        <wingarea unit="{FT2 | M2}"> {number} </wingarea>
        <wingspan unit="{FT | M}"> {number} </wingspan>
        <chord unit="{FT | M}"> {number} </chord>
        <htailarea unit="{FT2 | M2}"> {number} </htailarea>
        <htailarm unit="{FT | M}"> {number} </htailarm>
        <vtailarea unit="{FT2 | M}"> {number} </vtailarea>
        <vtailarm unit="{FT | M}"> {number} </vtailarm>
        <wing_incidence unit="{RAD | DEG}"> {number} </wing_incidence>
        <location name="{AERORP | EYEPOINT | VRP}" unit="{IN | M}">
            <x> {number} </x>
            <y> {number} </y>
            <z> {number} </z>
        </location>
        {other location blocks}
    </metrics>
@endcode

    @author Jon S. Berndt
    @see Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
     Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
     School, January 1994
    @see D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
     JSC 12960, July 1977
    @see Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
     NASA-Ames", NASA CR-2497, January 1975
    @see Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
     Wiley & Sons, 1979 ISBN 0-471-03032-5
    @see Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
     1982 ISBN 0-471-08936-2
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGAircraft : public FGModel {
public:
  /** Constructor
      @param Executive a pointer to the parent executive object */
  FGAircraft(FGFDMExec *Executive);

  /// Destructor
  ~FGAircraft() override;

  /** Runs the Aircraft model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @see JSBSim.cpp documentation
      @return false if no error */
  bool Run(bool Holding) override;

  bool InitModel(void) override;

  /** Loads the aircraft.
      The executive calls this method to load the aircraft into JSBSim.
      @param el a pointer to the element tree
      @return true if successful */
  bool Load(Element* el) override;

  /** Gets the aircraft name
      @return the name of the aircraft as a string type */
  const std::string& GetAircraftName(void) const { return AircraftName; }

  /// Gets the wing area
  double GetWingArea(void) const { return WingArea; }
  /// Gets the wing span
  double GetWingSpan(void) const { return WingSpan; }
  /// Gets the average wing chord
  double Getcbar(void) const { return cbar; }
  double GetWingIncidence(void) const { return WingIncidence; }
  double GetWingIncidenceDeg(void) const { return WingIncidence*radtodeg; }
  double GetHTailArea(void) const { return HTailArea; }
  double GetHTailArm(void)  const { return HTailArm; }
  double GetVTailArea(void) const { return VTailArea; }
  double GetVTailArm(void)  const { return VTailArm; }
  double Getlbarh(void) const { return lbarh; } // HTailArm / cbar
  double Getlbarv(void) const { return lbarv; } // VTailArm / cbar
  double Getvbarh(void) const { return vbarh; } // H. Tail Volume
  double Getvbarv(void) const { return vbarv; } // V. Tail Volume
  const FGColumnVector3& GetMoments(void) const { return vMoments; }
  double GetMoments(int idx) const { return vMoments(idx); }
  const FGColumnVector3& GetForces(void) const { return vForces; }
  double GetForces(int idx) const { return vForces(idx); }
  /** Gets the the aero reference point (RP) coordinates.
      @return a vector containing the RP coordinates in the structural frame. */
  const FGColumnVector3& GetXYZrp(void) const { return vXYZrp; }
  const FGColumnVector3& GetXYZvrp(void) const { return vXYZvrp; }
  const FGColumnVector3& GetXYZep(void) const { return vXYZep; }
  double GetXYZrp(int idx) const { return vXYZrp(idx); }
  double GetXYZvrp(int idx) const { return vXYZvrp(idx); }
  double GetXYZep(int idx) const { return vXYZep(idx); }
  void SetAircraftName(const std::string& name) {AircraftName = name;}

  void SetXYZrp(int idx, double value) {vXYZrp(idx) = value;}

  void SetWingArea(double S) {WingArea = S;}

  struct Inputs {
    FGColumnVector3 AeroForce;
    FGColumnVector3 PropForce;
    FGColumnVector3 GroundForce;
    FGColumnVector3 ExternalForce;
    FGColumnVector3 BuoyantForce;
    FGColumnVector3 AeroMoment;
    FGColumnVector3 PropMoment;
    FGColumnVector3 GroundMoment;
    FGColumnVector3 ExternalMoment;
    FGColumnVector3 BuoyantMoment;
  } in;

private:
  FGColumnVector3 vMoments;
  FGColumnVector3 vForces;
  FGColumnVector3 vXYZrp;
  FGColumnVector3 vXYZvrp;
  FGColumnVector3 vXYZep;
  FGColumnVector3 vDXYZcg;

  double WingArea, WingSpan, cbar, WingIncidence;
  double HTailArea, VTailArea, HTailArm, VTailArm;
  double lbarh,lbarv,vbarh,vbarv;
  std::string AircraftName;

  void bind(void);
  void unbind(void);
  void Debug(int from) override;
};

} // namespace JSBSim
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
