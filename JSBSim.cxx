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
// $Id: JSBSim.cxx,v 1.39 2000/10/22 14:02:15 jsb Exp $


#include <simgear/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/fgpath.hxx>

#include <Scenery/scenery.hxx>

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Main/options.hxx>
#include <Main/bfi.hxx>
//#include <WeatherCM/FGLocalWeatherDatabase.h>

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
#include <FDM/JSBSim/FGTrim.h>
#include <FDM/JSBSim/FGAtmosphere.h>


#include "JSBSim.hxx"

/******************************************************************************/

FGJSBsim::FGJSBsim(void) {
  bool result;
  
  fdmex=new FGFDMExec;
  fgic=new FGInitialCondition(fdmex);
  needTrim=true;
  
  FGPath aircraft_path( globals->get_options()->get_fg_root() );
  aircraft_path.append( "Aircraft" );

  FGPath engine_path( globals->get_options()->get_fg_root() );
  engine_path.append( "Engine" );
  float dt = 1.0 / globals->get_options()->get_model_hz();
  fdmex->GetState()->Setdt( dt );

  result = fdmex->LoadModel( aircraft_path.str(),
                                       engine_path.str(),
                                       globals->get_options()->get_aircraft() );
  int Neng=fdmex->GetAircraft()->GetNumEngines();
  FG_LOG(FG_FLIGHT,FG_INFO, "Neng: " << Neng );
  for(int i=0;i<Neng;i++) {
    add_engine( FGEngInterface() );
  }  

}

/******************************************************************************/
FGJSBsim::~FGJSBsim(void) {
  if(fdmex != NULL) {
    delete fdmex;
    delete fgic;
  }  
}

/******************************************************************************/

// Initialize the JSBsim flight model, dt is the time increment for
// each subsequent iteration through the EOM

bool FGJSBsim::init( double dt ) {

  bool result;

  FG_LOG( FG_FLIGHT, FG_INFO, "Starting and initializing JSBsim" );

/*   FGPath aircraft_path( globals->get_options()->get_fg_root() );
  aircraft_path.append( "Aircraft" );

  FGPath engine_path( globals->get_options()->get_fg_root() );
  engine_path.append( "Engine" );

  fdmex->GetState()->Setdt( dt );

  result = fdmex->LoadModel( aircraft_path.str(),
                                       engine_path.str(),
                                       globals->get_options()->get_aircraft() );
 */

  if (result) {
    FG_LOG( FG_FLIGHT, FG_INFO, "  loaded aircraft" << globals->get_options()->get_aircraft() );
  } else {
    FG_LOG( FG_FLIGHT, FG_INFO, "  aircraft" << globals->get_options()->get_aircraft()
                                << " does not exist");
    return 0;
  }

  fdmex->GetAtmosphere()->UseInternal();
  
  FG_LOG( FG_FLIGHT, FG_INFO, "  Initializing JSBSim with:" );
  switch(fgic->GetSpeedSet()) {
    case setned:
      FG_LOG(FG_FLIGHT,FG_INFO, "  Vn,Ve,Vd= " 
             << fdmex->GetPosition()->GetVn()
             << ", " << fdmex->GetPosition()->GetVe()
             << ", " << fdmex->GetPosition()->GetVd()
             << " ft/s");
      break;       
    case setuvw:
      FG_LOG(FG_FLIGHT,FG_INFO, "  U,V,W= " 
             << fdmex->GetTranslation()->GetUVW()(1)
             << ", " << fdmex->GetTranslation()->GetUVW()(2)
             << ", " << fdmex->GetTranslation()->GetUVW()(3)
             << " ft/s");
      break;       
    case setmach:
      FG_LOG(FG_FLIGHT,FG_INFO, "  Mach: " 
             << fdmex->GetTranslation()->GetMach() );
      break;
    case setvc:
    default:
       FG_LOG(FG_FLIGHT,FG_INFO, "  Indicated Airspeed: " 
             << fdmex->GetAuxiliary()->GetVcalibratedKTS() << " knots" );
      
  }

  //FG_LOG( FG_FLIGHT, FG_INFO, "  gamma: " <<  globals->get_options()->get_Gamma());
  FG_LOG( FG_FLIGHT, FG_INFO, "  Bank Angle: " 
              <<  fdmex->GetRotation()->Getphi()*RADTODEG << " deg");
  FG_LOG( FG_FLIGHT, FG_INFO, "  Pitch Angle: " 
              << fdmex->GetRotation()->Gettht()*RADTODEG << " deg"  );
  FG_LOG( FG_FLIGHT, FG_INFO, "  True Heading: " 
              << fdmex->GetRotation()->Getpsi()*RADTODEG << " deg"  );
  FG_LOG( FG_FLIGHT, FG_INFO, "  Latitude: " 
              <<  fdmex->GetPosition()->GetLatitude() << " deg" );
  FG_LOG( FG_FLIGHT, FG_INFO, "  Longitude: " 
              <<  fdmex->GetPosition()->GetLongitude() << " deg"  );
  
  // for debug only
  /* FG_LOG( FG_FLIGHT, FG_DEBUG, "  FGJSBSim::get_Altitude(): " <<  get_Altitude() );
  FG_LOG( FG_FLIGHT, FG_DEBUG, "  FGJSBSim::get_Sea_level_radius(): " << get_Sea_level_radius()  );
  FG_LOG( FG_FLIGHT, FG_DEBUG, "  scenery.cur_radius*METER_TO_FEET: "
          <<  scenery.cur_radius*METER_TO_FEET );
  FG_LOG( FG_FLIGHT, FG_DEBUG, "  Calculated Terrain ASL: " << endl 
                           << "    " << "scenery.cur_radius*METER_TO_FEET -get_Sea_level_radius()= " 
                           <<  scenery.cur_radius*METER_TO_FEET - get_Sea_level_radius()  );

  FG_LOG( FG_FLIGHT, FG_DEBUG, "  Calculated Aircraft AGL: " << endl 
                           << "    " << "get_Altitude() + get_Sea_level_radius() - scenery.cur_radius*METER_TO_FEET= " 
                           <<  get_Altitude() + get_Sea_level_radius()- scenery.cur_radius*METER_TO_FEET );
  FG_LOG( FG_FLIGHT, FG_DEBUG, "  globals->get_options()->get_altitude(): " 
          <<  globals->get_options()->get_altitude() );
  FG_LOG( FG_FLIGHT, FG_DEBUG, "  FGBFI::getAltitude(): " 
          <<  FGBFI::getAltitude() );    */


  FG_LOG( FG_FLIGHT, FG_INFO, "  loaded initial conditions" );

  FG_LOG( FG_FLIGHT, FG_INFO, "  set dt" );

  FG_LOG( FG_FLIGHT, FG_INFO, "Finished initializing JSBSim" );

  return 1;
}

