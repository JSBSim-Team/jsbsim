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
// $Id: JSBSim.cxx,v 1.164 2004/04/08 01:10:33 dpculp Exp $


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdio.h>	//	size_t
#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_STRING

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>

#include <FDM/flight.hxx>

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
#include <FDM/JSBSim/FGPropertyManager.h>
#include <FDM/JSBSim/FGEngine.h>
#include <FDM/JSBSim/FGPiston.h>
#include <FDM/JSBSim/FGSimTurbine.h>
#include <FDM/JSBSim/FGRocket.h>
#include <FDM/JSBSim/FGElectric.h>
#include <FDM/JSBSim/FGNozzle.h>
#include <FDM/JSBSim/FGPropeller.h>
#include <FDM/JSBSim/FGRotor.h>
#include "JSBSim.hxx"

static inline double
FMAX (double a, double b)
{
  return a > b ? a : b;
}


/******************************************************************************/

FGJSBsim::FGJSBsim( double dt )
  : FGInterface(dt)
{
    bool result;

                                // Set up the debugging level
                                // FIXME: this will not respond to
                                // runtime changes

                                // if flight is excluded, don't bother
    if ((logbuf::get_log_classes() & SG_FLIGHT) != 0) {

                                // do a rough-and-ready mapping to
                                // the levels documented in FGFDMExec.h
        switch (logbuf::get_log_priority()) {
        case SG_BULK:
            FGJSBBase::debug_lvl = 0x1f;
            break;
        case SG_DEBUG:
            FGJSBBase::debug_lvl = 0x0f;
        case SG_INFO:
            FGJSBBase::debug_lvl = 0x01;
            break;
        case SG_WARN:
        case SG_ALERT:
            FGJSBBase::debug_lvl = 0x00;
            break;
        }
    }

    fdmex = new FGFDMExec( (FGPropertyManager*)globals->get_props() );

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

    fgic=fdmex->GetIC();
    needTrim=true;

    SGPath aircraft_path( globals->get_fg_root() );
    aircraft_path.append( "Aircraft" );

    SGPath engine_path( globals->get_fg_root() );
    engine_path.append( "Engine" );
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

    if ( GroundReactions->GetNumGearUnits() <= 0 ) {
        SG_LOG( SG_FLIGHT, SG_ALERT, "num gear units = "
                << GroundReactions->GetNumGearUnits() );
        SG_LOG( SG_FLIGHT, SG_ALERT, "This is a very bad thing because with 0 gear units, the ground trimming");
        SG_LOG( SG_FLIGHT, SG_ALERT, "routine (coming up later in the code) will core dump.");
        SG_LOG( SG_FLIGHT, SG_ALERT, "Halting the sim now, and hoping a solution will present itself soon!");
        exit(-1);
    }

    init_gear();

    // Set initial fuel levels if provided.
    for (unsigned int i = 0; i < Propulsion->GetNumTanks(); i++) {
      SGPropertyNode * node = fgGetNode("/consumables/fuel/tank", i, true);
      if (node->getChild("level-gal_us", 0, false) != 0) {
        Propulsion->GetTank(i)->SetContents(node->getDoubleValue("level-gal_us") * 6.6);
      } else {
        node->setDoubleValue("level-lb", Propulsion->GetTank(i)->GetContents());
        node->setDoubleValue("level-gal_us", Propulsion->GetTank(i)->GetContents() / 6.6);
      }
    }

    fgSetDouble("/fdm/trim/pitch-trim", FCS->GetPitchTrimCmd());
    fgSetDouble("/fdm/trim/throttle",   FCS->GetThrottleCmd(0));
    fgSetDouble("/fdm/trim/aileron",    FCS->GetDaCmd());
    fgSetDouble("/fdm/trim/rudder",     FCS->GetDrCmd());

    startup_trim = fgGetNode("/sim/presets/trim", true);

    trimmed = fgGetNode("/fdm/trim/trimmed", true);
    trimmed->setBoolValue(false);

    pitch_trim = fgGetNode("/fdm/trim/pitch-trim", true );
    throttle_trim = fgGetNode("/fdm/trim/throttle", true );
    aileron_trim = fgGetNode("/fdm/trim/aileron", true );
    rudder_trim = fgGetNode("/fdm/trim/rudder", true );

    stall_warning = fgGetNode("/sim/alarms/stall-warning",true);
    stall_warning->setDoubleValue(0);


    flap_pos_pct=fgGetNode("/surface-positions/flap-pos-norm",true);
    elevator_pos_pct=fgGetNode("/surface-positions/elevator-pos-norm",true);
    left_aileron_pos_pct
        =fgGetNode("/surface-positions/left-aileron-pos-norm",true);
    right_aileron_pos_pct
        =fgGetNode("/surface-positions/right-aileron-pos-norm",true);
    rudder_pos_pct=fgGetNode("/surface-positions/rudder-pos-norm",true);
    speedbrake_pos_pct
        =fgGetNode("/surface-positions/speedbrake-pos-norm",true);
    spoilers_pos_pct=fgGetNode("/surface-positions/spoilers-pos-norm",true);

    elevator_pos_pct->setDoubleValue(0);
    left_aileron_pos_pct->setDoubleValue(0);
    right_aileron_pos_pct->setDoubleValue(0);
    rudder_pos_pct->setDoubleValue(0);
    flap_pos_pct->setDoubleValue(0);
    speedbrake_pos_pct->setDoubleValue(0);
    spoilers_pos_pct->setDoubleValue(0);

    temperature = fgGetNode("/environment/temperature-degc",true);
    pressure = fgGetNode("/environment/pressure-inhg",true);
    density = fgGetNode("/environment/density-slugft3",true);
    turbulence_gain = fgGetNode("/environment/turbulence/magnitude-norm",true);
    turbulence_rate = fgGetNode("/environment/turbulence/rate-hz",true);

    wind_from_north= fgGetNode("/environment/wind-from-north-fps",true);
    wind_from_east = fgGetNode("/environment/wind-from-east-fps" ,true);
    wind_from_down = fgGetNode("/environment/wind-from-down-fps" ,true);

    for (unsigned int i = 0; i < Propulsion->GetNumEngines(); i++) {
      SGPropertyNode * node = fgGetNode("engines/engine", i, true);
      Propulsion->GetThruster(i)->SetRPM(node->getDoubleValue("rpm") /
                     Propulsion->GetThruster(i)->GetGearRatio());
    }
}

