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
// $Id: JSBSim.cxx,v 1.92 2001/12/17 02:05:53 dmegginson Exp $


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
#include <FDM/JSBSim/FGInitialCondition.h>
#include <FDM/JSBSim/FGTrim.h>
#include <FDM/JSBSim/FGAtmosphere.h>
#include <FDM/JSBSim/FGMassBalance.h>
#include <FDM/JSBSim/FGAerodynamics.h>
#include <FDM/JSBSim/FGLGear.h>
#include "JSBSim.hxx"

/******************************************************************************/

FGJSBsim::FGJSBsim( double dt ) 
  : FGInterface(dt)
{
    bool result;
   
    fdmex = new FGFDMExec;
    
    State           = fdmex->GetState();
    Atmosphere      = fdmex->GetAtmosphere();
    FCS             = fdmex->GetFCS();
    MassBalance     = fdmex->GetMassBalance();
    Propulsion      = fdmex->GetPropulsion();
    Aircraft        = fdmex->GetAircraft();
    Translation     = fdmex->GetTranslation();
    Rotation        = fdmex->GetRotation();
    Position        = fdmex->GetPosition();
    Auxiliary       = fdmex->GetAuxiliary();
    Aerodynamics    = fdmex->GetAerodynamics();
    GroundReactions = fdmex->GetGroundReactions();  
  
    
    Atmosphere->UseInternal();
    
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
                               fgGetString("/sim/aero") );
    
    if (result) {
      SG_LOG( SG_FLIGHT, SG_INFO, "  loaded aero.");
    } else {
      SG_LOG( SG_FLIGHT, SG_INFO,
              "  aero does not exist (you may have mis-typed the name).");
      throw(-1);
    }

    SG_LOG( SG_FLIGHT, SG_INFO, "" );
    SG_LOG( SG_FLIGHT, SG_INFO, "" );
    SG_LOG( SG_FLIGHT, SG_INFO, "After loading aero definition file ..." );

    int Neng = Propulsion->GetNumEngines();
    SG_LOG( SG_FLIGHT, SG_INFO, "num engines = " << Neng );
    for(int i=0;i<Neng;i++) {
        add_engine( FGEngInterface() );
    }  
    
    if ( GroundReactions->GetNumGearUnits() <= 0 ) {
        SG_LOG( SG_FLIGHT, SG_ALERT, "num gear units = "
                << GroundReactions->GetNumGearUnits() );
        SG_LOG( SG_FLIGHT, SG_ALERT, "This is a very bad thing because with 0 gear units, the ground trimming");
         SG_LOG( SG_FLIGHT, SG_ALERT, "routine (coming up later in the code) will core dump.");
         SG_LOG( SG_FLIGHT, SG_ALERT, "Halting the sim now, and hoping a solution will present itself soon!");
         exit(-1);
    }
        
    
    init_gear();
    
    fgSetDouble("/fdm/trim/pitch-trim", FCS->GetPitchTrimCmd());
    fgSetDouble("/fdm/trim/throttle",   FCS->GetThrottleCmd(0));
    fgSetDouble("/fdm/trim/aileron",    FCS->GetDaCmd());
    fgSetDouble("/fdm/trim/rudder",     FCS->GetDrCmd());

    startup_trim = fgGetNode("/sim/startup/trim", true);

    trimmed = fgGetNode("/fdm/trim/trimmed", true);
    trimmed->setBoolValue(false);

    pitch_trim = fgGetNode("/fdm/trim/pitch-trim", true );
    throttle_trim = fgGetNode("/fdm/trim/throttle", true );
    aileron_trim = fgGetNode("/fdm/trim/aileron", true );
    rudder_trim = fgGetNode("/fdm/trim/rudder", true );
    
    
    stall_warning = fgGetNode("/sim/aero/alarms/stall-warning",true);
    stall_warning->setDoubleValue(0);
}