/******************************************************************************/

// Run an iteration of the EOM (equations of motion)

bool FGJSBsim::update( int multiloop ) {
  
  int i;
  
  double save_alt = 0.0;
 
  
  // lets try to avoid really screwing up the JSBsim model
  if ( get_Altitude() < -9000 ) {
    save_alt = get_Altitude();
    set_Altitude( 0.0 );
  }

  
  
  if(needTrim) {
    FGTrim *fgtrim=new FGTrim(fdmex,fgic,tLongitudinal);
    if(!fgtrim->DoTrim()) {
      fgtrim->Report();
      fgtrim->TrimStats();
    }  
    fgtrim->ReportState();
    delete fgtrim;
    
    needTrim=false;
    
    controls.set_elevator_trim(fdmex->GetFCS()->GetPitchTrimCmd());
    controls.set_elevator(fdmex->GetFCS()->GetDeCmd());
    controls.set_throttle(FGControls::ALL_ENGINES,
                            fdmex->GetFCS()->GetThrottleCmd(0)/100.0);
    controls.set_aileron(fdmex->GetFCS()->GetDaCmd());
    controls.set_rudder(fdmex->GetFCS()->GetDrCmd());
    
    FG_LOG( FG_FLIGHT, FG_INFO, "  Trim complete" );
  }  
  
  for( i=0; i<get_num_engines(); i++ ) {
    get_engine(i)->set_RPM( controls.get_throttle(i)*2700 );
    get_engine(i)->set_Throttle( controls.get_throttle(i) );
  }
  copy_to_JSBsim();
  
  for ( int i = 0; i < multiloop; i++ ) {
    fdmex->Run();
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
  
  //climb rate now set from FDM in copy_from_x()
  return 1;
}

/******************************************************************************/

// Convert from the FGInterface struct to the JSBsim generic_ struct

bool FGJSBsim::copy_to_JSBsim() {
  // copy control positions into the JSBsim structure

  fdmex->GetFCS()->SetDaCmd( controls.get_aileron());
  fdmex->GetFCS()->SetDeCmd( controls.get_elevator());
  fdmex->GetFCS()->SetPitchTrimCmd(controls.get_elevator_trim());
  fdmex->GetFCS()->SetDrCmd( -1*controls.get_rudder());
  fdmex->GetFCS()->SetDfCmd( controls.get_flaps() );
  fdmex->GetFCS()->SetDsbCmd( 0.0 ); //speedbrakes
  fdmex->GetFCS()->SetDspCmd( 0.0 ); //spoilers
  fdmex->GetFCS()->SetThrottleCmd( FGControls::ALL_ENGINES,
                                    controls.get_throttle( 0 ) * 100.0 );

  fdmex->GetFCS()->SetLBrake( controls.get_brake( 0 ) );
  fdmex->GetFCS()->SetRBrake( controls.get_brake( 1 ) );
  fdmex->GetFCS()->SetCBrake( controls.get_brake( 2 ) );

  fdmex->GetPosition()->SetRunwayRadius(scenery.cur_radius*METER_TO_FEET);
  fdmex->GetPosition()->SetSeaLevelRadius(get_Sea_level_radius());

  fdmex->GetAtmosphere()->SetExTemperature(get_Static_temperature());
  fdmex->GetAtmosphere()->SetExPressure(get_Static_pressure());
  fdmex->GetAtmosphere()->SetExDensity(get_Density());
  fdmex->GetAtmosphere()->SetWindNED(get_V_north_airmass(),
                                      get_V_east_airmass(),
                                      get_V_down_airmass());

  return true;
}

/******************************************************************************/

// Convert from the JSBsim generic_ struct to the FGInterface struct

bool FGJSBsim::copy_from_JSBsim() {
  unsigned i, j;
  
  
  mass=fdmex->GetAircraft()->GetMass();
  i_xx = fdmex->GetAircraft()->GetIxx();
  i_yy = fdmex->GetAircraft()->GetIyy();
  i_zz = fdmex->GetAircraft()->GetIzz();
  i_xz = fdmex->GetAircraft()->GetIxz();
  
  d_cg_rp_body_v[0] = fdmex->GetAircraft()->GetXYZcg()(1);
  d_cg_rp_body_v[1] = fdmex->GetAircraft()->GetXYZcg()(2);
  d_cg_rp_body_v[2] = fdmex->GetAircraft()->GetXYZcg()(3);
  
  v_dot_body_v[0] = fdmex->GetTranslation()->GetUVWdot()(1);
  v_dot_body_v[1] = fdmex->GetTranslation()->GetUVWdot()(2);
  v_dot_body_v[2] = fdmex->GetTranslation()->GetUVWdot()(3);
  
  a_cg_body_v[0] =  fdmex->GetTranslation()->GetUVWdot()(1);
  a_cg_body_v[1] =  fdmex->GetTranslation()->GetUVWdot()(2);
  a_cg_body_v[2] =  fdmex->GetTranslation()->GetUVWdot()(3);
                       
  
  //set_Accels_CG_Body_N ( fdmex->GetTranslation()->GetNcg()(1),
  //                       fdmex->GetTranslation()->GetNcg()(2),
  //                       fdmex->GetTranslation()->GetNcg()(3) );
  //
  a_pilot_body_v[0] = fdmex->GetAuxiliary()->GetPilotAccel()(1);
	a_pilot_body_v[1] = fdmex->GetAuxiliary()->GetPilotAccel()(2);
	a_pilot_body_v[2] = fdmex->GetAuxiliary()->GetPilotAccel()(3);
  
  //set_Accels_Pilot_Body_N( fdmex->GetAuxiliary()->GetNpilot()(1),
  //                         fdmex->GetAuxiliary()->GetNpilot()(2),
  //                         fdmex->GetAuxiliary()->GetNpilot()(3) );
  
  nlf=fdmex->GetAircraft()->GetNlf();                       
  
  // Velocities

  v_local_v[0] = fdmex->GetPosition()->GetVn();
  v_local_v[1] = fdmex->GetPosition()->GetVe();
  v_local_v[2] = fdmex->GetPosition()->GetVd();

  v_wind_body_v[0] = fdmex->GetTranslation()->GetUVW()(1);
  v_wind_body_v[1] = fdmex->GetTranslation()->GetUVW()(2);
  v_wind_body_v[2] = fdmex->GetTranslation()->GetUVW()(3);
  
  v_equiv_kts = fdmex->GetAuxiliary()->GetVequivalentKTS();

  //set_V_calibrated( fdmex->GetAuxiliary()->GetVcalibratedFPS() );

  v_calibrated_kts = fdmex->GetAuxiliary()->GetVcalibratedKTS();
  
  v_ground_speed = fdmex->GetPosition()->GetVground();

  omega_body_v[0] = fdmex->GetRotation()->GetPQR()(1);
  omega_body_v[1] = fdmex->GetRotation()->GetPQR()(2);
  omega_body_v[2] = fdmex->GetRotation()->GetPQR()(3);

  euler_rates_v[0] = fdmex->GetRotation()->GetEulerRates()(1);
  euler_rates_v[1] = fdmex->GetRotation()->GetEulerRates()(2);
  euler_rates_v[2] = fdmex->GetRotation()->GetEulerRates()(3);

 	geocentric_rates_v[0] = fdmex->GetPosition()->GetLatitudeDot();
 	geocentric_rates_v[1] = fdmex->GetPosition()->GetLongitudeDot();
 	geocentric_rates_v[2] = fdmex->GetPosition()->Gethdot();

  mach_number = fdmex->GetTranslation()->GetMach();

  // Positions

  double lat_geoc = fdmex->GetPosition()->GetLatitude();
  double lon = fdmex->GetPosition()->GetLongitude();
  double alt = fdmex->GetPosition()->Geth();
  double lat_geod, tmp_alt, sl_radius1, sl_radius2, tmp_lat_geoc;

  sgGeocToGeod( lat_geoc, EQUATORIAL_RADIUS_M + alt * FEET_TO_METER,
                &lat_geod, &tmp_alt, &sl_radius1 );
  sgGeodToGeoc( lat_geod, alt * FEET_TO_METER, &sl_radius2, &tmp_lat_geoc );

  FG_LOG( FG_FLIGHT, FG_DEBUG, "lon = " << lon << " lat_geod = " << lat_geod
          << " lat_geoc = " << lat_geoc
          << " alt = " << alt << " tmp_alt = " << tmp_alt * METER_TO_FEET
          << " sl_radius1 = " << sl_radius1 * METER_TO_FEET
          << " sl_radius2 = " << sl_radius2 * METER_TO_FEET
          << " Equator = " << EQUATORIAL_RADIUS_FT );

  geocentric_position_v[0] = lat_geoc,
  geocentric_position_v[1] = lon,
  geocentric_position_v[2] = sl_radius2 * METER_TO_FEET + alt ;
  
  geodetic_position_v[0] = lat_geod;
  geodetic_position_v[1] = lon;
  geodetic_position_v[2] = alt;
  
  euler_angles_v[0] = fdmex->GetRotation()->Getphi();
  euler_angles_v[1] = fdmex->GetRotation()->Gettht();
  euler_angles_v[2] = fdmex->GetRotation()->Getpsi();

  alpha = fdmex->GetTranslation()->Getalpha();
  beta  = fdmex->GetTranslation()->Getbeta();

  gamma_vert_rad=fdmex->GetPosition()->GetGamma();
  // set_Gamma_horiz_rad( Gamma_horiz_rad );

  /* **FIXME*** */ sea_level_radius = sl_radius2 * METER_TO_FEET;
  /* **FIXME*** */ earth_position_angle =
                     fdmex->GetAuxiliary()->GetEarthPositionAngle();

  /* ***FIXME*** */ runway_altitude = scenery.cur_radius*METERS_TO_FEET 
                                                        - sea_level_radius;

  sin_lat_geocentric = sin( lat_geoc );
  cos_lat_geocentric = cos( lat_geoc );
  
  sin_longitude = sin( lon );
  cos_longitude = cos( lon );
  
  sin_latitude = sin( lat_geod );
  cos_latitude = cos( lat_geod );
  
  climb_rate = fdmex->GetPosition()->Gethdot();
  
  
	for ( i = 0; i < 3; i++ ) {
	    for ( j = 0; j < 3; j++ ) {
		    t_local_to_body_m[i][j] = fdmex->GetState()->GetTl2b()(i,j);
	    }
	}
  return true;
}
    //Positions
void FGJSBsim::set_Latitude(double lat) {
  FG_LOG(FG_FLIGHT,FG_INFO,"FGJSBsim::set_Latitude: " << lat);
  fgic->SetLatitudeRadIC(lat);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus
  needTrim=true;
}  

void FGJSBsim::set_Longitude(double lon) {
  FG_LOG(FG_FLIGHT,FG_INFO,"FGJSBsim::set_Longitude: " << lon);
  fgic->SetLongitudeRadIC(lon);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus
  needTrim=true;
}  

void FGJSBsim::set_Altitude(double alt) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Altitude: " << alt );
  fgic->SetAltitudeFtIC(alt);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus
  needTrim=true;
}
  
