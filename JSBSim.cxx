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
// $Id: JSBSim.cxx,v 1.60 2001/04/24 22:14:42 jberndt Exp $


#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Scenery/scenery.hxx>

#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

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

FGJSBsim::FGJSBsim( double dt ) 
  : FGInterface(dt)
{
    bool result;
   
    fdmex=new FGFDMExec;
    
    State       = fdmex->GetState();
    Atmosphere  = fdmex->GetAtmosphere();
    FCS         = fdmex->GetFCS();
    MassBalance = fdmex->GetMassBalance();
    Propulsion  = fdmex->GetPropulsion();
    Aircraft    = fdmex->GetAircraft();
    Translation = fdmex->GetTranslation();
    Rotation    = fdmex->GetRotation();
    Position    = fdmex->GetPosition();
    Auxiliary   = fdmex->GetAuxiliary();

    fgic=new FGInitialCondition(fdmex);
    needTrim=true;
  
    SGPath aircraft_path( globals->get_fg_root() );
    aircraft_path.append( "Aircraft" );

    SGPath engine_path( globals->get_fg_root() );
    engine_path.append( "Engine" );
    set_delta_t( dt );
    State->Setdt( dt );

    result = fdmex->LoadModel( aircraft_path.str(),
                               engine_path.str(),
                               fgGetString("/sim/aircraft") );
    
    int Neng = Propulsion->GetNumEngines();
    SG_LOG(SG_FLIGHT,SG_INFO, "Neng: " << Neng );
    
    for(int i=0;i<Neng;i++) {
        add_engine( FGEngInterface() );
    }  
    
    fgSetDouble("/fdm/trim/pitch-trim", FCS->GetPitchTrimCmd());
    fgSetDouble("/fdm/trim/throttle",   FCS->GetThrottleCmd(0));
    fgSetDouble("/fdm/trim/aileron",    FCS->GetDaCmd());
    fgSetDouble("/fdm/trim/rudder",     FCS->GetDrCmd());

    trimmed = fgGetValue("/fdm/trim/trimmed",true);
    trimmed->setBoolValue(false);

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

void FGJSBsim::init() {
                // Explicitly call the superclass's
                // init method first.
    FGInterface::init();

    bool result;

    SG_LOG( SG_FLIGHT, SG_INFO, "Starting and initializing JSBsim" );

    Atmosphere->UseInternal();

    SG_LOG( SG_FLIGHT, SG_INFO, "  Initializing JSBSim with:" );

    switch(fgic->GetSpeedSet()) {
    case setned:
        SG_LOG(SG_FLIGHT,SG_INFO, "  Vn,Ve,Vd= "
               << Position->GetVn() << ", "
               << Position->GetVe() << ", "
               << Position->GetVd() << " ft/s");
    break;
    case setuvw:
        SG_LOG(SG_FLIGHT,SG_INFO, "  U,V,W= "
               << Translation->GetUVW(1) << ", "
               << Translation->GetUVW(2) << ", "
               << Translation->GetUVW(3) << " ft/s");
    break;
    case setmach:
        SG_LOG(SG_FLIGHT,SG_INFO, "  Mach: "
               << Translation->GetMach() );
    break;
    case setvc:
    default:
        SG_LOG(SG_FLIGHT,SG_INFO, "  Indicated Airspeed: "
               << Auxiliary->GetVcalibratedKTS() << " knots" );
    break;
    }

    SG_LOG( SG_FLIGHT, SG_INFO, "  Bank Angle: "
            <<  Rotation->Getphi()*RADTODEG << " deg");
    SG_LOG( SG_FLIGHT, SG_INFO, "  Pitch Angle: "
            << Rotation->Gettht()*RADTODEG << " deg"  );
    SG_LOG( SG_FLIGHT, SG_INFO, "  True Heading: "
            << Rotation->Getpsi()*RADTODEG << " deg"  );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Latitude: "
            <<  Position->GetLatitude() << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Longitude: "
            <<  Position->GetLongitude() << " deg"  );

    SG_LOG( SG_FLIGHT, SG_INFO, "  loaded initial conditions" );

    SG_LOG( SG_FLIGHT, SG_INFO, "  set dt" );

    SG_LOG( SG_FLIGHT, SG_INFO, "Finished initializing JSBSim" );
}

/******************************************************************************/

// Run an iteration of the EOM (equations of motion)