/******************************************************************************/
FGJSBsim::~FGJSBsim(void)
{
  delete fdmex;
}

/******************************************************************************/

// Initialize the JSBsim flight model, dt is the time increment for
// each subsequent iteration through the EOM

void FGJSBsim::init()
{
    double tmp;

    SG_LOG( SG_FLIGHT, SG_INFO, "Starting and initializing JSBsim" );

    // Explicitly call the superclass's
    // init method first.

#ifdef FG_WEATHERCM
    Atmosphere->UseInternal();
#else
    if (fgGetBool("/environment/params/control-fdm-atmosphere")) {
      Atmosphere->UseExternal();
      Atmosphere->SetExTemperature(
                  9.0/5.0*(temperature->getDoubleValue()+273.15) );
      Atmosphere->SetExPressure(pressure->getDoubleValue()*70.726566);
      Atmosphere->SetExDensity(density->getDoubleValue());

      tmp = turbulence_gain->getDoubleValue();
      Atmosphere->SetTurbGain(tmp * tmp * 100.0);

      tmp = turbulence_rate->getDoubleValue();
      Atmosphere->SetTurbRate(tmp);

    } else {
      Atmosphere->UseInternal();
    }
#endif

    fgic->SetVnorthFpsIC( wind_from_north->getDoubleValue() );
    fgic->SetVeastFpsIC( wind_from_east->getDoubleValue() );
    fgic->SetVdownFpsIC( wind_from_down->getDoubleValue() );

    //Atmosphere->SetExTemperature(get_Static_temperature());
    //Atmosphere->SetExPressure(get_Static_pressure());
    //Atmosphere->SetExDensity(get_Density());
    SG_LOG(SG_FLIGHT,SG_INFO,"T,p,rho: " << fdmex->GetAtmosphere()->GetTemperature()
     << ", " << fdmex->GetAtmosphere()->GetPressure()
     << ", " << fdmex->GetAtmosphere()->GetDensity() );

    common_init();

    copy_to_JSBsim();
    fdmex->RunIC();     //loop JSBSim once w/o integrating
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
               << Auxiliary->GetMach() );
    break;
    case setvc:
    default:
        SG_LOG(SG_FLIGHT,SG_INFO, "  Indicated Airspeed: "
               << Auxiliary->GetVcalibratedKTS() << " knots" );
    break;
    }

    stall_warning->setDoubleValue(0);

    SG_LOG( SG_FLIGHT, SG_INFO, "  Bank Angle: "
            << Rotation->Getphi()*RADTODEG << " deg" );
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