/******************************************************************************/
FGJSBsim::~FGJSBsim(void) {
    if (fdmex != NULL) {
        delete fdmex; fdmex=NULL;
        delete fgic; fgic=NULL;
    }  
}

/******************************************************************************/

// Initialize the JSBsim flight model, dt is the time increment for
// each subsequent iteration through the EOM

void FGJSBsim::init() {
    
    SG_LOG( SG_FLIGHT, SG_INFO, "Starting and initializing JSBsim" );
   
    // Explicitly call the superclass's
    // init method first.
    common_init();

    fdmex->GetState()->Initialize(fgic);
    fdmex->RunIC(fgic); //loop JSBSim once w/o integrating
    // fdmex->Run();       //loop JSBSim once
    copy_from_JSBsim(); //update the bus

    SG_LOG( SG_FLIGHT, SG_INFO, "  Initialized JSBSim with:" );

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
    
    stall_warning->setDoubleValue(0);
    
    SG_LOG( SG_FLIGHT, SG_INFO, "  Bank Angle: "
            <<  Rotation->Getphi()*RADTODEG << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Pitch Angle: "
            << Rotation->Gettht()*RADTODEG << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  True Heading: "
            << Rotation->Getpsi()*RADTODEG << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Latitude: "
            << Position->GetLatitude() << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Longitude: "
            << Position->GetLongitude() << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Altitude: "
        << Position->Geth() << " feet" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  loaded initial conditions" );

    SG_LOG( SG_FLIGHT, SG_INFO, "  set dt" );

    SG_LOG( SG_FLIGHT, SG_INFO, "Finished initializing JSBSim" );
    
    SG_LOG( SG_FLIGHT, SG_INFO, "FGControls::get_gear_down()= " << 
                                  globals->get_controls()->get_gear_down() );
    

   
}

/******************************************************************************/

// Run an iteration of the EOM (equations of motion)

bool FGJSBsim::update( int multiloop ) {

    int i;

    double save_alt = 0.0;

    copy_to_JSBsim();

    trimmed->setBoolValue(false);

    if ( needTrim ) {
      if ( startup_trim->getBoolValue() ) {
        do_trim();
      } else {
        fdmex->RunIC(fgic);  //apply any changes made through the set_ functions
      }
      needTrim = false;  
    }    
    
    for( i=0; i<get_num_engines(); i++ ) {
      FGEngInterface * e = get_engine(i);
      FGEngine * eng = Propulsion->GetEngine(i);
      FGThruster * thrust = Propulsion->GetThruster(i);
      eng->SetMagnetos( globals->get_controls()->get_magnetos(i) );
      eng->SetStarter( globals->get_controls()->get_starter(i) );
      e->set_Throttle( globals->get_controls()->get_throttle(i) );
    }


    for ( i=0; i < multiloop; i++ ) {
        fdmex->Run();
    }

    FGJSBBase::Message* msg;
    while (fdmex->ReadMessage()) {
      msg = fdmex->ProcessMessage();
      switch (msg->type) {
      case FGJSBBase::Message::eText:
        SG_LOG( SG_FLIGHT, SG_INFO, msg->messageId << ": " << msg->text );
        break;
      case FGJSBBase::Message::eBool:
        SG_LOG( SG_FLIGHT, SG_INFO, msg->messageId << ": " << msg->text << " " << msg->bVal );
        break;
      case FGJSBBase::Message::eInteger:
        SG_LOG( SG_FLIGHT, SG_INFO, msg->messageId << ": " << msg->text << " " << msg->iVal );
        break;
      case FGJSBBase::Message::eDouble:
        SG_LOG( SG_FLIGHT, SG_INFO, msg->messageId << ": " << msg->text << " " << msg->dVal );
        break;
      default:
        SG_LOG( SG_FLIGHT, SG_INFO, "Unrecognized message type." );
        break;
      }
    }

    for( i=0; i<get_num_engines(); i++ ) {
      FGEngInterface * e = get_engine(i);
      FGEngine * eng = Propulsion->GetEngine(i);
      FGThruster * thrust = Propulsion->GetThruster(i);
      e->set_Manifold_Pressure( eng->getManifoldPressure_inHg() );
      e->set_RPM( thrust->GetRPM() );
      e->set_EGT( eng->getExhaustGasTemp_degF() );
      e->set_CHT( eng->getCylinderHeadTemp_degF() );
      e->set_Oil_Temp( eng->getOilTemp_degF() );
      e->set_Running_Flag( eng->GetRunning() );
      e->set_Cranking_Flag( eng->GetCranking() );
    }

    
    update_gear();
    
    stall_warning->setDoubleValue( Aircraft->GetStallWarn() );
    
    // translate JSBsim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated values
    copy_from_JSBsim();
    return true;
}

/******************************************************************************/

// Convert from the FGInterface struct to the JSBsim generic_ struct

bool FGJSBsim::copy_to_JSBsim() {
    // copy control positions into the JSBsim structure

    FCS->SetDaCmd( globals->get_controls()->get_aileron());
    FCS->SetRollTrimCmd(globals->get_controls()->get_aileron_trim());
    FCS->SetDeCmd( globals->get_controls()->get_elevator());
    FCS->SetPitchTrimCmd(globals->get_controls()->get_elevator_trim());
    FCS->SetDrCmd( -globals->get_controls()->get_rudder());
    FCS->SetYawTrimCmd(globals->get_controls()->get_rudder_trim());
    FCS->SetDfCmd(  globals->get_controls()->get_flaps() );
    FCS->SetDsbCmd( 0.0 ); //speedbrakes
    FCS->SetDspCmd( 0.0 ); //spoilers
    FCS->SetLBrake( globals->get_controls()->get_brake( 0 ) );
    FCS->SetRBrake( globals->get_controls()->get_brake( 1 ) );
    FCS->SetCBrake( globals->get_controls()->get_brake( 2 ) );
    FCS->SetGearCmd( globals->get_controls()->get_gear_down());
    for (int i = 0; i < get_num_engines(); i++) {
      FCS->SetThrottleCmd(i, globals->get_controls()->get_throttle(i));
      FCS->SetMixtureCmd(i, globals->get_controls()->get_mixture(i));
      FCS->SetPropAdvanceCmd(i, globals->get_controls()->get_prop_advance(i));
    }

    Position->SetSeaLevelRadius( get_Sea_level_radius() );
    Position->SetRunwayRadius( scenery.get_cur_elev()*SG_METER_TO_FEET
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

    _set_Accels_Body( Aircraft->GetBodyAccel()(1),
                      Aircraft->GetBodyAccel()(2),
                      Aircraft->GetBodyAccel()(3) );

    //_set_Accels_CG_Body( Aircraft->GetBodyAccel()(1),
    //                     Aircraft->GetBodyAccel()(2),
    //                     Aircraft->GetBodyAccel()(3) );
    //
    _set_Accels_CG_Body_N ( Aircraft->GetNcg()(1),
                            Aircraft->GetNcg()(2),
                            Aircraft->GetNcg()(3) );
    
    _set_Accels_Pilot_Body( Auxiliary->GetPilotAccel()(1),
                            Auxiliary->GetPilotAccel()(2),
                            Auxiliary->GetPilotAccel()(3) );

   // _set_Accels_Pilot_Body_N( Auxiliary->GetPilotAccel()(1)/32.1739,
   //                           Auxiliary->GetNpilot(2)/32.1739,
   //                           Auxiliary->GetNpilot(3)/32.1739 );

    _set_Nlf( Aircraft->GetNlf() );

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
    _updateGeocentricPosition( Position->GetLatitude(),
			       Position->GetLongitude(),
			       Position->Geth() );

    _set_Altitude_AGL( Position->GetDistanceAGL() );

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
    static const SGPropertyNode *altitude = fgGetNode("/position/altitude-ft");
    double alt;
    if ( altitude->getDoubleValue() > -9990 ) {
      alt = altitude->getDoubleValue();
    } else {
      alt = 0.0;
    }

    double sea_level_radius_meters, lat_geoc;

    SG_LOG(SG_FLIGHT,SG_INFO,"FGJSBsim::set_Latitude: " << lat );
    SG_LOG(SG_FLIGHT,SG_INFO," cur alt (ft) =  " << alt );

    sgGeodToGeoc( lat, alt * SG_FEET_TO_METER, &sea_level_radius_meters, &lat_geoc );
    
    _set_Sea_level_radius( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetSeaLevelRadiusFtIC( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetLatitudeRadIC( lat_geoc );
    needTrim=true;
}

void FGJSBsim::set_Longitude(double lon) {

    SG_LOG(SG_FLIGHT,SG_INFO,"FGJSBsim::set_Longitude: " << lon );

    fgic->SetLongitudeRadIC( lon );
    needTrim=true;
}

void FGJSBsim::set_Altitude(double alt) {
    static const SGPropertyNode *latitude = fgGetNode("/position/latitude-deg");

    double sea_level_radius_meters,lat_geoc;

    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Altitude: " << alt );
    SG_LOG(SG_FLIGHT,SG_INFO, "  lat (deg) = " << latitude->getDoubleValue() );

    sgGeodToGeoc( latitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS, alt,
      &sea_level_radius_meters, &lat_geoc);
    _set_Sea_level_radius( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetSeaLevelRadiusFtIC( sea_level_radius_meters * SG_METER_TO_FEET );
    fgic->SetLatitudeRadIC( lat_geoc );
    fgic->SetAltitudeFtIC(alt);
    needTrim=true;
}

void FGJSBsim::set_V_calibrated_kts(double vc) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_V_calibrated_kts: " <<  vc );

    fgic->SetVcalibratedKtsIC(vc);
    needTrim=true;
}

void FGJSBsim::set_Mach_number(double mach) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Mach_number: " <<  mach );

    fgic->SetMachIC(mach);
    needTrim=true;
}

void FGJSBsim::set_Velocities_Local( double north, double east, double down ){
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Velocities_Local: "
       << north << ", " <<  east << ", " << down );

    fgic->SetVnorthFpsIC(north);
    fgic->SetVeastFpsIC(east);
    fgic->SetVdownFpsIC(down);
    needTrim=true;
}

void FGJSBsim::set_Velocities_Wind_Body( double u, double v, double w){
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Velocities_Wind_Body: "
       << u << ", " <<  v << ", " <<  w );

    fgic->SetUBodyFpsIC(u);
    fgic->SetVBodyFpsIC(v);
    fgic->SetWBodyFpsIC(w);
    needTrim=true;
}

//Euler angles
void FGJSBsim::set_Euler_Angles( double phi, double theta, double psi ) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Euler_Angles: "
       << phi << ", " << theta << ", " << psi );

    fgic->SetPitchAngleRadIC(theta);
    fgic->SetRollAngleRadIC(phi);
    fgic->SetTrueHeadingRadIC(psi);
    needTrim=true;
}