void FGJSBsim::set_V_calibrated_kts(double vc) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_V_calibrated_kts: " <<  vc );
  fgic->SetVcalibratedKtsIC(vc);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus
  needTrim=true;
}  

void FGJSBsim::set_Mach_number(double mach) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Mach_number: " <<  mach );
  fgic->SetMachIC(mach);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus
  needTrim=true;
}  

void FGJSBsim::set_Velocities_Local( double north, double east, double down ){
 FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Velocities_Local: " 
                               << north << ", " <<  east << ", " << down ); 
 fgic->SetVnorthFpsIC(north);
 fgic->SetVeastFpsIC(east);
 fgic->SetVdownFpsIC(down);
 fdmex->RunIC(fgic); //loop JSBSim once
 cout << "fdmex->GetTranslation()->GetVt(): " << fdmex->GetTranslation()->GetVt() << endl;
 cout << "fdmex->GetPosition()->GetVn(): " << fdmex->GetPosition()->GetVn() << endl;

 copy_from_JSBsim(); //update the bus
 busdump();
 needTrim=true;
}  

void FGJSBsim::set_Velocities_Wind_Body( double u, double v, double w){
 FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Velocities_Wind_Body: " 
                              << u << ", " <<  v << ", " <<  w );
 
 fgic->SetUBodyFpsIC(u);
 fgic->SetVBodyFpsIC(v);
 fgic->SetWBodyFpsIC(w);
 fdmex->RunIC(fgic); //loop JSBSim once
 copy_from_JSBsim(); //update the bus
 needTrim=true;
} 

