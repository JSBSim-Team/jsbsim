// JSBsim.cxx -- interface to the JSBsim flight model
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id: JSBSim.cxx,v 1.62 2010/07/14 05:50:40 ehofman Exp $


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdio.h>    //    size_t
#include <string>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/commands.hxx>

#include <FDM/flight.hxx>

#include <Aircraft/controls.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "JSBSim.hxx"
#include <FDM/JSBSim/FGFDMExec.h>
#include <FDM/JSBSim/FGJSBBase.h>
#include <FDM/JSBSim/initialization/FGInitialCondition.h>
#include <FDM/JSBSim/initialization/FGTrim.h>
#include <FDM/JSBSim/models/FGModel.h>
#include <FDM/JSBSim/models/FGAircraft.h>
#include <FDM/JSBSim/models/FGFCS.h>
#include <FDM/JSBSim/models/FGPropagate.h>
#include <FDM/JSBSim/models/FGAuxiliary.h>
#include <FDM/JSBSim/models/FGInertial.h>
#include <FDM/JSBSim/models/FGAtmosphere.h>
#include <FDM/JSBSim/models/FGMassBalance.h>
#include <FDM/JSBSim/models/FGAerodynamics.h>
#include <FDM/JSBSim/models/FGLGear.h>
#include <FDM/JSBSim/models/FGGroundReactions.h>
#include <FDM/JSBSim/models/FGPropulsion.h>
#include <FDM/JSBSim/models/propulsion/FGEngine.h>
#include <FDM/JSBSim/models/propulsion/FGPiston.h>
#include <FDM/JSBSim/models/propulsion/FGTurbine.h>
#include <FDM/JSBSim/models/propulsion/FGTurboProp.h>
#include <FDM/JSBSim/models/propulsion/FGRocket.h>
#include <FDM/JSBSim/models/propulsion/FGElectric.h>
#include <FDM/JSBSim/models/propulsion/FGNozzle.h>
#include <FDM/JSBSim/models/propulsion/FGPropeller.h>
#include <FDM/JSBSim/models/propulsion/FGRotor.h>
#include <FDM/JSBSim/models/propulsion/FGTank.h>
#include <FDM/JSBSim/input_output/FGPropertyManager.h>
#include <FDM/JSBSim/input_output/FGGroundCallback.h>

using namespace JSBSim;

static inline double
FMAX (double a, double b)
{
  return a > b ? a : b;
}

class FGFSGroundCallback : public FGGroundCallback {
public:
  FGFSGroundCallback(FGJSBsim* ifc) : mInterface(ifc) {}
  virtual ~FGFSGroundCallback() {}

  /** Get the altitude above sea level dependent on the location. */
  virtual double GetAltitude(const FGLocation& l) const {
    double pt[3] = { SG_FEET_TO_METER*l(eX),
                     SG_FEET_TO_METER*l(eY),
                     SG_FEET_TO_METER*l(eZ) };
    double lat, lon, alt;
    sgCartToGeod( pt, &lat, &lon, &alt);
    return alt * SG_METER_TO_FEET;
  }

  /** Compute the altitude above ground. */
  virtual double GetAGLevel(double t, const FGLocation& l,
                            FGLocation& cont,
                            FGColumnVector3& n, FGColumnVector3& v) const {
    double loc_cart[3] = { l(eX), l(eY), l(eZ) };
    double contact[3], normal[3], vel[3], agl = 0;
    mInterface->get_agl_ft(t, loc_cart, SG_METER_TO_FEET*2, contact, normal,
                           vel, &agl);
    n = FGColumnVector3( normal[0], normal[1], normal[2] );
    v = FGColumnVector3( vel[0], vel[1], vel[2] );
    cont = FGColumnVector3( contact[0], contact[1], contact[2] );
    return agl;
  }
private:
  FGJSBsim* mInterface;
};

/******************************************************************************/

FGJSBsim::FGJSBsim( double dt )
  : FGInterface(dt), got_wire(false)
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

    // Register ground callback.
    fdmex->SetGroundCallback( new FGFSGroundCallback(this) );

    Atmosphere      = fdmex->GetAtmosphere();
    FCS             = fdmex->GetFCS();
    MassBalance     = fdmex->GetMassBalance();
    Propulsion      = fdmex->GetPropulsion();
    Aircraft        = fdmex->GetAircraft();
    Propagate        = fdmex->GetPropagate();
    Auxiliary       = fdmex->GetAuxiliary();
    Inertial        = fdmex->GetInertial();
    Aerodynamics    = fdmex->GetAerodynamics();
    GroundReactions = fdmex->GetGroundReactions();

    fgic=fdmex->GetIC();
    needTrim=true;

    SGPath aircraft_path( fgGetString("/sim/aircraft-dir") );

    SGPath engine_path( fgGetString("/sim/aircraft-dir") );
    engine_path.append( "Engine" );

    SGPath systems_path( fgGetString("/sim/aircraft-dir") );
    systems_path.append( "Systems" );

// deprecate sim-time-sec for simulation/sim-time-sec
// remove alias with increased configuration file version number (2.1 or later)
    SGPropertyNode * node = fgGetNode("/fdm/jsbsim/simulation/sim-time-sec");
    fgGetNode("/fdm/jsbsim/sim-time-sec", true)->alias( node );