//Flight Path
void FGJSBsim::set_Climb_Rate( double roc) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Climb_Rate: " << roc );

    fgic->SetClimbRateFpsIC(roc);
    needTrim=true;
}

void FGJSBsim::set_Gamma_vert_rad( double gamma) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Gamma_vert_rad: " << gamma );

    fgic->SetFlightPathAngleRadIC(gamma);
    needTrim=true;
}

//Earth
void FGJSBsim::set_Sea_level_radius(double slr) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Sea_level_radius: " << slr );

    fgic->SetSeaLevelRadiusFtIC(slr);
    needTrim=true;
}

void FGJSBsim::set_Runway_altitude(double ralt) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Runway_altitude: " << ralt );

    _set_Runway_altitude( ralt );
    fgic->SetTerrainAltitudeFtIC( ralt );
    needTrim=true;
}

void FGJSBsim::set_Static_pressure(double p) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Static_pressure: " << p );

    Atmosphere->SetExPressure(p);
    if(Atmosphere->External() == true)
    needTrim=true;
}

void FGJSBsim::set_Static_temperature(double T) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Static_temperature: " << T );
    
    Atmosphere->SetExTemperature(T);
    if(Atmosphere->External() == true)
    needTrim=true;
}
 

void FGJSBsim::set_Density(double rho) {
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Density: " << rho );
    
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
    Atmosphere->SetWindNED(wnorth, weast, wdown );
    if(Atmosphere->External() == true)
        needTrim=true;
}     