bool FGJSBsim::update( int multiloop ) {

    int i;

    double save_alt = 0.0;

    copy_to_JSBsim();

    trimmed->setBoolValue(false);

    if (needTrim && fgGetBool("/sim/startup/trim")) {

      //fgic->SetSeaLevelRadiusFtIC( get_Sea_level_radius() );
      //fgic->SetTerrainAltitudeFtIC( scenery.cur_elev * SG_METER_TO_FEET );

    FGTrim *fgtrim;
    if(fgic->GetVcalibratedKtsIC() < 10 ) {
        fgic->SetVcalibratedKtsIC(0.0);
        fgtrim=new FGTrim(fdmex,fgic,tGround);
    } else {
        fgtrim=new FGTrim(fdmex,fgic,tLongitudinal);
    }
    if(!fgtrim->DoTrim()) {
        fgtrim->Report();
        fgtrim->TrimStats();
    } else {
        trimmed->setBoolValue(true);
    }
    fgtrim->ReportState();
    delete fgtrim;

    needTrim=false;

    fgSetDouble("/fdm/trim/pitch-trim", FCS->GetPitchTrimCmd());
    fgSetDouble("/fdm/trim/throttle",   FCS->GetThrottleCmd(0));
    fgSetDouble("/fdm/trim/aileron",    FCS->GetDaCmd());
    fgSetDouble("/fdm/trim/rudder",     FCS->GetDrCmd());

    controls.set_elevator_trim(FCS->GetPitchTrimCmd());
    controls.set_elevator(FCS->GetDeCmd());
    controls.set_throttle(FGControls::ALL_ENGINES,
                          FCS->GetThrottleCmd(0));

    controls.set_aileron(FCS->GetDaCmd());
    controls.set_rudder( FCS->GetDrCmd());
    
    SG_LOG( SG_FLIGHT, SG_INFO, "  Trim complete" );
    }  
  
    for( i=0; i<get_num_engines(); i++ ) {
      get_engine(i)->set_RPM( Propulsion->GetThruster(i)->GetRPM() );
      get_engine(i)->set_Throttle( controls.get_throttle(i) );
    }

    for ( i=0; i < multiloop; i++ ) {
        fdmex->Run();
    }

    // printf("%d FG_Altitude = %.2f\n", i, FG_Altitude * 0.3048);
    // printf("%d Altitude = %.2f\n", i, Altitude * 0.3048);

    // translate JSBsim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated values

    copy_from_JSBsim();
    return true;
}

/******************************************************************************/

// Convert from the FGInterface struct to the JSBsim generic_ struct

bool FGJSBsim::copy_to_JSBsim() {
    // copy control positions into the JSBsim structure

    FCS->SetDaCmd( controls.get_aileron());
    FCS->SetDeCmd( controls.get_elevator());
    FCS->SetPitchTrimCmd(controls.get_elevator_trim());
    FCS->SetDrCmd( -controls.get_rudder());
    FCS->SetDfCmd(  controls.get_flaps() );
    FCS->SetDsbCmd( 0.0 ); //speedbrakes
    FCS->SetDspCmd( 0.0 ); //spoilers
    FCS->SetThrottleCmd( FGControls::ALL_ENGINES,
                         controls.get_throttle( 0 ));
    FCS->SetLBrake( controls.get_brake( 0 ) );
    FCS->SetRBrake( controls.get_brake( 1 ) );
    FCS->SetCBrake( controls.get_brake( 2 ) );

    Position->SetSeaLevelRadius( get_Sea_level_radius() );
    Position->SetRunwayRadius( scenery.cur_elev*SG_METER_TO_FEET
                               + get_Sea_level_radius() );

    Atmosphere->SetExTemperature(get_Static_temperature());
    Atmosphere->SetExPressure(get_Static_pressure());
    Atmosphere->SetExDensity(get_Density());
    Atmosphere->SetWindNED(get_V_north_airmass(),
                           get_V_east_airmass(),
                           get_V_down_airmass());
//    SG_LOG(SG_FLIGHT,SG_INFO, "Wind NED: "
//                  << get_V_north_airmass() << ", "
//                  << get_V_east_airmass()  << ", "
//                  << get_V_down_airmass() );

    return true;
}

/******************************************************************************/

// Convert from the JSBsim generic_ struct to the FGInterface struct