void FGJSBsim::update( double dt )
{
    if (is_suspended())
      return;

    int multiloop = _calc_multiloop(dt);

    int i;

    // double save_alt = 0.0;

    copy_to_JSBsim();

    trimmed->setBoolValue(false);

    if ( needTrim ) {
      if ( startup_trim->getBoolValue() ) {
        SG_LOG(SG_FLIGHT, SG_INFO,
          "Ready to trim, terrain altitude is: "
            << cur_fdm_state->get_Runway_altitude() * SG_METER_TO_FEET );
        fgic->SetTerrainAltitudeFtIC( cur_fdm_state->get_ground_elev_ft() );
        do_trim();
      } else {
        fdmex->RunIC();  //apply any changes made through the set_ functions
      }
      needTrim = false;
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

    // translate JSBsim back to FG structure so that the
    // autopilot (and the rest of the sim can use the updated values
    copy_from_JSBsim();
}

/******************************************************************************/

// Convert from the FGInterface struct to the JSBsim generic_ struct

bool FGJSBsim::copy_to_JSBsim()
{
    double tmp;
    unsigned int i;

    // copy control positions into the JSBsim structure

    FCS->SetDaCmd( globals->get_controls()->get_aileron());
    FCS->SetRollTrimCmd( globals->get_controls()->get_aileron_trim() );
    FCS->SetDeCmd( globals->get_controls()->get_elevator());
    FCS->SetPitchTrimCmd( globals->get_controls()->get_elevator_trim() );
    FCS->SetDrCmd( -globals->get_controls()->get_rudder() );
    FCS->SetYawTrimCmd( -globals->get_controls()->get_rudder_trim() );
    FCS->SetDfCmd(  globals->get_controls()->get_flaps() );
    FCS->SetDsbCmd( globals->get_controls()->get_speedbrake() );
    FCS->SetDspCmd( globals->get_controls()->get_spoilers() );

        // Parking brake sets minimum braking
        // level for mains.
    double parking_brake = globals->get_controls()->get_brake_parking();
    FCS->SetLBrake(FMAX(globals->get_controls()->get_brake_left(), parking_brake));
    FCS->SetRBrake(FMAX(globals->get_controls()->get_brake_right(), parking_brake));
    FCS->SetCBrake( 0.0 );
    // FCS->SetCBrake( globals->get_controls()->get_brake(2) );

    FCS->SetGearCmd( globals->get_controls()->get_gear_down());
    for (i = 0; i < Propulsion->GetNumEngines(); i++) {
      SGPropertyNode * node = fgGetNode("engines/engine", i, true);

      FCS->SetThrottleCmd(i, globals->get_controls()->get_throttle(i));
      FCS->SetMixtureCmd(i, globals->get_controls()->get_mixture(i));
      FCS->SetPropAdvanceCmd(i, globals->get_controls()->get_prop_advance(i));

      switch (Propulsion->GetEngine(i)->GetType()) {
      case FGEngine::etPiston:
        { // FGPiston code block
        FGPiston* eng = (FGPiston*)Propulsion->GetEngine(i);
        eng->SetMagnetos( globals->get_controls()->get_magnetos(i) );
        break;
        } // end FGPiston code block
      case FGEngine::etSimTurbine:
        { // FGSimTurbine code block
        FGSimTurbine* eng = (FGSimTurbine*)Propulsion->GetEngine(i);
        eng->SetAugmentation( globals->get_controls()->get_augmentation(i) );
        eng->SetReverse( globals->get_controls()->get_reverser(i) );
        eng->SetInjection( globals->get_controls()->get_water_injection(i) );
        eng->SetCutoff( globals->get_controls()->get_cutoff(i) );
        eng->SetIgnition( globals->get_controls()->get_ignition(i) );
        break;
        } // end FGSimTurbine code block
      case FGEngine::etRocket:
        { // FGRocket code block
        FGRocket* eng = (FGRocket*)Propulsion->GetEngine(i);
        break;
        } // end FGRocket code block
      }

      { // FGEngine code block
      FGEngine* eng = Propulsion->GetEngine(i);

      eng->SetStarter( globals->get_controls()->get_starter(i) );
      eng->SetRunning( node->getBoolValue("running") );
      } // end FGEngine code block
    }

    _set_Runway_altitude( cur_fdm_state->get_Runway_altitude() );
    Position->SetSeaLevelRadius( get_Sea_level_radius() );
    Position->SetRunwayRadius( get_Runway_altitude()
                               + get_Sea_level_radius() );

    Atmosphere->SetExTemperature(
                  9.0/5.0*(temperature->getDoubleValue()+273.15) );
    Atmosphere->SetExPressure(pressure->getDoubleValue()*70.726566);
    Atmosphere->SetExDensity(density->getDoubleValue());

    tmp = turbulence_gain->getDoubleValue();
    Atmosphere->SetTurbGain(tmp * tmp * 100.0);

    tmp = turbulence_rate->getDoubleValue();
    Atmosphere->SetTurbRate(tmp);

    Atmosphere->SetWindNED( wind_from_north->getDoubleValue(),
                            wind_from_east->getDoubleValue(),
                            wind_from_down->getDoubleValue() );
//    SG_LOG(SG_FLIGHT,SG_INFO, "Wind NED: "
//                  << get_V_north_airmass() << ", "
//                  << get_V_east_airmass()  << ", "
//                  << get_V_down_airmass() );

    for (i = 0; i < Propulsion->GetNumTanks(); i++) {
      SGPropertyNode * node = fgGetNode("/consumables/fuel/tank", i, true);
      FGTank * tank = Propulsion->GetTank(i);
      tank->SetContents(node->getDoubleValue("level-gal_us") * 6.6);
//       tank->SetContents(node->getDoubleValue("level-lb"));
    }

    return true;
}

/******************************************************************************/

// Convert from the JSBsim generic_ struct to the FGInterface struct

bool FGJSBsim::copy_from_JSBsim()
{
    unsigned int i, j;
/*
    _set_Inertias( MassBalance->GetMass(),
                   MassBalance->GetIxx(),
                   MassBalance->GetIyy(),
                   MassBalance->GetIzz(),
                   MassBalance->GetIxz() );
*/
    _set_CG_Position( MassBalance->GetXYZcg(1),
                      MassBalance->GetXYZcg(2),
                      MassBalance->GetXYZcg(3) );

    _set_Accels_Body( Aircraft->GetBodyAccel(1),
                      Aircraft->GetBodyAccel(2),
                      Aircraft->GetBodyAccel(3) );

    _set_Accels_CG_Body_N ( Aircraft->GetNcg(1),
                            Aircraft->GetNcg(2),
                            Aircraft->GetNcg(3) );

    _set_Accels_Pilot_Body( Auxiliary->GetPilotAccel(1),
                            Auxiliary->GetPilotAccel(2),
                            Auxiliary->GetPilotAccel(3) );

    _set_Nlf( Aircraft->GetNlf() );

    // Velocities

    _set_Velocities_Local( Position->GetVn(),
                           Position->GetVe(),
                           Position->GetVd() );

    _set_Velocities_Wind_Body( Translation->GetUVW(1),
                               Translation->GetUVW(2),
                               Translation->GetUVW(3) );

    // Make the HUD work ...
    _set_Velocities_Ground( Position->GetVn(),
                            Position->GetVe(),
                            -Position->GetVd() );

    _set_V_rel_wind( Auxiliary->GetVt() );

    _set_V_equiv_kts( Auxiliary->GetVequivalentKTS() );

    _set_V_calibrated_kts( Auxiliary->GetVcalibratedKTS() );

    _set_V_ground_speed( Position->GetVground() );

    _set_Omega_Body( Rotation->GetPQR(1),
                     Rotation->GetPQR(2),
                     Rotation->GetPQR(3) );

    _set_Euler_Rates( Auxiliary->GetEulerRates(1),
                      Auxiliary->GetEulerRates(2),
                      Auxiliary->GetEulerRates(3) );

    _set_Geocentric_Rates(Position->GetLatitudeDot(),
                          Position->GetLongitudeDot(),
                          Position->Gethdot() );

    _set_Mach_number( Auxiliary->GetMach() );

    // Positions
    _updateGeocentricPosition( Position->GetLatitude(),
             Position->GetLongitude(),
             Position->Geth() );

    // Positions of Visual Reference Point
/*
    _updateGeocentricPosition( Position->GetLatitudeVRP(),
             Position->GetLongitudeVRP(),
             Position->GethVRP() );
*/
    _set_Altitude_AGL( Position->GetDistanceAGL() );

    _set_Euler_Angles( Rotation->Getphi(),
                       Rotation->Gettht(),
                       Rotation->Getpsi() );

    _set_Alpha( Auxiliary->Getalpha() );
    _set_Beta( Auxiliary->Getbeta() );


    _set_Gamma_vert_rad( Position->GetGamma() );

    _set_Earth_position_angle( Auxiliary->GetEarthPositionAngle() );

    _set_Climb_Rate( Position->Gethdot() );

    const FGMatrix33& Tl2b = Rotation->GetTl2b();
    for ( i = 1; i <= 3; i++ ) {
        for ( j = 1; j <= 3; j++ ) {
            _set_T_Local_to_Body( i, j, Tl2b(i,j) );
        }
    }

    // Copy the engine values from JSBSim.
    for ( i=0; i < Propulsion->GetNumEngines(); i++ ) {
      SGPropertyNode * node = fgGetNode("engines/engine", i, true);
      char buf[30];
      sprintf(buf, "engines/engine[%d]/thruster", i);
      SGPropertyNode * tnode = fgGetNode(buf, true);
      FGThruster * thruster = Propulsion->GetThruster(i);

      switch (Propulsion->GetEngine(i)->GetType()) {
      case FGEngine::etPiston:
        { // FGPiston code block
        FGPiston* eng = (FGPiston*)Propulsion->GetEngine(i);
        node->setDoubleValue("egt-degf", eng->getExhaustGasTemp_degF());
        node->setDoubleValue("oil-temperature-degf", eng->getOilTemp_degF());
        node->setDoubleValue("oil-pressure-psi", eng->getOilPressure_psi());
        node->setDoubleValue("mp-osi", eng->getManifoldPressure_inHg());
        node->setDoubleValue("cht-degf", eng->getCylinderHeadTemp_degF());
        node->setDoubleValue("rpm", eng->getRPM());
        } // end FGPiston code block
        break;
      case FGEngine::etRocket:
        { // FGRocket code block
        FGRocket* eng = (FGRocket*)Propulsion->GetEngine(i);
        } // end FGRocket code block
        break;
      case FGEngine::etSimTurbine:
        { // FGSimTurbine code block
        FGSimTurbine* eng = (FGSimTurbine*)Propulsion->GetEngine(i);
        node->setDoubleValue("n1", eng->GetN1());
        node->setDoubleValue("n2", eng->GetN2());
        node->setDoubleValue("egt_degf", 32 + eng->GetEGT()*9/5);
        node->setBoolValue("augmentation", eng->GetAugmentation());
        node->setBoolValue("water-injection", eng->GetInjection());
        node->setBoolValue("ignition", eng->GetIgnition());
        node->setDoubleValue("nozzle-pos-norm", eng->GetNozzle());
        node->setDoubleValue("inlet-pos-norm", eng->GetInlet());
        node->setDoubleValue("oil-pressure-psi", eng->getOilPressure_psi());
        node->setBoolValue("reversed", eng->GetReversed());
        node->setBoolValue("cutoff", eng->GetCutoff());
        globals->get_controls()->set_reverser(i, eng->GetReversed() );
        globals->get_controls()->set_cutoff(i, eng->GetCutoff() );
        globals->get_controls()->set_water_injection(i, eng->GetInjection() );
        globals->get_controls()->set_augmentation(i, eng->GetAugmentation() );
        } // end FGSimTurbine code block
        break;
      case FGEngine::etElectric:
        { // FGElectric code block
        FGElectric* eng = (FGElectric*)Propulsion->GetEngine(i);
        node->setDoubleValue("rpm", eng->getRPM());
        } // end FGElectric code block
        break;
      }

      { // FGEngine code block
      FGEngine* eng = Propulsion->GetEngine(i);
      node->setDoubleValue("fuel-flow-gph", eng->getFuelFlow_gph());
      node->setDoubleValue("thrust_lb", thruster->GetThrust());
      node->setDoubleValue("fuel-flow_pph", eng->getFuelFlow_pph());
      node->setBoolValue("running", eng->GetRunning());
      node->setBoolValue("starter", eng->GetStarter());
      node->setBoolValue("cranking", eng->GetCranking());
      globals->get_controls()->set_starter(i, eng->GetStarter() );
      } // end FGEngine code block

      switch (thruster->GetType()) {
      case FGThruster::ttNozzle:
        { // FGNozzle code block
        FGNozzle* noz = (FGNozzle*)thruster;
        } // end FGNozzle code block
        break;
      case FGThruster::ttPropeller:
        { // FGPropeller code block
        FGPropeller* prop = (FGPropeller*)thruster;
        tnode->setDoubleValue("rpm", thruster->GetRPM());
        tnode->setDoubleValue("pitch", prop->GetPitch());
        tnode->setDoubleValue("torque", prop->GetTorque());
        } // end FGPropeller code block
        break;
      case FGThruster::ttRotor:
        { // FGRotor code block
        FGRotor* rotor = (FGRotor*)thruster;
        } // end FGRotor code block
        break;
      case FGThruster::ttDirect:
        { // Direct code block
        } // end Direct code block
        break;
      }

    }

    static const SGPropertyNode *fuel_freeze = fgGetNode("/sim/freeze/fuel");

    // Copy the fuel levels from JSBSim if fuel
    // freeze not enabled.
    if ( ! fuel_freeze->getBoolValue() ) {
      for (i = 0; i < Propulsion->GetNumTanks(); i++) {
        SGPropertyNode * node = fgGetNode("/consumables/fuel/tank", i, true);
        double contents = Propulsion->GetTank(i)->GetContents();
        node->setDoubleValue("level-gal_us", contents/6.6);
        node->setDoubleValue("level-lb", contents);
        // node->setDoubleValue("temperature_degC",
      }
    }

    update_gear();

    stall_warning->setDoubleValue( Aerodynamics->GetStallWarn() );

    elevator_pos_pct->setDoubleValue( FCS->GetDePos(ofNorm) );
    left_aileron_pos_pct->setDoubleValue( FCS->GetDaLPos(ofNorm) );
    right_aileron_pos_pct->setDoubleValue( FCS->GetDaRPos(ofNorm) );
    rudder_pos_pct->setDoubleValue( -1*FCS->GetDrPos(ofNorm) );
    flap_pos_pct->setDoubleValue( FCS->GetDfPos(ofNorm) );
    speedbrake_pos_pct->setDoubleValue( FCS->GetDsbPos(ofNorm) );
    spoilers_pos_pct->setDoubleValue( FCS->GetDspPos(ofNorm) );

    return true;
}


bool FGJSBsim::ToggleDataLogging(void)
{
    return fdmex->GetOutput()->Toggle();
}


bool FGJSBsim::ToggleDataLogging(bool state)
{
    if (state) {
      fdmex->GetOutput()->Enable();
      return true;
    } else {
      fdmex->GetOutput()->Disable();
      return false;
    }
}


//Positions
void FGJSBsim::set_Latitude(double lat)
{
    static const SGPropertyNode *altitude = fgGetNode("/position/altitude-ft");
    double alt;
    double sea_level_radius_meters, lat_geoc;

    // In case we're not trimming
    FGInterface::set_Latitude(lat);

    if ( altitude->getDoubleValue() > -9990 ) {
      alt = altitude->getDoubleValue();
    } else {
      alt = 0.0;
    }

    update_ic();
    SG_LOG(SG_FLIGHT,SG_INFO,"FGJSBsim::set_Latitude: " << lat );
    SG_LOG(SG_FLIGHT,SG_INFO," cur alt (ft) =  " << alt );

    sgGeodToGeoc( lat, alt * SG_FEET_TO_METER,
                      &sea_level_radius_meters, &lat_geoc );
    _set_Sea_level_radius( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetSeaLevelRadiusFtIC( sea_level_radius_meters * SG_METER_TO_FEET  );
    _set_Runway_altitude( cur_fdm_state->get_Runway_altitude() );
    fgic->SetTerrainAltitudeFtIC( cur_fdm_state->get_ground_elev_ft() );
    fgic->SetLatitudeRadIC( lat_geoc );
    needTrim=true;
}


void FGJSBsim::set_Longitude(double lon)
{
    SG_LOG(SG_FLIGHT,SG_INFO,"FGJSBsim::set_Longitude: " << lon );

    // In case we're not trimming
    FGInterface::set_Longitude(lon);

    update_ic();
    fgic->SetLongitudeRadIC( lon );
    _set_Runway_altitude( cur_fdm_state->get_Runway_altitude() );
    fgic->SetTerrainAltitudeFtIC( cur_fdm_state->get_ground_elev_ft() );
    needTrim=true;
}

void FGJSBsim::set_Altitude(double alt)
{
    static const SGPropertyNode *latitude = fgGetNode("/position/latitude-deg");

    double sea_level_radius_meters,lat_geoc;

    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Altitude: " << alt );
    SG_LOG(SG_FLIGHT,SG_INFO, "  lat (deg) = " << latitude->getDoubleValue() );

    // In case we're not trimming
    FGInterface::set_Altitude(alt);

    update_ic();
    sgGeodToGeoc( latitude->getDoubleValue() * SGD_DEGREES_TO_RADIANS, alt,
                  &sea_level_radius_meters, &lat_geoc);
    _set_Sea_level_radius( sea_level_radius_meters * SG_METER_TO_FEET  );
    fgic->SetSeaLevelRadiusFtIC( sea_level_radius_meters * SG_METER_TO_FEET );
    _set_Runway_altitude( cur_fdm_state->get_Runway_altitude() );
    fgic->SetTerrainAltitudeFtIC( cur_fdm_state->get_ground_elev_ft() );
    SG_LOG(SG_FLIGHT, SG_INFO,
          "Terrain altitude: " << cur_fdm_state->get_Runway_altitude() * SG_METER_TO_FEET );
    fgic->SetLatitudeRadIC( lat_geoc );
    fgic->SetAltitudeFtIC(alt);
    needTrim=true;
}

void FGJSBsim::set_V_calibrated_kts(double vc)
{
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_V_calibrated_kts: " <<  vc );

    // In case we're not trimming
    FGInterface::set_V_calibrated_kts(vc);

    update_ic();
    fgic->SetVcalibratedKtsIC(vc);
    needTrim=true;
}

void FGJSBsim::set_Mach_number(double mach)
{
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Mach_number: " <<  mach );

    // In case we're not trimming
    FGInterface::set_Mach_number(mach);

    update_ic();
    fgic->SetMachIC(mach);
    needTrim=true;
}

void FGJSBsim::set_Velocities_Local( double north, double east, double down )
{
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Velocities_Local: "
       << north << ", " <<  east << ", " << down );

    // In case we're not trimming
    FGInterface::set_Velocities_Local(north, east, down);

    update_ic();
    fgic->SetVnorthFpsIC(north);
    fgic->SetVeastFpsIC(east);
    fgic->SetVdownFpsIC(down);
    needTrim=true;
}

void FGJSBsim::set_Velocities_Wind_Body( double u, double v, double w)
{
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Velocities_Wind_Body: "
       << u << ", " <<  v << ", " <<  w );

    // In case we're not trimming
    FGInterface::set_Velocities_Wind_Body(u, v, w);

    update_ic();
    fgic->SetUBodyFpsIC(u);
    fgic->SetVBodyFpsIC(v);
    fgic->SetWBodyFpsIC(w);
    needTrim=true;
}

