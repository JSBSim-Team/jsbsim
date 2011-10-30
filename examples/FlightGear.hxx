/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:        JSBSim.hxx
 Author:        Curtis L. Olson
 Maintained by: Tony Peden, Curt Olson
 Date started:  02/01/1999

------ Copyright (C) 1999 - 2000  Curtis L. Olson (curt@flightgear.org) ------

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public License as
 published by the Free Software Foundation; either version 2 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

HISTORY
--------------------------------------------------------------------------------
02/01/1999   CLO   Created
Additional log messages stored in CVS

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef _JSBSIM_HXX
#define _JSBSIM_HXX

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#undef MAX_ENGINES

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_JSBSIMXX "$Header JSBSim.hxx,v 1.4 2000/10/22 14:02:16 jsb Exp $"

#define METERS_TO_FEET 3.2808398950
#define RADTODEG 57.2957795

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <simgear/props/props.hxx>

#include <FDM/JSBSim/FGFDMExec.h>

namespace JSBSim {
class FGAtmosphere;
class FGWinds;
class FGFCS;
class FGPropulsion;
class FGMassBalance;
class FGAerodynamics;
class FGInertial;
class FGAircraft;
class FGPropagate;
class FGAuxiliary;
class FGOutput;
class FGInitialCondition;
class FGLocation;
class FGAccelerations;
}