//Euler angles 
void FGJSBsim::set_Euler_Angles( double phi, double theta, double psi ) {
 FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Euler_Angles: " 
                             << phi << ", " << theta << ", " << psi );
 fgic->SetPitchAngleRadIC(theta);
 fgic->SetRollAngleRadIC(phi);
 fgic->SetTrueHeadingRadIC(psi);
 fdmex->RunIC(fgic); //loop JSBSim once
 copy_from_JSBsim(); //update the bus 
 needTrim=true;                                        
}  

//Flight Path
void FGJSBsim::set_Climb_Rate( double roc) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Climb_Rate: " << roc );
  fgic->SetClimbRateFpsIC(roc);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus 
  needTrim=true;                                        
}  

void FGJSBsim::set_Gamma_vert_rad( double gamma) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Gamma_vert_rad: " << gamma );
  fgic->SetFlightPathAngleRadIC(gamma);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus  
  needTrim=true;                                       
}  

//Earth
void FGJSBsim::set_Sea_level_radius(double slr) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Sea_level_radius: " << slr );
  fgic->SetSeaLevelRadiusFtIC(slr);
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus 
  needTrim=true;  
}  

void FGJSBsim::set_Runway_altitude(double ralt) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Runway_altitude: " << ralt );
  runway_altitude = ralt;
  fdmex->RunIC(fgic); //loop JSBSim once
  copy_from_JSBsim(); //update the bus 
  needTrim=true;  
}  

void FGJSBsim::set_Static_pressure(double p) { 
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Static_pressure: " << p );
  fdmex->GetAtmosphere()->SetExPressure(p);
  if(fdmex->GetAtmosphere()->External() == true)
    needTrim=true;
}
  
void FGJSBsim::set_Static_temperature(double T) { 
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Static_temperature: " << T );
  fdmex->GetAtmosphere()->SetExTemperature(T);
  if(fdmex->GetAtmosphere()->External() == true)
    needTrim=true;
}
 

void FGJSBsim::set_Density(double rho) {
  FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Density: " << rho );
  fdmex->GetAtmosphere()->SetExDensity(rho);
  if(fdmex->GetAtmosphere()->External() == true)
    needTrim=true;
}
  

void FGJSBsim::set_Velocities_Local_Airmass (double wnorth, 
                                              double weast, 
                                               double wdown ) {
     FG_LOG(FG_FLIGHT,FG_INFO, "FGJSBsim::set_Velocities_Local_Airmass: " 
                             << wnorth << ", " << weast << ", " << wdown );
     fdmex->GetAtmosphere()->SetWindNED(wnorth, weast, wdown );
     if(fdmex->GetAtmosphere()->External() == true)
        needTrim=true;
}     