// end of sim-time-sec deprecation patch

    fdmex->Setdt( dt );

    result = fdmex->LoadModel( aircraft_path.str(),
                               engine_path.str(),
                               systems_path.str(),
                               fgGetString("/sim/aero"), false );

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
        node->setDoubleValue("level-lbs", Propulsion->GetTank(i)->GetContents());
        node->setDoubleValue("level-gal_us", Propulsion->GetTank(i)->GetContents() / 6.6);
      }
      node->setDoubleValue("capacity-gal_us",
                           Propulsion->GetTank(i)->GetCapacity() / 6.6);
    }
    Propulsion->SetFuelFreeze((fgGetNode("/sim/freeze/fuel",true))->getBoolValue());

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
    tailhook_pos_pct=fgGetNode("/gear/tailhook/position-norm",true);
    wing_fold_pos_pct=fgGetNode("surface-positions/wing-fold-pos-norm",true);

    elevator_pos_pct->setDoubleValue(0);
    left_aileron_pos_pct->setDoubleValue(0);
    right_aileron_pos_pct->setDoubleValue(0);
    rudder_pos_pct->setDoubleValue(0);
    flap_pos_pct->setDoubleValue(0);
    speedbrake_pos_pct->setDoubleValue(0);
    spoilers_pos_pct->setDoubleValue(0);

    ab_brake_engaged = fgGetNode("/autopilot/autobrake/engaged", true);
    ab_brake_left_pct = fgGetNode("/autopilot/autobrake/brake-left-output", true);
    ab_brake_right_pct = fgGetNode("/autopilot/autobrake/brake-right-output", true);
    
    temperature = fgGetNode("/environment/temperature-degc",true);
    pressure = fgGetNode("/environment/pressure-inhg",true);
    density = fgGetNode("/environment/density-slugft3",true);
    turbulence_gain = fgGetNode("/environment/turbulence/magnitude-norm",true);
    turbulence_rate = fgGetNode("/environment/turbulence/rate-hz",true);

    wind_from_north= fgGetNode("/environment/wind-from-north-fps",true);
    wind_from_east = fgGetNode("/environment/wind-from-east-fps" ,true);
    wind_from_down = fgGetNode("/environment/wind-from-down-fps" ,true);

    slaved = fgGetNode("/sim/slaved/enabled", true);

    for (unsigned int i = 0; i < Propulsion->GetNumEngines(); i++) {
      SGPropertyNode * node = fgGetNode("engines/engine", i, true);
      Propulsion->GetEngine(i)->GetThruster()->SetRPM(node->getDoubleValue("rpm") /
                     Propulsion->GetEngine(i)->GetThruster()->GetGearRatio());
    }

    hook_root_struct = FGColumnVector3(
        fgGetDouble("/fdm/jsbsim/systems/hook/tailhook-offset-x-in", 196),
        fgGetDouble("/fdm/jsbsim/systems/hook/tailhook-offset-y-in", 0),
        fgGetDouble("/fdm/jsbsim/systems/hook/tailhook-offset-z-in", -16));

    crashed = false;
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
    SG_LOG( SG_FLIGHT, SG_INFO, "Starting and initializing JSBsim" );

    // Explicitly call the superclass's
    // init method first.

    if (fgGetBool("/environment/params/control-fdm-atmosphere")) {
      Atmosphere->UseExternal();
      Atmosphere->SetExTemperature(
                  9.0/5.0*(temperature->getDoubleValue()+273.15) );
      Atmosphere->SetExPressure(pressure->getDoubleValue()*70.726566);
      Atmosphere->SetExDensity(density->getDoubleValue());
      Atmosphere->SetTurbType(FGAtmosphere::ttCulp);
      Atmosphere->SetTurbGain(turbulence_gain->getDoubleValue());
      Atmosphere->SetTurbRate(turbulence_rate->getDoubleValue());

    } else {
      Atmosphere->UseInternal();
    }

    fgic->SetVNorthFpsIC( -wind_from_north->getDoubleValue() );
    fgic->SetVEastFpsIC( -wind_from_east->getDoubleValue() );
    fgic->SetVDownFpsIC( -wind_from_down->getDoubleValue() );

    //Atmosphere->SetExTemperature(get_Static_temperature());
    //Atmosphere->SetExPressure(get_Static_pressure());
    //Atmosphere->SetExDensity(get_Density());
    SG_LOG(SG_FLIGHT,SG_INFO,"T,p,rho: " << fdmex->GetAtmosphere()->GetTemperature()
     << ", " << fdmex->GetAtmosphere()->GetPressure()
     << ", " << fdmex->GetAtmosphere()->GetDensity() );

// deprecate egt_degf for egt-degf to have consistent naming
// TODO: raise log-level to ALERT in summer 2010, 
// remove alias in fall 2010, 
// remove this code in winter 2010
    for (unsigned int i=0; i < Propulsion->GetNumEngines(); i++) {
      SGPropertyNode * node = fgGetNode("engines/engine", i, true);
      SGPropertyNode * egtn = node->getNode( "egt_degf" );
      if( egtn != NULL ) {
        SG_LOG(SG_FLIGHT,SG_WARN,
               "Aircraft uses deprecated node egt_degf. Please upgrade to egt-degf");
        node->getNode("egt-degf", true)->alias( egtn );
      }
    }
