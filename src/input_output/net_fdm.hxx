// net_fdm.hxx -- defines a common net I/O interface to the flight
//                dynamics model
//
// Written by Curtis Olson - http://www.flightgear.org/~curt
// Started September 2001.
//
// This file is in the Public Domain, and comes with no warranty.
//


#ifndef _NET_FDM_HXX
#define _NET_FDM_HXX


#include <time.h> // time_t
#include <simgear/misc/stdint.hxx>

// This file is a modified version of FlightGear net_fdm.hxx to cope with
// versions 24 and 25 of the FDM network protocol.

// NOTE: this file defines an external interface structure.  Due to
// variability between platforms and architectures, we only used fixed
// length types here.  Specifically, integer types can vary in length.
// I am not aware of any platforms that don't use 4 bytes for float
// and 8 bytes for double.

// Define a structure containing the top level flight dynamics model
// parameters
enum
{
    FG_MAX_ENGINES = 4,
    FG_MAX_WHEELS = 3,
    FG_MAX_TANKS = 4
};

// Split the structure FGNetFDM in 3 parts: FGNetFDM2 is only used in the
// protocol version 25.
struct FGNetFDM1 // 1st part of the protocol FGNetFDM
{
    uint32_t version; // increment when data values change
    uint32_t padding; // padding

    // Positions
    double longitude; // geodetic (radians)
    double latitude;  // geodetic (radians)
    double altitude;  // above sea level (meters)
    float agl;        // above ground level (meters)
    float phi;        // roll (radians)
    float theta;      // pitch (radians)
    float psi;        // yaw or true heading (radians)
    float alpha;      // angle of attack (radians)
    float beta;       // side slip angle (radians)

    // Velocities
    float phidot;     // roll rate (radians/sec)
    float thetadot;   // pitch rate (radians/sec)
    float psidot;     // yaw rate (radians/sec)
    float vcas;       // calibrated airspeed
    float climb_rate; // feet per second
    float v_north;    // north velocity in local/body frame, fps
    float v_east;     // east velocity in local/body frame, fps
    float v_down;     // down/vertical velocity in local/body frame, fps
    float v_body_u;   // ECEF velocity in body axis
    float v_body_v;   // ECEF velocity in body axis
    float v_body_w;   // ECEF velocity in body axis

    // Accelerations
    float A_X_pilot; // X accel in body frame ft/sec^2
    float A_Y_pilot; // Y accel in body frame ft/sec^2
    float A_Z_pilot; // Z accel in body frame ft/sec^2

    // Stall
    float stall_warning; // 0.0 - 1.0 indicating the amount of stall
    float slip_deg;      // slip ball deflection

    // Pressure

    // Engine status
    uint32_t num_engines;               // Number of valid engines
    uint32_t eng_state[FG_MAX_ENGINES]; // Engine state (off, cranking, running)
    float rpm[FG_MAX_ENGINES];          // Engine RPM rev/min
    float fuel_flow[FG_MAX_ENGINES];    // Fuel flow gallons/hr
    float fuel_px[FG_MAX_ENGINES];      // Fuel pressure psi
    float egt[FG_MAX_ENGINES];          // Exhuast gas temp deg F
    float cht[FG_MAX_ENGINES];          // Cylinder head temp deg F
    float mp_osi[FG_MAX_ENGINES];       // Manifold pressure
    float tit[FG_MAX_ENGINES];          // Turbine Inlet Temperature
    float oil_temp[FG_MAX_ENGINES];     // Oil temp deg F
    float oil_px[FG_MAX_ENGINES];       // Oil pressure psi

    // Consumables
    uint32_t num_tanks;                // Max number of fuel tanks
    float fuel_quantity[FG_MAX_TANKS]; // used by GPSsmooth and possibly others
};

// 2nd part of the protocol. This one is only used in the protocol version 25.
struct FGNetFDM2
{
    uint32_t tank_selected[FG_MAX_TANKS]; // selected, capacity, usable, density and level required for multiple-pc setups to work
    double capacity_m3[FG_MAX_TANKS];
    double unusable_m3[FG_MAX_TANKS];
    double density_kgpm3[FG_MAX_TANKS];
    double level_m3[FG_MAX_TANKS];
};

// 3rd part of the network protocol. This one is used in both versions 24 and 25
struct FGNetFDM3
{
    // Gear status
    uint32_t num_wheels;
    uint32_t wow[FG_MAX_WHEELS];
    float gear_pos[FG_MAX_WHEELS];
    float gear_steer[FG_MAX_WHEELS];
    float gear_compression[FG_MAX_WHEELS];

    // Environment
    uint32_t cur_time; // current simulation time
    int32_t warp;      // offset in seconds to unix time
    float visibility;  // visibility in meters (for env. effects)

    // Control surface positions (normalized values)
    float elevator;
    float elevator_trim_tab;
    float left_flap;
    float right_flap;
    float left_aileron;
    float right_aileron;
    float rudder;
    float nose_wheel;
    float speedbrake;
    float spoilers;
};

#endif // _NET_FDM_HXX