bool FGJSBsim::copy_from_JSBsim() {
    unsigned int i, j;

    _set_Inertias( MassBalance->GetMass(),
                   MassBalance->GetIxx(),
                   MassBalance->GetIyy(),
                   MassBalance->GetIzz(),
                   MassBalance->GetIxz() );

    _set_CG_Position( MassBalance->GetXYZcg(1),
                      MassBalance->GetXYZcg(2),
                      MassBalance->GetXYZcg(3) );

    _set_Accels_Body( Translation->GetUVWdot(1),
                      Translation->GetUVWdot(2),
                      Translation->GetUVWdot(3) );

    _set_Accels_CG_Body( Translation->GetUVWdot(1),
                         Translation->GetUVWdot(2),
                         Translation->GetUVWdot(3) );

    //_set_Accels_CG_Body_N ( Translation->GetNcg(1),
    //                       Translation->GetNcg(2),
    //                       Translation->GetNcg(3) );
    //
    _set_Accels_Pilot_Body( Auxiliary->GetPilotAccel(1),
                            Auxiliary->GetPilotAccel(2),
                            Auxiliary->GetPilotAccel(3) );

    //_set_Accels_Pilot_Body_N( Auxiliary->GetNpilot(1),
    //                         Auxiliary->GetNpilot(2),
    //                         Auxiliary->GetNpilot(3) );

    _set_Nlf( Aerodynamics->GetNlf() );

    // Velocities

    _set_Velocities_Local( Position->GetVn(),
                           Position->GetVe(),
                           Position->GetVd() );

    _set_Velocities_Wind_Body( Translation->GetUVW(1),
                               Translation->GetUVW(2),
                               Translation->GetUVW(3) );

    _set_V_rel_wind( Translation->GetVt() );

    _set_V_equiv_kts( Auxiliary->GetVequivalentKTS() );

    // _set_V_calibrated( Auxiliary->GetVcalibratedFPS() );

    _set_V_calibrated_kts( Auxiliary->GetVcalibratedKTS() );

    _set_V_ground_speed( Position->GetVground() );

    _set_Omega_Body( Rotation->GetPQR(1),
                     Rotation->GetPQR(2),
                     Rotation->GetPQR(3) );

    _set_Euler_Rates( Rotation->GetEulerRates(1),
                      Rotation->GetEulerRates(2),
                      Rotation->GetEulerRates(3) );

    _set_Geocentric_Rates(Position->GetLatitudeDot(),
                          Position->GetLongitudeDot(),
                          Position->Gethdot() );

    _set_Mach_number( Translation->GetMach() );

    // Positions
    _updatePosition( Position->GetLatitude(),
                     Position->GetLongitude(),
                     Position->Geth() );

    _set_Euler_Angles( Rotation->Getphi(),
                       Rotation->Gettht(),
                       Rotation->Getpsi() );

    _set_Alpha( Translation->Getalpha() );
    _set_Beta( Translation->Getbeta() );


    _set_Gamma_vert_rad( Position->GetGamma() );
    // set_Gamma_horiz_rad( Gamma_horiz_rad );

    _set_Earth_position_angle( Auxiliary->GetEarthPositionAngle() );

    _set_Climb_Rate( Position->Gethdot() );


    for ( i = 1; i <= 3; i++ ) {
        for ( j = 1; j <= 3; j++ ) {
            _set_T_Local_to_Body( i, j, State->GetTl2b(i,j) );
        }
    }
    return true;
}

void FGJSBsim::snap_shot(void) {
    fgic->SetLatitudeRadIC(get_Lat_geocentric() );
    fgic->SetLongitudeRadIC( get_Longitude() );
    fgic->SetAltitudeFtIC( get_Altitude() );
    fgic->SetTerrainAltitudeFtIC( get_Runway_altitude() );
    fgic->SetVtrueFpsIC( get_V_rel_wind() );
    fgic->SetPitchAngleRadIC( get_Theta() );
    fgic->SetRollAngleRadIC( get_Phi() );
    fgic->SetTrueHeadingRadIC( get_Psi() );
    fgic->SetClimbRateFpsIC( get_Climb_Rate() );
}


bool FGJSBsim::ToggleDataLogging(void) {
    return fdmex->GetOutput()->Toggle();
}


bool FGJSBsim::ToggleDataLogging(bool state) {
    if (state) {
      fdmex->GetOutput()->Enable();
      return true;
    } else {
      fdmex->GetOutput()->Disable();
      return false;
    }
}