// end of egt_degf deprecation patch

    if (fgGetBool("/sim/presets/running")) {
          for (unsigned int i=0; i < Propulsion->GetNumEngines(); i++) {
            SGPropertyNode * node = fgGetNode("engines/engine", i, true);
            node->setBoolValue("running", true);
            Propulsion->GetEngine(i)->SetRunning(true);
          }
    }

    FCS->SetDfPos( ofNorm, globals->get_controls()->get_flaps() );

    common_init();

    copy_to_JSBsim();
    fdmex->RunIC();     //loop JSBSim once w/o integrating
    copy_from_JSBsim(); //update the bus

    SG_LOG( SG_FLIGHT, SG_INFO, "  Initialized JSBSim with:" );

    switch(fgic->GetSpeedSet()) {
    case setned:
        SG_LOG(SG_FLIGHT,SG_INFO, "  Vn,Ve,Vd= "
               << Propagate->GetVel(FGJSBBase::eNorth) << ", "
               << Propagate->GetVel(FGJSBBase::eEast) << ", "
               << Propagate->GetVel(FGJSBBase::eDown) << " ft/s");
    break;
    case setuvw:
        SG_LOG(SG_FLIGHT,SG_INFO, "  U,V,W= "
               << Propagate->GetUVW(1) << ", "
               << Propagate->GetUVW(2) << ", "
               << Propagate->GetUVW(3) << " ft/s");
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
            << Propagate->GetEuler(FGJSBBase::ePhi)*RADTODEG << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Pitch Angle: "
            << Propagate->GetEuler(FGJSBBase::eTht)*RADTODEG << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  True Heading: "
            << Propagate->GetEuler(FGJSBBase::ePsi)*RADTODEG << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Latitude: "
            << Propagate->GetLocation().GetLatitudeDeg() << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Longitude: "
            << Propagate->GetLocation().GetLongitudeDeg() << " deg" );
    SG_LOG( SG_FLIGHT, SG_INFO, "  Altitude: "
            << Propagate->GetAltitudeASL() << " feet" );
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
    if(crashed) {
      if(!fgGetBool("/sim/crashed"))
        fgSetBool("/sim/crashed", true);
      return;
    }

    if (is_suspended())
      return;

    int multiloop = _calc_multiloop(dt);

    int i;

    // Compute the radius of the aircraft. That is the radius of a ball
    // where all gear units are in. At the moment it is at least 10ft ...
    double acrad = 10.0;
    int n_gears = GroundReactions->GetNumGearUnits();
    for (i=0; i<n_gears; ++i) {
      FGColumnVector3 bl = GroundReactions->GetGearUnit(i)->GetBodyLocation();
      double r = bl.Magnitude();
      if (acrad < r)
        acrad = r;
    }

    // Compute the potential movement of this aircraft and query for the
    // ground in this area.
    double groundCacheRadius = acrad + 2*dt*Propagate->GetUVW().Magnitude();
    double alt, slr, lat, lon;
    FGColumnVector3 cart = Auxiliary->GetLocationVRP();
    if ( needTrim && startup_trim->getBoolValue() ) {
      alt = fgic->GetAltitudeASLFtIC();
      slr = fgic->GetSeaLevelRadiusFtIC();
      lat = fgic->GetLatitudeDegIC() * SGD_DEGREES_TO_RADIANS;
      lon = fgic->GetLongitudeDegIC() * SGD_DEGREES_TO_RADIANS;
      cart = FGLocation(lon, lat, alt+slr);
    }
    double cart_pos[3] = { cart(1), cart(2), cart(3) };
    double t0 = fdmex->GetSimTime();
    bool cache_ok = prepare_ground_cache_ft( t0, t0 + dt, cart_pos,
                                             groundCacheRadius );
    if (!cache_ok) {
      SG_LOG(SG_FLIGHT, SG_WARN,
             "FGInterface is being called without scenery below the aircraft!");

      alt = fgic->GetAltitudeASLFtIC();
      SG_LOG(SG_FLIGHT, SG_WARN, "altitude         = " << alt);

      slr = fgic->GetSeaLevelRadiusFtIC();
      SG_LOG(SG_FLIGHT, SG_WARN, "sea level radius = " << slr);

      lat = fgic->GetLatitudeDegIC() * SGD_DEGREES_TO_RADIANS;
      SG_LOG(SG_FLIGHT, SG_WARN, "latitude         = " << lat);

      lon = fgic->GetLongitudeDegIC() * SGD_DEGREES_TO_RADIANS;
      SG_LOG(SG_FLIGHT, SG_WARN, "longitude        = " << lon);
      //return;
    }

    copy_to_JSBsim();

    trimmed->setBoolValue(false);

    if ( needTrim ) {
      if ( startup_trim->getBoolValue() ) {
        double contact[3], d[3], agl;
        get_agl_ft(fdmex->GetSimTime(), cart_pos, SG_METER_TO_FEET*2, contact,
                   d, d, &agl);
        double terrain_alt = sqrt(contact[0]*contact[0] + contact[1]*contact[1]
             + contact[2]*contact[2]) - fgic->GetSeaLevelRadiusFtIC();

        SG_LOG(SG_FLIGHT, SG_INFO,
          "Ready to trim, terrain elevation is: "
            << terrain_alt * SG_METER_TO_FEET );

        fgic->SetTerrainElevationFtIC( terrain_alt );
        do_trim();
      } else {
        fdmex->RunIC();  //apply any changes made through the set_ functions
      }
      needTrim = false;
    }

    for ( i=0; i < multiloop; i++ ) {
      fdmex->Run();
      update_external_forces(fdmex->GetSimTime() + i * fdmex->GetDeltaT());      
    }

    FGJSBBase::Message* msg;
    while (msg = fdmex->ProcessNextMessage()) {
//      msg = fdmex->ProcessNextMessage();
      switch (msg->type) {
      case FGJSBBase::Message::eText:
        if (msg->text == "Crash Detected: Simulation FREEZE.")
          crashed = true;
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
    FCS->SetDsCmd( globals->get_controls()->get_rudder() );
    FCS->SetDfCmd( globals->get_controls()->get_flaps() );
    FCS->SetDsbCmd( globals->get_controls()->get_speedbrake() );
    FCS->SetDspCmd( globals->get_controls()->get_spoilers() );

        // Parking brake sets minimum braking
        // level for mains.
    double parking_brake = globals->get_controls()->get_brake_parking();
    double left_brake = globals->get_controls()->get_brake_left();
    double right_brake = globals->get_controls()->get_brake_right();
    
    if (ab_brake_engaged->getBoolValue()) {
      left_brake = ab_brake_left_pct->getDoubleValue();
      right_brake = ab_brake_right_pct->getDoubleValue(); 
    }
    
    FCS->SetLBrake(FMAX(left_brake, parking_brake));
    FCS->SetRBrake(FMAX(right_brake, parking_brake));
    
    
    FCS->SetCBrake( 0.0 );
    // FCS->SetCBrake( globals->get_controls()->get_brake(2) );

    FCS->SetGearCmd( globals->get_controls()->get_gear_down());
    for (i = 0; i < Propulsion->GetNumEngines(); i++) {
      SGPropertyNode * node = fgGetNode("engines/engine", i, true);

      FCS->SetThrottleCmd(i, globals->get_controls()->get_throttle(i));
      FCS->SetMixtureCmd(i, globals->get_controls()->get_mixture(i));
      FCS->SetPropAdvanceCmd(i, globals->get_controls()->get_prop_advance(i));
      FCS->SetFeatherCmd(i, globals->get_controls()->get_feather(i));

      switch (Propulsion->GetEngine(i)->GetType()) {
      case FGEngine::etPiston:
        { // FGPiston code block
        FGPiston* eng = (FGPiston*)Propulsion->GetEngine(i);
        eng->SetMagnetos( globals->get_controls()->get_magnetos(i) );
        break;
        } // end FGPiston code block
      case FGEngine::etTurbine:
        { // FGTurbine code block
        FGTurbine* eng = (FGTurbine*)Propulsion->GetEngine(i);
        eng->SetAugmentation( globals->get_controls()->get_augmentation(i) );
        eng->SetReverse( globals->get_controls()->get_reverser(i) );
        //eng->SetInjection( globals->get_controls()->get_water_injection(i) );
        eng->SetCutoff( globals->get_controls()->get_cutoff(i) );
        eng->SetIgnition( globals->get_controls()->get_ignition(i) );
        break;
        } // end FGTurbine code block
      case FGEngine::etRocket:
        { // FGRocket code block
        FGRocket* eng = (FGRocket*)Propulsion->GetEngine(i);
        break;
        } // end FGRocket code block
      case FGEngine::etTurboprop:
        { // FGTurboProp code block
        FGTurboProp* eng = (FGTurboProp*)Propulsion->GetEngine(i);
        eng->SetReverse( globals->get_controls()->get_reverser(i) );
        eng->SetCutoff( globals->get_controls()->get_cutoff(i) );
        eng->SetIgnition( globals->get_controls()->get_ignition(i) );

        eng->SetGeneratorPower( globals->get_controls()->get_generator_breaker(i) );
        eng->SetCondition( globals->get_controls()->get_condition(i) );
        break;
        } // end FGTurboProp code block
      default:
        break;
      }

      { // FGEngine code block
      FGEngine* eng = Propulsion->GetEngine(i);

      eng->SetStarter( globals->get_controls()->get_starter(i) );
      eng->SetRunning( node->getBoolValue("running") );
      } // end FGEngine code block
    }


    Propagate->SetSeaLevelRadius( get_Sea_level_radius() );

    Atmosphere->SetExTemperature(
                  9.0/5.0*(temperature->getDoubleValue()+273.15) );
    Atmosphere->SetExPressure(pressure->getDoubleValue()*70.726566);
    Atmosphere->SetExDensity(density->getDoubleValue());

    tmp = turbulence_gain->getDoubleValue();
    //Atmosphere->SetTurbGain(tmp * tmp * 100.0);

    tmp = turbulence_rate->getDoubleValue();
    //Atmosphere->SetTurbRate(tmp);

    Atmosphere->SetWindNED( -wind_from_north->getDoubleValue(),
                            -wind_from_east->getDoubleValue(),
                            -wind_from_down->getDoubleValue() );
//    SG_LOG(SG_FLIGHT,SG_INFO, "Wind NED: "
//                  << get_V_north_airmass() << ", "
//                  << get_V_east_airmass()  << ", "
//                  << get_V_down_airmass() );

    for (i = 0; i < Propulsion->GetNumTanks(); i++) {
      SGPropertyNode * node = fgGetNode("/consumables/fuel/tank", i, true);
      FGTank * tank = Propulsion->GetTank(i);
      tank->SetContents(node->getDoubleValue("level-gal_us") * 6.6);
//       tank->SetContents(node->getDoubleValue("level-lbs"));
    }

    Propulsion->SetFuelFreeze((fgGetNode("/sim/freeze/fuel",true))->getBoolValue());
    fdmex->SetChild(slaved->getBoolValue());

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

    _set_Velocities_Local( Propagate->GetVel(FGJSBBase::eNorth),
                           Propagate->GetVel(FGJSBBase::eEast),
                           Propagate->GetVel(FGJSBBase::eDown) );

    _set_Velocities_Wind_Body( Propagate->GetUVW(1),
                               Propagate->GetUVW(2),
                               Propagate->GetUVW(3) );

    // Make the HUD work ...
    _set_Velocities_Ground( Propagate->GetVel(FGJSBBase::eNorth),
                            Propagate->GetVel(FGJSBBase::eEast),
                            -Propagate->GetVel(FGJSBBase::eDown) );

    _set_V_rel_wind( Auxiliary->GetVt() );

    _set_V_equiv_kts( Auxiliary->GetVequivalentKTS() );

    _set_V_calibrated_kts( Auxiliary->GetVcalibratedKTS() );

    _set_V_ground_speed( Auxiliary->GetVground() );

    _set_Omega_Body( Propagate->GetPQR(FGJSBBase::eP),
                     Propagate->GetPQR(FGJSBBase::eQ),
                     Propagate->GetPQR(FGJSBBase::eR) );

    _set_Euler_Rates( Auxiliary->GetEulerRates(FGJSBBase::ePhi),
                      Auxiliary->GetEulerRates(FGJSBBase::eTht),
                      Auxiliary->GetEulerRates(FGJSBBase::ePsi) );

    _set_Mach_number( Auxiliary->GetMach() );

    // Positions of Visual Reference Point
    FGLocation l = Auxiliary->GetLocationVRP();
    _updateGeocentricPosition( l.GetLatitude(), l.GetLongitude(),
                               l.GetRadius() - get_Sea_level_radius() );

    _set_Altitude_AGL( Propagate->GetDistanceAGL() );
    {
      double loc_cart[3] = { l(FGJSBBase::eX), l(FGJSBBase::eY), l(FGJSBBase::eZ) };
      double contact[3], d[3], sd, t;
      is_valid_m(&t, d, &sd);
      get_agl_ft(t, loc_cart, SG_METER_TO_FEET*2, contact, d, d, &sd);
      double rwrad
        = FGColumnVector3( contact[0], contact[1], contact[2] ).Magnitude();
      _set_Runway_altitude( rwrad - get_Sea_level_radius() );
    }

    _set_Euler_Angles( Propagate->GetEuler(FGJSBBase::ePhi),
                       Propagate->GetEuler(FGJSBBase::eTht),
                       Propagate->GetEuler(FGJSBBase::ePsi) );

    _set_Alpha( Auxiliary->Getalpha() );
    _set_Beta( Auxiliary->Getbeta() );


    _set_Gamma_vert_rad( Auxiliary->GetGamma() );

    _set_Earth_position_angle( Inertial->GetEarthPositionAngle() );

    _set_Climb_Rate( Propagate->Gethdot() );

    const FGMatrix33& Tl2b = Propagate->GetTl2b();
    for ( i = 1; i <= 3; i++ ) {
        for ( j = 1; j <= 3; j++ ) {
            _set_T_Local_to_Body( i, j, Tl2b(i,j) );
        }
    }

    // Copy the engine values from JSBSim.
    for ( i=0; i < Propulsion->GetNumEngines(); i++ ) {
      SGPropertyNode * node = fgGetNode("engines/engine", i, true);
      SGPropertyNode * tnode = node->getChild("thruster", 0, true);
      FGThruster * thruster = Propulsion->GetEngine(i)->GetThruster();

      switch (Propulsion->GetEngine(i)->GetType()) {
      case FGEngine::etPiston:
        { // FGPiston code block
        FGPiston* eng = (FGPiston*)Propulsion->GetEngine(i);
        node->setDoubleValue("egt-degf", eng->getExhaustGasTemp_degF());
        node->setDoubleValue("oil-temperature-degf", eng->getOilTemp_degF());
        node->setDoubleValue("oil-pressure-psi", eng->getOilPressure_psi());
        node->setDoubleValue("mp-osi", eng->getManifoldPressure_inHg());
        // NOTE: mp-osi is not in ounces per square inch.
        // This error is left for reasons of backwards compatibility with
        // existing FlightGear sound and instrument configurations.
        node->setDoubleValue("mp-inhg", eng->getManifoldPressure_inHg());
        node->setDoubleValue("cht-degf", eng->getCylinderHeadTemp_degF());
        node->setDoubleValue("rpm", eng->getRPM());
        } // end FGPiston code block
        break;
      case FGEngine::etRocket:
        { // FGRocket code block
        FGRocket* eng = (FGRocket*)Propulsion->GetEngine(i);
        } // end FGRocket code block
        break;
      case FGEngine::etTurbine:
        { // FGTurbine code block
        FGTurbine* eng = (FGTurbine*)Propulsion->GetEngine(i);
        node->setDoubleValue("n1", eng->GetN1());
        node->setDoubleValue("n2", eng->GetN2());
        node->setDoubleValue("egt-degf", 32 + eng->GetEGT()*9/5);
        node->setBoolValue("augmentation", eng->GetAugmentation());
        node->setBoolValue("water-injection", eng->GetInjection());
        node->setBoolValue("ignition", eng->GetIgnition());
        node->setDoubleValue("nozzle-pos-norm", eng->GetNozzle());
        node->setDoubleValue("inlet-pos-norm", eng->GetInlet());
        node->setDoubleValue("oil-pressure-psi", eng->getOilPressure_psi());
        node->setBoolValue("reversed", eng->GetReversed());
        node->setBoolValue("cutoff", eng->GetCutoff());
        node->setDoubleValue("epr", eng->GetEPR());
        globals->get_controls()->set_reverser(i, eng->GetReversed() );
        globals->get_controls()->set_cutoff(i, eng->GetCutoff() );
        globals->get_controls()->set_water_injection(i, eng->GetInjection() );
        globals->get_controls()->set_augmentation(i, eng->GetAugmentation() );
        } // end FGTurbine code block
        break;
      case FGEngine::etTurboprop:
        { // FGTurboProp code block
        FGTurboProp* eng = (FGTurboProp*)Propulsion->GetEngine(i);
        node->setDoubleValue("n1", eng->GetN1());
        //node->setDoubleValue("n2", eng->GetN2());
        node->setDoubleValue("itt_degf", 32 + eng->GetITT()*9/5);
        node->setBoolValue("ignition", eng->GetIgnition());
        node->setDoubleValue("nozzle-pos-norm", eng->GetNozzle());
        node->setDoubleValue("inlet-pos-norm", eng->GetInlet());
        node->setDoubleValue("oil-pressure-psi", eng->getOilPressure_psi());
        node->setBoolValue("reversed", eng->GetReversed());
        node->setBoolValue("cutoff", eng->GetCutoff());
        node->setBoolValue("starting", eng->GetEngStarting());
        node->setBoolValue("generator-power", eng->GetGeneratorPower());
        node->setBoolValue("damaged", eng->GetCondition());
        node->setBoolValue("ielu-intervent", eng->GetIeluIntervent());
        node->setDoubleValue("oil-temperature-degf", eng->getOilTemp_degF());
//        node->setBoolValue("onfire", eng->GetFire());
        globals->get_controls()->set_reverser(i, eng->GetReversed() );
        globals->get_controls()->set_cutoff(i, eng->GetCutoff() );
        } // end FGTurboProp code block
        break;
      case FGEngine::etElectric:
        { // FGElectric code block
        FGElectric* eng = (FGElectric*)Propulsion->GetEngine(i);
        node->setDoubleValue("rpm", eng->getRPM());
        } // end FGElectric code block
        break;
      case FGEngine::etUnknown:
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
        tnode->setBoolValue("feathered", prop->GetFeather());
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

    // Copy the fuel levels from JSBSim if fuel
    // freeze not enabled.
    if ( ! Propulsion->GetFuelFreeze() ) {
      for (i = 0; i < Propulsion->GetNumTanks(); i++) {
        SGPropertyNode * node = fgGetNode("/consumables/fuel/tank", i, true);
        FGTank* tank = Propulsion->GetTank(i);
        double contents = tank->GetContents();
        double temp = tank->GetTemperature_degC();
        node->setDoubleValue("level-gal_us", contents/6.6);
        node->setDoubleValue("level-lbs", contents);
        if (temp != -9999.0) node->setDoubleValue("temperature_degC", temp);
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
    tailhook_pos_pct->setDoubleValue( FCS->GetTailhookPos() );
    wing_fold_pos_pct->setDoubleValue( FCS->GetWingFoldPos() );

    // force a sim crashed if crashed (altitude AGL < 0)
    if (get_Altitude_AGL() < -100.0) {
         fdmex->SuspendIntegration();
         crashed = true;
    }

    return true;
}


bool FGJSBsim::ToggleDataLogging(void)
{
  // ToDo: handle this properly
  fdmex->DisableOutput();
  return false;
}


bool FGJSBsim::ToggleDataLogging(bool state)
{
    if (state) {
      fdmex->EnableOutput();
      return true;
    } else {
      fdmex->DisableOutput();
      return false;
    }
}


//Positions
void FGJSBsim::set_Latitude(double lat)
{
    static SGConstPropertyNode_ptr altitude = fgGetNode("/position/altitude-ft");
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
    needTrim=true;
}

// Sets the altitude above sea level.
void FGJSBsim::set_Altitude(double alt)
{
    static SGConstPropertyNode_ptr latitude = fgGetNode("/position/latitude-deg");

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
    SG_LOG(SG_FLIGHT, SG_INFO,
          "Terrain elevation: " << FGInterface::get_Runway_altitude() * SG_METER_TO_FEET );
    fgic->SetLatitudeRadIC( lat_geoc );
    fgic->SetAltitudeASLFtIC(alt);
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
    fgic->SetVNorthFpsIC(north);
    fgic->SetVEastFpsIC(east);
    fgic->SetVDownFpsIC(down);
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
    fgic->SetThetaRadIC(theta);
    fgic->SetPhiRadIC(phi);
    fgic->SetPsiRadIC(psi);
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
      FGLGear *gear = gr->GetGearUnit(i);
      SGPropertyNode * node = fgGetNode("gear/gear", i, true);
      node->setDoubleValue("xoffset-in", gear->GetBodyLocation()(1));
      node->setDoubleValue("yoffset-in", gear->GetBodyLocation()(2));
      node->setDoubleValue("zoffset-in", gear->GetBodyLocation()(3));
      node->setBoolValue("wow", gear->GetWOW());
      node->setDoubleValue("rollspeed-ms", gear->GetWheelRollVel()*0.3043);
      node->setBoolValue("has-brake", gear->GetBrakeGroup() > 0);
      node->setDoubleValue("position-norm", gear->GetGearUnitPos());
      node->setDoubleValue("tire-pressure-norm", gear->GetTirePressure());
      node->setDoubleValue("compression-norm", gear->GetCompLen());
      node->setDoubleValue("compression-ft", gear->GetCompLen());
      if ( gear->GetSteerable() )
        node->setDoubleValue("steering-norm", gear->GetSteerNorm());
    }
}

void FGJSBsim::update_gear(void)
{
    FGGroundReactions* gr=fdmex->GetGroundReactions();
    int Ngear=GroundReactions->GetNumGearUnits();
    for (int i=0;i<Ngear;i++) {
      FGLGear *gear = gr->GetGearUnit(i);
      SGPropertyNode * node = fgGetNode("gear/gear", i, true);
      node->getChild("wow", 0, true)->setBoolValue( gear->GetWOW());
      node->getChild("rollspeed-ms", 0, true)->setDoubleValue(gear->GetWheelRollVel()*0.3043);
      node->getChild("position-norm", 0, true)->setDoubleValue(gear->GetGearUnitPos());
      gear->SetTirePressure(node->getDoubleValue("tire-pressure-norm"));
      node->setDoubleValue("compression-norm", gear->GetCompLen());
      node->setDoubleValue("compression-ft", gear->GetCompLen());
      if ( gear->GetSteerable() )
        node->setDoubleValue("steering-norm", gear->GetSteerNorm());
    }
}

void FGJSBsim::do_trim(void)
{
  FGTrim *fgtrim;

  if ( fgGetBool("/sim/presets/onground") )
  {
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
     fgic->SetAltitudeASLFtIC( get_Altitude() );
     fgic->SetVcalibratedKtsIC( get_V_calibrated_kts() );
     fgic->SetThetaRadIC( get_Theta() );
     fgic->SetPhiRadIC( get_Phi() );
     fgic->SetPsiRadIC( get_Psi() );
     fgic->SetClimbRateFpsIC( get_Climb_Rate() );
   }
}

bool
FGJSBsim::get_agl_ft(double t, const double pt[3], double alt_off,
                     double contact[3], double normal[3], double vel[3],
                     double *agl)
{
   double angularVel[3];
   const SGMaterial* material;
   simgear::BVHNode::Id id;
   if (!FGInterface::get_agl_ft(t, pt, alt_off, contact, normal, vel,
                                angularVel, material, id))
       return false;
   SGGeod geodPt = SGGeod::fromCart(SG_FEET_TO_METER*SGVec3d(pt));
   SGQuatd hlToEc = SGQuatd::fromLonLat(geodPt);
   *agl = dot(hlToEc.rotate(SGVec3d(0, 0, 1)), SGVec3d(contact) - SGVec3d(pt));
   return true;
}

inline static double dot3(const FGColumnVector3& a, const FGColumnVector3& b)
{
    return a(1) * b(1) + a(2) * b(2) + a(3) * b(3);
}

inline static double sqr(double x)
{
    return x * x;
}

static double angle_diff(double a, double b)
{
    double diff = fabs(a - b);
    if (diff > 180) diff = 360 - diff;
    
    return diff;
}

static void check_hook_solution(const FGColumnVector3& ground_normal_body, double E, double hook_length, double sin_fi_guess, double cos_fi_guess, double* sin_fis, double* cos_fis, double* fis, int* points)
{
    FGColumnVector3 tip(-hook_length * cos_fi_guess, 0, hook_length * sin_fi_guess);
    double dist = dot3(tip, ground_normal_body);
    if (fabs(dist + E) < 0.0001) {
	sin_fis[*points] = sin_fi_guess;
	cos_fis[*points] = cos_fi_guess;
	fis[*points] = atan2(sin_fi_guess, cos_fi_guess) * SG_RADIANS_TO_DEGREES;
	(*points)++;
    } 
}


static void check_hook_solution(const FGColumnVector3& ground_normal_body, double E, double hook_length, double sin_fi_guess, double* sin_fis, double* cos_fis, double* fis, int* points)
{
    if (sin_fi_guess >= -1 && sin_fi_guess <= 1) {
	double cos_fi_guess = sqrt(1 - sqr(sin_fi_guess));
	check_hook_solution(ground_normal_body, E, hook_length, sin_fi_guess, cos_fi_guess, sin_fis, cos_fis, fis, points);
	if (fabs(cos_fi_guess) > SG_EPSILON) {
	    check_hook_solution(ground_normal_body, E, hook_length, sin_fi_guess, -cos_fi_guess, sin_fis, cos_fis, fis, points);
	}
    }
}

void FGJSBsim::update_external_forces(double t_off)
{
    const FGMatrix33& Tb2l = Propagate->GetTb2l();
    const FGMatrix33& Tl2b = Propagate->GetTl2b();
    const FGLocation& Location = Propagate->GetLocation();
    const FGMatrix33& Tec2l = Location.GetTec2l();
        
    double hook_area[4][3];
    
    FGColumnVector3 hook_root_body = MassBalance->StructuralToBody(hook_root_struct);
    FGColumnVector3 hook_root = Location.LocalToLocation(Tb2l *   hook_root_body);
    hook_area[1][0] = hook_root(1);
    hook_area[1][1] = hook_root(2);
    hook_area[1][2] = hook_root(3);
    
    hook_length = fgGetDouble("/fdm/jsbsim/systems/hook/tailhook-length-ft", 6.75);
    double fi_min = fgGetDouble("/fdm/jsbsim/systems/hook/tailhook-pos-min-deg", -18);
    double fi_max = fgGetDouble("/fdm/jsbsim/systems/hook/tailhook-pos-max-deg", 30);
    double fi = fgGetDouble("/fdm/jsbsim/systems/hook/tailhook-pos-norm") * (fi_max - fi_min) + fi_min;
    double cos_fi = cos(fi * SG_DEGREES_TO_RADIANS);
    double sin_fi = sin(fi * SG_DEGREES_TO_RADIANS);

    FGColumnVector3 hook_tip_body = hook_root_body;
    hook_tip_body(1) -= hook_length * cos_fi;
    hook_tip_body(3) += hook_length * sin_fi;    
    
    double contact[3];
    double ground_normal[3];
    double ground_vel[3];
    double root_agl_ft;

    if (!got_wire) {
        bool got = get_agl_ft(t_off, hook_area[1], 0, contact, ground_normal, ground_vel, &root_agl_ft);
        if (got && root_agl_ft > 0 && root_agl_ft < hook_length) {
            FGColumnVector3 ground_normal_body = Tl2b * (Tec2l * FGColumnVector3(ground_normal[0], ground_normal[1], ground_normal[2]));
            FGColumnVector3 contact_body = Tl2b * Location.LocationToLocal(FGColumnVector3(contact[0], contact[1], contact[2]));
            double D = -dot3(contact_body, ground_normal_body);

	    // check hook tip agl against same ground plane
	    double hook_tip_agl_ft = dot3(hook_tip_body, ground_normal_body) + D;
	    if (hook_tip_agl_ft < 0) {

        	// hook tip: hx - l cos, hy, hz + l sin
        	// on ground:  - n0 l cos + n2 l sin + E = 0

        	double E = D + dot3(hook_root_body, ground_normal_body);

        	// substitue x = sin fi, cos fi = sqrt(1 - x * x)
		// and rearrange to get a quadratic with coeffs:
        	double a = sqr(hook_length) * (sqr(ground_normal_body(1)) + sqr(ground_normal_body(3)));
        	double b = 2 * E * ground_normal_body(3) * hook_length;
        	double c = sqr(E) - sqr(ground_normal_body(1) * hook_length);	

        	double disc = sqr(b) - 4 * a * c;
        	if (disc >= 0) {
		    double delta = sqrt(disc) / (2 * a);
		
		    // allow 4 solutions for safety, should never happen
		    double sin_fis[4];
		    double cos_fis[4];
		    double fis[4];
		    int points = 0;
		
        	    double sin_fi_guess = -b / (2 * a) - delta;
		    check_hook_solution(ground_normal_body, E, hook_length, sin_fi_guess, sin_fis, cos_fis, fis, &points);
		    check_hook_solution(ground_normal_body, E, hook_length, sin_fi_guess + 2 * delta, sin_fis, cos_fis, fis, &points);
		
		    if (points == 2) {
			double diff1 = angle_diff(fi, fis[0]);
			double diff2 = angle_diff(fi, fis[1]);
			int point = diff1 < diff2 ? 0 : 1;
			fi = fis[point];
			sin_fi = sin_fis[point];
			cos_fi = cos_fis[point];
			hook_tip_body(1) = hook_root_body(1) - hook_length * cos_fi;
			hook_tip_body(3) = hook_root_body(3) + hook_length * sin_fi;
		    }
        	}
    	    }
	}
    } else {
        FGColumnVector3 hook_root_vel = Propagate->GetVel() + (Tb2l * (Propagate->GetPQR() *  hook_root_body));
        double wire_ends_ec[2][3];
        double wire_vel_ec[2][3];
        get_wire_ends_ft(t_off, wire_ends_ec, wire_vel_ec);
        FGColumnVector3 wire_vel_1 = Tec2l * FGColumnVector3(wire_vel_ec[0][0], wire_vel_ec[0][1], wire_vel_ec[0][2]);
        FGColumnVector3 wire_vel_2 = Tec2l * FGColumnVector3(wire_vel_ec[1][0], wire_vel_ec[1][1], wire_vel_ec[1][2]);
        FGColumnVector3 rel_vel = hook_root_vel - (wire_vel_1 + wire_vel_2) / 2;
        if (rel_vel.Magnitude() < 3) {
            got_wire = false;
            release_wire();
            fgSetDouble("/fdm/jsbsim/external_reactions/hook/magnitude", 0.0);
        } else {
            FGColumnVector3 wire_end1_body = Tl2b * Location.LocationToLocal(FGColumnVector3(wire_ends_ec[0][0], wire_ends_ec[0][1], wire_ends_ec[0][2])) - hook_root_body;
            FGColumnVector3 wire_end2_body = Tl2b * Location.LocationToLocal(FGColumnVector3(wire_ends_ec[1][0], wire_ends_ec[1][1], wire_ends_ec[1][2])) - hook_root_body;
            FGColumnVector3 force_plane_normal = wire_end1_body * wire_end2_body;
            force_plane_normal.Normalize();
            cos_fi = dot3(force_plane_normal, FGColumnVector3(0, 0, 1));
            if (cos_fi < 0) cos_fi = -cos_fi;
            sin_fi = sqrt(1 - sqr(cos_fi));
            fi = atan2(sin_fi, cos_fi) * SG_RADIANS_TO_DEGREES;
        
            fgSetDouble("/fdm/jsbsim/external_reactions/hook/x", -cos_fi);
            fgSetDouble("/fdm/jsbsim/external_reactions/hook/y", 0);
            fgSetDouble("/fdm/jsbsim/external_reactions/hook/z", sin_fi);
            fgSetDouble("/fdm/jsbsim/external_reactions/hook/magnitude", fgGetDouble("/fdm/jsbsim/systems/hook/force"));
        }
    }

    FGColumnVector3 hook_tip = Location.LocalToLocation(Tb2l * hook_tip_body);

    hook_area[0][0] = hook_tip(1);
    hook_area[0][1] = hook_tip(2);
    hook_area[0][2] = hook_tip(3);

    if (!got_wire) {
        // The previous positions.
        hook_area[2][0] = last_hook_root[0];
        hook_area[2][1] = last_hook_root[1];
        hook_area[2][2] = last_hook_root[2];
        hook_area[3][0] = last_hook_tip[0];
        hook_area[3][1] = last_hook_tip[1];
        hook_area[3][2] = last_hook_tip[2];

        // Check if we caught a wire.
        // Returns true if we caught one.
        if (caught_wire_ft(t_off, hook_area)) {
                got_wire = true;
        }
    }
    
    // save actual position as old position ...
    last_hook_tip[0] = hook_area[0][0];
    last_hook_tip[1] = hook_area[0][1];
    last_hook_tip[2] = hook_area[0][2];
    last_hook_root[0] = hook_area[1][0];
    last_hook_root[1] = hook_area[1][1];
    last_hook_root[2] = hook_area[1][2];
    
    fgSetDouble("/fdm/jsbsim/systems/hook/tailhook-pos-deg", fi);
}