void FGJSBsim::init_gear(void ) {
    
    FGGearInterface *gear;
    FGGroundReactions* gr=fdmex->GetGroundReactions();
    int Ngear=GroundReactions->GetNumGearUnits();
    for (int i=0;i<Ngear;i++) {
      add_gear_unit( FGGearInterface() );
      gear=get_gear_unit(i);
      gear->SetX( gr->GetGearUnit(i)->GetBodyLocation()(1) );
      gear->SetY( gr->GetGearUnit(i)->GetBodyLocation()(2) );
      gear->SetZ( gr->GetGearUnit(i)->GetBodyLocation()(3) );
      gear->SetWoW( gr->GetGearUnit(i)->GetWOW() );
      if ( gr->GetGearUnit(i)->GetBrakeGroup() > 0 ) {
        gear->SetBrake(true);
      }
      if ( gr->GetGearUnit(i)->GetRetractable() ) {
        gear->SetPosition( FCS->GetGearPos() );
      } else {
        gear->SetPosition( 1.0 );
      }  
    }  
}

void FGJSBsim::update_gear(void) {
    
    FGGearInterface* gear;
    FGGroundReactions* gr=fdmex->GetGroundReactions();
    int Ngear=GroundReactions->GetNumGearUnits();
    for (int i=0;i<Ngear;i++) {
      gear=get_gear_unit(i);
      gear->SetWoW( gr->GetGearUnit(i)->GetWOW() );
      if ( gr->GetGearUnit(i)->GetRetractable() ) {
        gear->SetPosition( FCS->GetGearPos() );
      }   
    }  
}