//Euler angles
void FGJSBsim::set_Euler_Angles( double phi, double theta, double psi )
{
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Euler_Angles: "
       << phi << ", " << theta << ", " << psi );

    // In case we're not trimming
    FGInterface::set_Euler_Angles(phi, theta, psi);

    update_ic();
    fgic->SetPitchAngleRadIC(theta);
    fgic->SetRollAngleRadIC(phi);
    fgic->SetTrueHeadingRadIC(psi);
    needTrim=true;
}

//Flight Path
void FGJSBsim::set_Climb_Rate( double roc)
{
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Climb_Rate: " << roc );

    // In case we're not trimming
    FGInterface::set_Climb_Rate(roc);

    update_ic();
    //since both climb rate and flight path angle are set in the FG
    //startup sequence, something is needed to keep one from cancelling
    //out the other.
    if( !(fabs(roc) > 1 && fabs(fgic->GetFlightPathAngleRadIC()) < 0.01) ) {
      fgic->SetClimbRateFpsIC(roc);
    }
    needTrim=true;
}

void FGJSBsim::set_Gamma_vert_rad( double gamma)
{
    SG_LOG(SG_FLIGHT,SG_INFO, "FGJSBsim::set_Gamma_vert_rad: " << gamma );

    update_ic();
    if( !(fabs(gamma) < 0.01 && fabs(fgic->GetClimbRateFpsIC()) > 1) ) {
      fgic->SetFlightPathAngleRadIC(gamma);
    }
    needTrim=true;
}

