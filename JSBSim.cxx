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
// $Id: JSBSim.cxx,v 1.24 2000/07/25 11:51:41 jsb Exp $


#include <simgear/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/fg_geodesy.hxx>
#include <simgear/misc/fgpath.hxx>

#include <Scenery/scenery.hxx>

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Main/options.hxx>

#include <FDM/JSBSim/FGFDMExec.h>
#include <FDM/JSBSim/FGAircraft.h>
#include <FDM/JSBSim/FGFCS.h>
#include <FDM/JSBSim/FGPosition.h>
#include <FDM/JSBSim/FGRotation.h>
#include <FDM/JSBSim/FGState.h>
#include <FDM/JSBSim/FGTranslation.h>
#include <FDM/JSBSim/FGAuxiliary.h>
#include <FDM/JSBSim/FGDefs.h>
#include <FDM/JSBSim/FGInitialCondition.h>
#include <FDM/JSBSim/FGTrimLong.h>
#include <FDM/JSBSim/FGAtmosphere.h>

#include "JSBSim.hxx"

/******************************************************************************/

// Initialize the JSBsim flight model, dt is the time increment for
// each subsequent iteration through the EOM

int FGJSBsim::init( double dt ) {

  bool result;

  FG_LOG( FG_FLIGHT, FG_INFO, "Starting and initializing JSBsim" );
  FG_LOG( FG_FLIGHT, FG_INFO, "  created FDMExec" );

  FGPath aircraft_path( current_options.get_fg_root() );
  aircraft_path.append( "Aircraft" );

  FGPath engine_path( current_options.get_fg_root() );
  engine_path.append( "Engine" );

  FDMExec.GetState()->Setdt( dt );

  result = FDMExec.GetAircraft()->LoadAircraft( aircraft_path.str(),
                                       engine_path.str(),
                                       current_options.get_aircraft() );

  if (result) {
    FG_LOG( FG_FLIGHT, FG_INFO, "  loaded aircraft" << current_options.get_aircraft() );
  } else {
    FG_LOG( FG_FLIGHT, FG_INFO, "  aircraft" << current_options.get_aircraft()
                                << " does not exist");
    return 0;
  }

  FDMExec.GetAtmosphere()->UseInternal();

  FG_LOG( FG_FLIGHT, FG_INFO, "  Initializing JSBSim with:" );

  FGInitialCondition *fgic = new FGInitialCondition(&FDMExec);
  fgic->SetAltitudeFtIC(get_Altitude());
  if((current_options.get_mach() < 0) && (current_options.get_vc() < 0 )) {
    fgic->SetUBodyFpsIC(current_options.get_uBody());
    fgic->SetVBodyFpsIC(current_options.get_vBody());
    fgic->SetWBodyFpsIC(current_options.get_wBody());
    FG_LOG(FG_FLIGHT,FG_INFO, "  u,v,w= " << current_options.get_uBody()
           << ", " << current_options.get_vBody()
           << ", " << current_options.get_wBody());
  } else if (current_options.get_vc() < 0) {
    fgic->SetMachIC(current_options.get_mach());
    FG_LOG(FG_FLIGHT,FG_INFO, "  mach: " << current_options.get_mach() );
  } else {
    fgic->SetVcalibratedKtsIC(current_options.get_vc());
    FG_LOG(FG_FLIGHT,FG_INFO, "  vc: " << current_options.get_vc() );
    //this should cover the case in which no speed switches are used
    //current_options.get_vc() will return zero by default
  }


  fgic->SetRollAngleRadIC(get_Phi());
  fgic->SetPitchAngleRadIC(get_Theta());
  fgic->SetHeadingRadIC(get_Psi());
//  fgic->SetLatitudeRadIC(get_Latitude());
  fgic->SetLatitudeRadIC(get_Lat_geocentric());
  fgic->SetLongitudeRadIC(get_Longitude());

  FDMExec.GetPosition()->SetRunwayRadius(scenery.cur_radius*METER_TO_FEET);
  FDMExec.GetPosition()->SetSeaLevelRadius(get_Sea_level_radius());

  FG_LOG( FG_FLIGHT, FG_INFO, "  phi: " <<  get_Phi());
  FG_LOG( FG_FLIGHT, FG_INFO, "  theta: " <<  get_Theta() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  psi: " <<  get_Psi() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  lat: " <<  get_Latitude() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  lon: " <<  get_Longitude() );
  FG_LOG( FG_FLIGHT, FG_INFO, "  alt: " <<  get_Altitude() );

  //must check > 0, != 0 will give bad result if --notrim set
  if(current_options.get_trim_mode() > 0) {
    FDMExec.RunIC(fgic);
    FG_LOG( FG_FLIGHT, FG_INFO, "  Starting trim..." );
    FGTrimLong *fgtrim=new FGTrimLong(&FDMExec,fgic);
    fgtrim->DoTrim();
    fgtrim->Report();
    fgtrim->TrimStats();
    fgtrim->ReportState();

    controls.set_elevator_trim(FDMExec.GetFCS()->GetPitchTrimCmd());
    controls.set_throttle(FGControls::ALL_ENGINES,FDMExec.GetFCS()->GetThrottleCmd(0)/100);
    //the trimming routine only knows how to get 1 value for throttle

    
    delete fgtrim;
    FG_LOG( FG_FLIGHT, FG_INFO, "  Trim complete." );
  } else {
    FG_LOG( FG_FLIGHT, FG_INFO, "  Initializing without trim" );
    FDMExec.GetState()->Initialize(fgic);

  }

  delete fgic;

  FG_LOG( FG_FLIGHT, FG_INFO, "  loaded initial conditions" );

  FG_LOG( FG_FLIGHT, FG_INFO, "  set dt" );

  FG_LOG( FG_FLIGHT, FG_INFO, "Finished initializing JSBSim" );

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
  FDMExec.GetFCS()->SetDeCmd( controls.get_elevator());
  FDMExec.GetFCS()->SetPitchTrimCmd(controls.get_elevator_trim());
  FDMExec.GetFCS()->SetDrCmd( controls.get_rudder());
  FDMExec.GetFCS()->SetDfCmd( controls.get_flaps() );
  FDMExec.GetFCS()->SetDsbCmd( 0.0 ); //speedbrakes
  FDMExec.GetFCS()->SetDspCmd( 0.0 ); //spoilers
  FDMExec.GetFCS()->SetThrottleCmd( FGControls::ALL_ENGINES,
                                    controls.get_throttle( 0 ) * 100.0 );

  // FCS->SetBrake( controls.get_brake( 0 ) );

  // Inform JSBsim of the local terrain altitude; uncommented 5/3/00
  //  FDMExec.GetPosition()->SetRunwayElevation(get_Runway_altitude()); // seems to work
  FDMExec.GetPosition()->SetRunwayRadius(scenery.cur_radius*METER_TO_FEET);
  FDMExec.GetPosition()->SetSeaLevelRadius(get_Sea_level_radius());

  FDMExec.GetAtmosphere()->SetExTemperature(get_Static_temperature());
  FDMExec.GetAtmosphere()->SetExPressure(get_Static_pressure());
  FDMExec.GetAtmosphere()->SetExDensity(get_Density());
  FDMExec.GetAtmosphere()->SetWindNED(get_V_north_airmass(),
                                      get_V_east_airmass(),
                                      get_V_down_airmass());

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
  //if ( time_step > 0.0 ) {
    // feet per second
  //  set_Climb_Rate( (end_elev - start_elev) / time_step );
  //}

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

  set_Inertias( FDMExec.GetAircraft()->GetMass(),
                FDMExec.GetAircraft()->GetIxx(),
                FDMExec.GetAircraft()->GetIyy(),
                FDMExec.GetAircraft()->GetIzz(),
                FDMExec.GetAircraft()->GetIxz() );
  
  set_CG_Position ( FDMExec.GetAircraft()->GetXYZcg()(1),
                    FDMExec.GetAircraft()->GetXYZcg()(2),
                    FDMExec.GetAircraft()->GetXYZcg()(3) );
  
  set_Accels_Body ( FDMExec.GetTranslation()->GetUVWdot()(1),
                    FDMExec.GetTranslation()->GetUVWdot()(2),
                    FDMExec.GetTranslation()->GetUVWdot()(3) );
  
  set_Accels_CG_Body ( FDMExec.GetTranslation()->GetUVWdot()(1),
                       FDMExec.GetTranslation()->GetUVWdot()(2),
                       FDMExec.GetTranslation()->GetUVWdot()(3) );
  
  set_Accels_CG_Body_N ( FDMExec.GetTranslation()->GetNcg()(1),
                         FDMExec.GetTranslation()->GetNcg()(2),
                         FDMExec.GetTranslation()->GetNcg()(3) );
  
  set_Accels_Pilot_Body( FDMExec.GetAuxiliary()->GetPilotAccel()(1),
                         FDMExec.GetAuxiliary()->GetPilotAccel()(2),
                         FDMExec.GetAuxiliary()->GetPilotAccel()(3) );
  
  set_Accels_Pilot_Body_N( FDMExec.GetAuxiliary()->GetNpilot()(1),
                           FDMExec.GetAuxiliary()->GetNpilot()(2),
                           FDMExec.GetAuxiliary()->GetNpilot()(3) );
  
                           
  
  set_Nlf( FDMExec.GetAircraft()->GetNlf());                       
  
  
   
  // Velocities

  set_Velocities_Local( FDMExec.GetPosition()->GetVn(),
                        FDMExec.GetPosition()->GetVe(),
                        FDMExec.GetPosition()->GetVd() );

  set_Velocities_Wind_Body( FDMExec.GetTranslation()->GetUVW()(1),
                            FDMExec.GetTranslation()->GetUVW()(2),
                            FDMExec.GetTranslation()->GetUVW()(3)  );
  
  set_V_equiv_kts( FDMExec.GetAuxiliary()->GetVequivalentKTS() );

  //set_V_calibrated( FDMExec.GetAuxiliary()->GetVcalibratedFPS() );

  set_V_calibrated_kts( FDMExec.GetAuxiliary()->GetVcalibratedKTS() );
  
  set_V_ground_speed ( FDMExec.GetPosition()->GetVground() );

  set_Omega_Body( FDMExec.GetState()->GetParameter(FG_ROLLRATE),
                  FDMExec.GetState()->GetParameter(FG_PITCHRATE),
                  FDMExec.GetState()->GetParameter(FG_YAWRATE) );

  /* HUH!?! */ set_Euler_Rates( FDMExec.GetRotation()->Getphi(),
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

  set_Gamma_vert_rad( FDMExec.GetPosition()->GetGamma() );
  // set_Gamma_horiz_rad( Gamma_horiz_rad );
  
  

  /* **FIXME*** */ set_Sea_level_radius( sl_radius2 * METER_TO_FEET );
  /* **FIXME*** */ set_Earth_position_angle( 0.0 );

  // /* ***FIXME*** */ set_Runway_altitude( 0.0 );

  set_sin_lat_geocentric( lat_geoc );
  set_cos_lat_geocentric( lat_geoc );
  set_sin_cos_longitude( lon );
  set_sin_cos_latitude( lat_geod );
  
  set_Climb_Rate(FDMExec.GetPosition()->Gethdot());

  return 1;
}
