/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:        JSBSim.hxx
 Author:        Curtis L. Olson
 Maintained by: Tony Peden, Curt Olson
 Date started:  02/01/1999

------ Copyright (C) 1999 - 2000  Curtis L. Olson (curt@flightgear.org) ------

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; either version 2 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
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
#include <Aircraft/aircraft.hxx>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_JSBSIMXX "$Header JSBSim.hxx,v 1.4 2000/10/22 14:02:16 jsb Exp $"

#define METERS_TO_FEET 3.2808398950
#define RADTODEG 57.2957795

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <simgear/misc/props.hxx>

#include <FDM/JSBSim/FGFDMExec.h>

class FGState;
class FGAtmosphere;
class FGFCS;
class FGPropulsion;
class FGMassBalance;
class FGAerodynamics;
class FGInertial;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPosition;
class FGAuxiliary;
class FGOutput;
class FGInitialCondition;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

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
    @version $Id: JSBSim.hxx,v 1.34 2002/04/02 05:40:49 jberndt Exp $
    @see main in file JSBSim.cpp (use main() wrapper for standalone usage)
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/JSBSim.hxx?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/JSBSim.cxx?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
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
    void set_Static_pressure(double p);

    /** Sets the atmospheric temperature
        @param T temperature in degrees rankine */
    void set_Static_temperature(double T);

    /** Sets the atmospheric density.
        @param rho air density slugs/cubic foot */
    void set_Density(double rho);

    /** Sets the velocity of the local airmass for wind modeling.
        @param wnorth velocity north in fps
        @param weast velocity east in fps
        @param wdown velocity down in fps*/
    void set_Velocities_Local_Airmass (double wnorth,
				       double weast,
				       double wdown );
    /// @name Position Parameter Update
    //@{


    /** Update the position based on inputs, positions, velocities, etc.
        @param multiloop number of times to loop through the FDM
	      @return true if successful */
    void update( int multiloop );
    bool ToggleDataLogging(bool state);
    bool ToggleDataLogging(void);
    void do_trim(void);
    void update_ic(void);

private:
    FGFDMExec *fdmex;
    FGInitialCondition *fgic;
    bool needTrim;

    FGState*        State;
    FGAtmosphere*   Atmosphere;
    FGFCS*          FCS;
    FGPropulsion*   Propulsion;
    FGMassBalance*  MassBalance;
    FGAircraft*     Aircraft;
    FGTranslation*  Translation;
    FGRotation*     Rotation;
    FGPosition*     Position;
    FGAuxiliary*    Auxiliary;
    FGAerodynamics* Aerodynamics;
    FGGroundReactions *GroundReactions;

    int runcount;
    float trim_elev;
    float trim_throttle;
    
    SGPropertyNode *startup_trim;
    SGPropertyNode *trimmed;
    SGPropertyNode *pitch_trim;
    SGPropertyNode *throttle_trim;
    SGPropertyNode *aileron_trim;
    SGPropertyNode *rudder_trim;
    SGPropertyNode *stall_warning;
    
    /* SGPropertyNode *elevator_pos_deg;
    SGPropertyNode *left_aileron_pos_deg;
    SGPropertyNode *right_aileron_pos_deg;
    SGPropertyNode *rudder_pos_deg;
    SGPropertyNode *flap_pos_deg; */

    
    SGPropertyNode *elevator_pos_pct;
    SGPropertyNode *left_aileron_pos_pct;
    SGPropertyNode *right_aileron_pos_pct;
    SGPropertyNode *rudder_pos_pct;
    SGPropertyNode *flap_pos_pct;
    
    SGPropertyNode *gear_pos_pct;
    
    void init_gear(void);
    void update_gear(void);
    
};


#endif // _JSBSIM_HXX


