// JSBsim.cxx -- interface to the JSBsim flight model
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id: JSBSim.cxx,v 1.7 2000/05/03 12:29:06 jsb Exp $


#include <simgear/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/fg_geodesy.hxx>
#include <simgear/misc/fgpath.hxx>

#include <scenery/scenery.hxx>

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Main/options.hxx>

#include <FDM/JSBsim/FGFDMExec.h>
#include <FDM/JSBsim/FGAircraft.h>
#include <FDM/JSBsim/FGFCS.h>
#include <FDM/JSBsim/FGPosition.h>
#include <FDM/JSBsim/FGRotation.h>
#include <FDM/JSBsim/FGState.h>
#include <FDM/JSBsim/FGTranslation.h>
#include <FDM/JSBsim/FGAuxiliary.h>
#include <FDM/JSBsim/FGDefs.h>

#include "JSBsim.hxx"


/******************************************************************************/

// Initialize the JSBsim flight model, dt is the time increment for
// each subsequent iteration through the EOM

int FGJSBsim::init( double dt ) {

  FG_LOG( FG_FLIGHT, FG_INFO, "Starting and initializing JSBsim" );
  FG_LOG( FG_FLIGHT, FG_INFO, "  created FDMExec" );

  FGPath aircraft_path( current_options.get_fg_root() );
  aircraft_path.append( "Aircraft" );

  FGPath engine_path( current_options.get_fg_root() );
  engine_path.append( "Engine" );

  FDMExec.GetState()->Setdt( dt );

  FDMExec.GetAircraft()->LoadAircraft( aircraft_path.str(),
         engine_path.str(),
         current_options.get_aircraft() );

  FG_LOG( FG_FLIGHT, FG_INFO, "  loaded aircraft" <<
    current_options.get_aircraft() );

  FG_LOG( FG_FLIGHT, FG_INFO, "Initializing JSBsim with:" );
  FG_LOG( FG_FLIGHT, FG_INFO, "    U: " << current_options.get_uBody() );
  FG_LOG( FG_FLIGHT, FG_INFO, "    V: " <<current_options.get_vBody() );
  FG_LOG( FG_FLIGHT, FG_INFO, "    W: " <<current_options.get_wBody() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  phi: " <<get_Phi() );
  FG_LOG( FG_FLIGHT, FG_INFO, "theta: " <<  get_Theta() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  psi: " <<  get_Psi() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  lat: " <<  get_Latitude() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  lon: " <<  get_Longitude() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  alt: " <<  get_Altitude() );

  FDMExec.GetState()->Initialize(
    current_options.get_uBody(),
    current_options.get_vBody(),
    current_options.get_wBody(),
    get_Phi(),
    get_Theta(),
    get_Psi(),
    get_Latitude(),
    get_Longitude(),
    get_Altitude()
  );

  FG_LOG( FG_FLIGHT, FG_INFO, "  loaded initial conditions" );

  FG_LOG( FG_FLIGHT, FG_INFO, "  set dt" );

  FG_LOG( FG_FLIGHT, FG_INFO, "Finished initializing JSBsim" );

  copy_from_JSBsim();

  return 1;
}

/******************************************************************************/

// Run an iteration of the EOM (equations of motion)

int FGJSBsim::update( int multiloop ) {
  double save_alt = 0.0;
  double time_step = (1.0 / current_options.get_model_hz()) * multiloop;
  double start_elev = get_Altitude();

  // lets try to avoid really screwing up the JSBsim model
  if ( get_Altitude() < -9000 ) {
    save_alt = get_Altitude();
    set_Altitude( 0.0 );
  }

  // copy control positions into the JSBsim structure

  FDMExec.GetFCS()->SetDaCmd( controls.get_aileron());
  FDMExec.GetFCS()->SetDeCmd( controls.get_elevator()
                                             + controls.get_elevator_trim() );
  FDMExec.GetFCS()->SetDrCmd( controls.get_rudder());
  FDMExec.GetFCS()->SetDfCmd( 0.0 );
  FDMExec.GetFCS()->SetDsbCmd( 0.0 );
  FDMExec.GetFCS()->SetDspCmd( 0.0 );
  FDMExec.GetFCS()->SetThrottleCmd( FGControls::ALL_ENGINES,
                                         controls.get_throttle( 0 ) * 100.0 );
  FDMExec.GetFCS()->SetThrottlePos( FGControls::ALL_ENGINES,
                                         controls.get_throttle( 0 ) * 100.0 );
  // FCS->SetBrake( controls.get_brake( 0 ) );

  // Inform JSBsim of the local terrain altitude
  // Runway_altitude =   get_Runway_altitude();

  for ( int i = 0; i < multiloop; i++ ) {
    FDMExec.Run();
  }

  // printf("%d FG_Altitude = %.2f\n", i, FG_Altitude * 0.3048);
  // printf("%d Altitude = %.2f\n", i, Altitude * 0.3048);

  // translate JSBsim back to FG structure so that the
  // autopilot (and the rest of the sim can use the updated values

  copy_from_JSBsim();

  // but lets restore our original bogus altitude when we are done

  if ( save_alt < -9000.0 ) {
    set_Altitude( save_alt );
  }

  double end_elev = get_Altitude();
  if ( time_step > 0.0 ) {
    // feet per second
    set_Climb_Rate( (end_elev - start_elev) / time_step );
  }

  return 1;
}

/******************************************************************************/

// Convert from the FGInterface struct to the JSBsim generic_ struct

int FGJSBsim::copy_to_JSBsim() {
  return 1;
}

/******************************************************************************/

// Convert from the JSBsim generic_ struct to the FGInterface struct

int FGJSBsim::copy_from_JSBsim() {

  // Velocities

  set_Velocities_Local( FDMExec.GetPosition()->GetVn(),
      FDMExec.GetPosition()->GetVe(),
      FDMExec.GetPosition()->GetVd() );

  set_V_equiv_kts( FDMExec.GetAuxiliary()->GetVequivalentKTS() );

  //set_V_calibrated( FDMExec.GetAuxiliary()->GetVcalibratedFPS() );

  set_V_calibrated_kts( FDMExec.GetAuxiliary()->GetVcalibratedKTS() );

  set_Omega_Body( FDMExec.GetState()->GetParameter(FG_ROLLRATE),
      FDMExec.GetState()->GetParameter(FG_PITCHRATE),
      FDMExec.GetState()->GetParameter(FG_YAWRATE) );

  set_Euler_Rates( FDMExec.GetRotation()->Getphi(),
       FDMExec.GetRotation()->Gettht(),
       FDMExec.GetRotation()->Getpsi() );

  // ***FIXME*** set_Geocentric_Rates( Latitude_dot, Longitude_dot, Radius_dot );

  set_Mach_number( FDMExec.GetTranslation()->GetMach());

  // Positions

  double lat_geoc = FDMExec.GetPosition()->GetLatitude();
  double lon = FDMExec.GetPosition()->GetLongitude();
  double alt = FDMExec.GetPosition()->Geth();
  double lat_geod, tmp_alt, sl_radius1, sl_radius2, tmp_lat_geoc;

  fgGeocToGeod( lat_geoc, EQUATORIAL_RADIUS_M + alt * FEET_TO_METER,
    &lat_geod, &tmp_alt, &sl_radius1 );
  fgGeodToGeoc( lat_geod, alt * FEET_TO_METER, &sl_radius2, &tmp_lat_geoc );

  FG_LOG( FG_FLIGHT, FG_DEBUG, "lon = " << lon << " lat_geod = " << lat_geod
    << " lat_geoc = " << lat_geoc
    << " alt = " << alt << " tmp_alt = " << tmp_alt * METER_TO_FEET
    << " sl_radius1 = " << sl_radius1 * METER_TO_FEET
    << " sl_radius2 = " << sl_radius2 * METER_TO_FEET
    << " Equator = " << EQUATORIAL_RADIUS_FT );

  set_Geocentric_Position( lat_geoc, lon,
           sl_radius2 * METER_TO_FEET + alt );
  set_Geodetic_Position( lat_geod, lon, alt );
  set_Euler_Angles( FDMExec.GetRotation()->Getphi(),
        FDMExec.GetRotation()->Gettht(),
        FDMExec.GetRotation()->Getpsi() );

  set_Alpha( FDMExec.GetTranslation()->Getalpha() );
  set_Beta( FDMExec.GetTranslation()->Getbeta() );

  // ***ATTENDTOME*** set_Gamma_vert_rad( Gamma_vert_rad );
  // set_Gamma_horiz_rad( Gamma_horiz_rad );

  /* **FIXME*** */ set_Sea_level_radius( sl_radius2 * METER_TO_FEET );
  /* **FIXME*** */ set_Earth_position_angle( 0.0 );

  // /* ***FIXME*** */ set_Runway_altitude( 0.0 );
  set_Runway_altitude( scenery.cur_elev );
  
  FDMExec.Position()->SetRunwayElevation(scenery.cur_elev);

  set_sin_lat_geocentric( lat_geoc );
  set_cos_lat_geocentric( lat_geoc );
  set_sin_cos_longitude( lon );
  set_sin_cos_latitude( lat_geod );

  return 1;
}

/******************************************************************************/