//Positions
void FGJSBsim::set_Latitude(double lat) {
    double sea_level_radius_meters,lat_geoc;

    SG_LOG(SG_FLIGHT,SG_INFO,"FGJSBsim::set_Latitude: " << lat );

    snap_shot();
    sgGeodToGeoc( lat, get_Altitude() , &sea_level_radius_meters, &lat_geoc);
    _set_Sea_level_radius( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetSeaLevelRadiusFtIC( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetLatitudeRadIC( lat_geoc );
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Longitude(double lon) {

    SG_LOG(SG_FLIGHT,SG_INFO,"FGJSBsim::set_Longitude: " << lon );

    snap_shot();
    fgic->SetLongitudeRadIC(lon);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Altitude(double alt) {
    double sea_level_radius_meters,lat_geoc;

    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Altitude: " << alt );

    snap_shot();
    sgGeodToGeoc( get_Latitude(), alt , &sea_level_radius_meters, &lat_geoc);
    _set_Sea_level_radius( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetSeaLevelRadiusFtIC( sea_level_radius_meters * SG_METER_TO_FEET );
    fgic->SetLatitudeRadIC( lat_geoc );
    fgic->SetAltitudeFtIC(alt);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_V_calibrated_kts(double vc) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_V_calibrated_kts: " <<  vc );

    snap_shot();
    fgic->SetVcalibratedKtsIC(vc);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Mach_number(double mach) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Mach_number: " <<  mach );

    snap_shot();
    fgic->SetMachIC(mach);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Velocities_Local( double north, double east, double down ){
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Velocities_Local: "
       << north << ", " <<  east << ", " << down );

    snap_shot();
    fgic->SetVnorthFpsIC(north);
    fgic->SetVeastFpsIC(east);
    fgic->SetVdownFpsIC(down);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Velocities_Wind_Body( double u, double v, double w){
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Velocities_Wind_Body: "
       << u << ", " <<  v << ", " <<  w );

    snap_shot();
    fgic->SetUBodyFpsIC(u);
    fgic->SetVBodyFpsIC(v);
    fgic->SetWBodyFpsIC(w);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

//Euler angles
void FGJSBsim::set_Euler_Angles( double phi, double theta, double psi ) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Euler_Angles: "
       << phi << ", " << theta << ", " << psi );

    snap_shot();
    fgic->SetPitchAngleRadIC(theta);
    fgic->SetRollAngleRadIC(phi);
    fgic->SetTrueHeadingRadIC(psi);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

//Flight Path
void FGJSBsim::set_Climb_Rate( double roc) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Climb_Rate: " << roc );

    snap_shot();
    fgic->SetClimbRateFpsIC(roc);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Gamma_vert_rad( double gamma) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Gamma_vert_rad: " << gamma );

    snap_shot();
    fgic->SetFlightPathAngleRadIC(gamma);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

//Earth
void FGJSBsim::set_Sea_level_radius(double slr) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Sea_level_radius: " << slr );

    snap_shot();
    fgic->SetSeaLevelRadiusFtIC(slr);
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Runway_altitude(double ralt) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Runway_altitude: " << ralt );

    snap_shot();
    _set_Runway_altitude( ralt );
    fgic->SetTerrainAltitudeFtIC( ralt );
    fdmex->RunIC(fgic); //loop JSBSim once
    copy_from_JSBsim(); //update the bus
    needTrim=true;
}

void FGJSBsim::set_Static_pressure(double p) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Static_pressure: " << p );

    snap_shot();
    Atmosphere->SetExPressure(p);
    if(Atmosphere->External() == true)
    needTrim=true;
}

void FGJSBsim::set_Static_temperature(double T) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Static_temperature: " << T );
    
    snap_shot();
    Atmosphere->SetExTemperature(T);
    if(Atmosphere->External() == true)
    needTrim=true;
}
 

void FGJSBsim::set_Density(double rho) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Density: " << rho );
    
    snap_shot();
    Atmosphere->SetExDensity(rho);
    if(Atmosphere->External() == true)
    needTrim=true;
}
  

void FGJSBsim::set_Velocities_Local_Airmass (double wnorth, 
                         double weast, 
                         double wdown ) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Velocities_Local_Airmass: " 
       << wnorth << ", " << weast << ", " << wdown );
    
    _set_Velocities_Local_Airmass( wnorth, weast, wdown );
    snap_shot();
    Atmosphere->SetWindNED(wnorth, weast, wdown );
    if(Atmosphere->External() == true)
        needTrim=true;
}     