// Adding it here will cause a namespace clash in FlightGear -EMH-
// using namespace JSBSim;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** FGFS / JSBSim interface (aka "The Bus").
    This class provides for an interface between FlightGear and its data
    structures and JSBSim and its data structures. This is the class which is
    used to command JSBSim when integrated with FlightGear. See the
    documentation for main for direction on running JSBSim apart from FlightGear.
    @author Curtis L. Olson (original)
    @author Tony Peden (Maintained and refined)
    @version $Id: FlightGear.hxx,v 1.9 2011/10/30 12:37:13 ehofman Exp $
    @see main in file JSBSim.cpp (use main() wrapper for standalone usage)
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGJSBsim: public FGInterface {

public:
    /// Constructor
    FGJSBsim( double dt );
    /// Destructor
    ~FGJSBsim();

    /// copy FDM state to LaRCsim structures
    bool copy_to_JSBsim();

    /// copy FDM state from LaRCsim structures
    bool copy_from_JSBsim();

    /// Reset flight params to a specific position
    void init();

    /// Unbind properties
    void unbind();

    /// Suspend integration
    void suspend();

    /// Resume integration
    void resume();

    /// @name Position Parameter Set
    //@{
    /** Set geocentric latitude
        @param lat latitude in radians measured from the 0 meridian where
                   the westerly direction is positive and east is negative */
    void set_Latitude(double lat);  // geocentric

    /** Set longitude
        @param lon longitude in radians measured from the equator where
                   the northerly direction is positive and south is negative */
    void set_Longitude(double lon);

    /** Set altitude
        Note: this triggers a recalculation of AGL altitude
        @param alt altitude in feet */
    void set_Altitude(double alt);        // triggers re-calc of AGL altitude
    //@}

    //void set_AltitudeAGL(double altagl); // and vice-versa

    /// @name Velocity Parameter Set
    //@{
    /** Sets calibrated airspeed
        Setting this will trigger a recalc of the other velocity terms.
        @param vc Calibrated airspeed in ft/sec */
    void set_V_calibrated_kts(double vc);

    /** Sets Mach number.
        Setting this will trigger a recalc of the other velocity terms.
        @param mach Mach number */
    void set_Mach_number(double mach);

    /** Sets velocity in N-E-D coordinates.
        Setting this will trigger a recalc of the other velocity terms.
        @param north velocity northward in ft/sec
        @param east velocity eastward in ft/sec
        @param down velocity downward in ft/sec */
    void set_Velocities_Local( double north, double east, double down );

    /** Sets aircraft velocity in stability frame.
        Setting this will trigger a recalc of the other velocity terms.
        @param u X velocity in ft/sec
        @param v Y velocity  in ft/sec
        @param w Z velocity in ft/sec */
    void set_Velocities_Wind_Body( double u, double v, double w);
    //@}

    /** Euler Angle Parameter Set
        @param phi roll angle in radians
        @param theta pitch angle in radians
        @param psi heading angle in radians */
    void set_Euler_Angles( double phi, double theta, double psi );

    /// @name Flight Path Parameter Set
    //@{
    /** Sets rate of climb
        @param roc Rate of climb in ft/sec */
    void set_Climb_Rate( double roc);

    /** Sets the flight path angle in radians
        @param gamma flight path angle in radians. */
    void set_Gamma_vert_rad( double gamma);
    //@}


    /// @name Atmospheric Parameter Set
    //@{
    /** Sets the atmospheric static pressure
        @param p pressure in psf */
//     void set_Static_pressure(double p);

    /** Sets the atmospheric temperature
        @param T temperature in degrees rankine */
//     void set_Static_temperature(double T);

    /** Sets the atmospheric density.
        @param rho air density slugs/cubic foot */
//     void set_Density(double rho);

    /** Sets the velocity of the local airmass for wind modeling.
        @param wnorth velocity north in fps
        @param weast velocity east in fps
        @param wdown velocity down in fps*/
    /// @name Position Parameter Update
    //@{


    /** Update the position based on inputs, positions, velocities, etc.
        @param dt delta time in seconds. */
    void update(double dt);

    bool ToggleDataLogging(bool state);
    bool ToggleDataLogging(void);

    bool get_agl_ft(double t, const double pt[3], double alt_off,
                    double contact[3], double normal[3], double vel[3],
                    double angularVel[3], double *agl);
private:
    JSBSim::FGFDMExec *fdmex;
    JSBSim::FGInitialCondition *fgic;
    bool needTrim;

    JSBSim::FGAtmosphere*      Atmosphere;
    JSBSim::FGWinds*           Winds;
    JSBSim::FGFCS*             FCS;
    JSBSim::FGPropulsion*      Propulsion;
    JSBSim::FGMassBalance*     MassBalance;
    JSBSim::FGAircraft*        Aircraft;
    JSBSim::FGPropagate*       Propagate;
    JSBSim::FGAuxiliary*       Auxiliary;
    JSBSim::FGAerodynamics*    Aerodynamics;
    JSBSim::FGGroundReactions* GroundReactions;
    JSBSim::FGInertial*        Inertial;
    JSBSim::FGAccelerations*   Accelerations;

    int runcount;
    double trim_elev;
    double trim_throttle;

    SGPropertyNode_ptr startup_trim;
    SGPropertyNode_ptr trimmed;
    SGPropertyNode_ptr pitch_trim;
    SGPropertyNode_ptr throttle_trim;
    SGPropertyNode_ptr aileron_trim;
    SGPropertyNode_ptr rudder_trim;
    SGPropertyNode_ptr stall_warning;

    /* SGPropertyNode_ptr elevator_pos_deg;
    SGPropertyNode_ptr left_aileron_pos_deg;
    SGPropertyNode_ptr right_aileron_pos_deg;
    SGPropertyNode_ptr rudder_pos_deg;
    SGPropertyNode_ptr flap_pos_deg; */


    SGPropertyNode_ptr elevator_pos_pct;
    SGPropertyNode_ptr left_aileron_pos_pct;
    SGPropertyNode_ptr right_aileron_pos_pct;
    SGPropertyNode_ptr rudder_pos_pct;
    SGPropertyNode_ptr flap_pos_pct;
    SGPropertyNode_ptr speedbrake_pos_pct;
    SGPropertyNode_ptr spoilers_pos_pct;

    SGPropertyNode_ptr ab_brake_engaged;
    SGPropertyNode_ptr ab_brake_left_pct;
    SGPropertyNode_ptr ab_brake_right_pct;
    
    SGPropertyNode_ptr gear_pos_pct;
    SGPropertyNode_ptr wing_fold_pos_pct;
    SGPropertyNode_ptr tailhook_pos_pct;

    SGPropertyNode_ptr temperature;
    SGPropertyNode_ptr pressure;
    SGPropertyNode_ptr pressureSL;
    SGPropertyNode_ptr ground_wind;
    SGPropertyNode_ptr turbulence_gain;
    SGPropertyNode_ptr turbulence_rate;
    SGPropertyNode_ptr turbulence_model;

    SGPropertyNode_ptr wind_from_north;
    SGPropertyNode_ptr wind_from_east;
    SGPropertyNode_ptr wind_from_down;

    SGPropertyNode_ptr slaved;

    static std::map<std::string,int> TURBULENCE_TYPE_NAMES;

    double last_hook_tip[3];
    double last_hook_root[3];
    JSBSim::FGColumnVector3 hook_root_struct;
    double hook_length;
    bool got_wire;

    bool crashed;

    void do_trim(void);

    bool update_ground_cache(JSBSim::FGLocation cart, double* cart_pos, double dt);
    void init_gear(void);
    void update_gear(void);

    void update_external_forces(double t_off);
};


#endif // _JSBSIM_HXX


