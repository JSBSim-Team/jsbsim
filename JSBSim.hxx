/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:        JSBSim.hxx
 Author:        Curtis L. Olson
 Maintained by: Tony Peden, Curt Olson
 Date started:  02/01/1999

---------- Copyright (C) 1999  Curtis L. Olson (curt@flightgear.org) ----------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
02/01/1999   CLO   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef _JSBSIM_HXX
#define _JSBSIM_HXX

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <FDM/JSBSim/FGFDMExec.h>
#include <FDM/JSBSim/FGInitialCondition.h>
#undef MAX_ENGINES
#include <Aircraft/aircraft.hxx>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_JSBSIMXX "$Header JSBSim.hxx,v 1.4 2000/10/22 14:02:16 jsb Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** FGFS / JSBSim interface (a.k.a. "The Bus")
    This class provides for an interface between FlightGear and its data
    structures and JSBSim and its data structures.
    @author Curtis L. Olson (original)
    @author Tony Peden (Maintained and refined)
    @version $Id: JSBSim.hxx,v 1.5 2000/10/22 18:42:39 jsb Exp $
    @see -
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGJSBsim: public FGInterface {

public:
    FGJSBsim::FGJSBsim(void);
    FGJSBsim::~FGJSBsim();

    /// copy FDM state to LaRCsim structures
    bool copy_to_JSBsim();

    /// copy FDM state from LaRCsim structures
    bool copy_from_JSBsim();

    /// reset flight params to a specific position
    bool init( double dt );

    /// Positions
    void set_Latitude(double lat);  //geocentric
    void set_Longitude(double lon);
    void set_Altitude(double alt);        // triggers re-calc of AGL altitude

    //void set_AltitudeAGL(double altagl); // and vice-versa

    //Speeds -- setting any of these will trigger a re-calc of the rest

    void set_V_calibrated_kts(double vc);
    void set_Mach_number(double mach);
    void set_Velocities_Local( double north, double east, double down );
    void set_Velocities_Wind_Body( double u, double v, double w);

    /// Euler angles
    void set_Euler_Angles( double phi, double theta, double psi );

    /// Flight Path
    void set_Climb_Rate( double roc);
    void set_Gamma_vert_rad( double gamma);

    /// Earth
    void set_Sea_level_radius(double slr);
    void set_Runway_altitude(double ralt);

    /// Atmosphere
    void set_Static_pressure(double p);
    void set_Static_temperature(double T);
    void set_Density(double rho);
    void set_Velocities_Local_Airmass (double wnorth,
                                         double weast,
                                           double wdown );

    /// update position based on inputs, positions, velocities, etc.
    bool update( int multiloop );

private:
    // The aircraft for this instance
    FGFDMExec *fdmex;
    FGInitialCondition *fgic;
    bool needTrim;
    
    bool trimmed;
    float trim_elev;
    float trim_throttle;
};

#endif // _JSBSIM_HXX