void FGJSBsim::init_gear(void )
{
    FGGroundReactions* gr=fdmex->GetGroundReactions();
    int Ngear=GroundReactions->GetNumGearUnits();
    for (int i=0;i<Ngear;i++) {
      SGPropertyNode * node = fgGetNode("gear/gear", i, true);
      node->setDoubleValue("xoffset-in",
         gr->GetGearUnit(i)->GetBodyLocation()(1));
      node->setDoubleValue("yoffset-in",
         gr->GetGearUnit(i)->GetBodyLocation()(2));
      node->setDoubleValue("zoffset-in",
         gr->GetGearUnit(i)->GetBodyLocation()(3));
      node->setBoolValue("wow", gr->GetGearUnit(i)->GetWOW());
      node->setBoolValue("has-brake", gr->GetGearUnit(i)->GetBrakeGroup() > 0);
      node->setDoubleValue("position-norm", FCS->GetGearPos());
      node->setDoubleValue("tire-pressure-norm", gr->GetGearUnit(i)->GetTirePressure());
    }
}

void FGJSBsim::update_gear(void)
{
    FGGroundReactions* gr=fdmex->GetGroundReactions();
    int Ngear=GroundReactions->GetNumGearUnits();
    for (int i=0;i<Ngear;i++) {
      SGPropertyNode * node = fgGetNode("gear/gear", i, true);
      node->getChild("wow", 0, true)->setBoolValue(gr->GetGearUnit(i)->GetWOW());
      node->getChild("position-norm", 0, true)->setDoubleValue(FCS->GetGearPos());
      gr->GetGearUnit(i)->SetTirePressure(node->getDoubleValue("tire-pressure-norm"));
    }
}

void FGJSBsim::do_trim(void)
{
  FGTrim *fgtrim;

  if ( fgGetBool("/sim/presets/onground") )
  {
    fgic->SetVcalibratedKtsIC(0.0);
    fgtrim = new FGTrim(fdmex,tGround);
  } else {
    fgtrim = new FGTrim(fdmex,tLongitudinal);
  }

  if ( !fgtrim->DoTrim() ) {
    fgtrim->Report();
    fgtrim->TrimStats();
  } else {
    trimmed->setBoolValue(true);
  }
  if (FGJSBBase::debug_lvl > 0)
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

void FGJSBsim::update_ic(void)
{
   if ( !needTrim ) {
     fgic->SetLatitudeRadIC(get_Lat_geocentric() );
     fgic->SetLongitudeRadIC( get_Longitude() );
     fgic->SetAltitudeFtIC( get_Altitude() );
     fgic->SetVcalibratedKtsIC( get_V_calibrated_kts() );
     fgic->SetPitchAngleRadIC( get_Theta() );
     fgic->SetRollAngleRadIC( get_Phi() );
     fgic->SetTrueHeadingRadIC( get_Psi() );
     fgic->SetClimbRateFpsIC( get_Climb_Rate() );
   }
}