void FGJSBsim::do_trim(void) {

        FGTrim *fgtrim;
        if(fgic->GetVcalibratedKtsIC() < 10 ) {
            fgic->SetVcalibratedKtsIC(0.0);
            fgtrim=new FGTrim(fdmex,fgic,tGround);
        } else {
            fgtrim=new FGTrim(fdmex,fgic,tLongitudinal);
        }
        if( !fgtrim->DoTrim() ) {
            fgtrim->Report();
            fgtrim->TrimStats();
        } else {
            trimmed->setBoolValue(true);
        }
        State->ReportState();
        delete fgtrim;
        pitch_trim->setDoubleValue( FCS->GetPitchTrimCmd() );
        throttle_trim->setDoubleValue( FCS->GetThrottleCmd(0) );
        aileron_trim->setDoubleValue( FCS->GetDaCmd() );
        rudder_trim->setDoubleValue( FCS->GetDrCmd() );

        globals->get_controls()->set_elevator_trim(FCS->GetPitchTrimCmd());
        globals->get_controls()->set_elevator(FCS->GetDeCmd());
        globals->get_controls()->set_throttle(FGControls::ALL_ENGINES,
                                              FCS->GetThrottleCmd(0));

        globals->get_controls()->set_aileron(FCS->GetDaCmd());
        globals->get_controls()->set_rudder( FCS->GetDrCmd());
    
        SG_LOG( SG_FLIGHT, SG_INFO, "  Trim complete" );
}          
