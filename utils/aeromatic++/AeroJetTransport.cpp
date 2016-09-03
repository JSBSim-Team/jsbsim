// AeroJetTransport.cpp -- Implements a Aeromatic Jet Transport Aircraft type.
//
// Based on Aeromatic2 PHP code by David P. Culp
// Started June 2003
//
// C++-ified and modulized by Erik Hofman, started October 2015.
//
// Copyright (C) 2003, David P. Culp <davidculp2@comcast.net>
// Copyright (C) 2015 Erik Hofman <erik@ehofman.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

#include <Systems/Systems.h>
#include <Systems/Propulsion.h>
#include <Systems/Controls.h>
#include "Aircraft.h"


namespace Aeromatic
{

JetTransport::JetTransport(Aeromatic *p) : Aircraft(p)
{
    _description = "Jet Transport";

    _subclasses.push_back("Passenger Jet Airliner");
    _subclasses.push_back("Transonic Jet Transport");

    _systems.push_back(new Propulsion(_aircraft));
    _systems.push_back(new ThrustReverse(_aircraft));
    _systems.push_back(new Controls(_aircraft));
    _systems.push_back(new LandingGear(_aircraft));
    _systems.push_back(new Flaps(_aircraft));
    _systems.push_back(new Spoilers(_aircraft));
    _systems.push_back(new Speedbrake(_aircraft));
}

void JetTransport::set_lift()
{
    // estimate slope of lift curve based on airplane type
    // units: per radian
    if (_aircraft->_CLalpha[0] == 0.0f) {
        _aircraft->_CLalpha[0] = _CLalpha_t[_subtype][_engines];
    }

    // estimate CL at zero alpha
    if (_aircraft->_CL0 == 0.0f) {
        _aircraft->_CL0 = _CL0_t[_subtype][_engines];
    }

    // estimate stall CL, based on airplane type
    if (_aircraft->_CLmax[0] == 0.0f) {
        _aircraft->_CLmax[0] = _CLmax_t[_subtype][_engines];
    }

    // estimate lift due to elevator deflection
    if (_aircraft->_CLde == 0.0f) {
        _aircraft->_CLde = 0.2f;
    }
}

void JetTransport::set_drag()
{
    // estimate drag at zero lift, based on airplane type
    // NOT including landing gear
    if (_aircraft->_CD0 == 0.0f) {
        _aircraft->_CD0 = _CD0_t[_subtype][_engines];
    }

    // estimate induced drag coefficient K
    if (_aircraft->_Kdi == 0.0f) {
        _aircraft->_Kdi = _K_t[_subtype][_engines];
    }

    if (_aircraft->_CDde == 0.0f) {
        _aircraft->_CDde = 0.04f;      // elevator deflection
    }

    if (_aircraft->_CDbeta == 0.0f) {
        _aircraft->_CDbeta = 0.2f;     // sideslip
    }

    // estimate critical mach, based on airplane type
    if ( _aircraft->_Mcrit == 0.0f) {
        _aircraft->_Mcrit = _Mcrit_t[_subtype][_engines];
    }
}

void JetTransport::set_side()
{
    if (_aircraft->_CYbeta == 0.0f) {
        _aircraft->_CYbeta = -1.0f;
    }
}

void JetTransport::set_roll()
{
    // estimate roll coefficients
    if (_aircraft->_Clbeta[0] == 0.0f) {
        _aircraft->_Clbeta[0] = -0.1f;    // sideslip
    }

    if (_aircraft->_Clp == 0.0f) {
        _aircraft->_Clp = -0.4f;       // roll rate
    }

    if (_aircraft->_Clr[0] == 0.0f) {
        _aircraft->_Clr[0] = 0.15f;       // yaw rate
    }

    if (_aircraft->_Cldr == 0.0f) {
        _aircraft->_Cldr = 0.01f;      // rudder deflection
    }

    // aileron
    if (_aircraft->_Clda == 0.0f) {
        _aircraft->_Clda = _Clda_t[_subtype][_engines];
    }
}

void JetTransport::set_pitch()
{
    // per radian alpha
    if (_aircraft->_Cmalpha == 0.0f) {
        _aircraft->_Cmalpha = _Cmalpha_t[_subtype][_engines];
    }

    // elevator deflection
    if (_aircraft->_Cmde == 0.0f) {
        _aircraft->_Cmde = _Cmde_t[_subtype][_engines];
    }

    // pitch rate
    if (_aircraft->_Cmq == 0.0f) {
        _aircraft->_Cmq = _Cmq_t[_subtype][_engines];
    }

    // alpha-dot
    if (_aircraft->_Cmadot == 0.0f) {
        _aircraft->_Cmadot = _Cmadot_t[_subtype][_engines];
    }
}

void JetTransport::set_yaw()
{
    if (_aircraft->_Cnbeta == 0.0f) {
        _aircraft->_Cnbeta = 0.12f;    // sideslip
    }

    if (_aircraft->_Cnr == 0.0f) {
        _aircraft->_Cnr = -0.15f;      // yaw rate
    }

    if (_aircraft->_Cndr == 0.0f) {
        _aircraft->_Cndr = -0.10f;        // rudder deflection
    }

    // adverse yaw
    if (_aircraft->_Cnda == 0.0f) {
        _aircraft->_Cnda = _Cnda_t[_subtype][_engines];
    }
}
// ----------------------------------------------------------------------------

float const JetTransport::_fuselage_diameter_t[1][5] =
{
    { 3.5f, 4.5f, 10.0f, 19.0f, 22.5f }
};

float const JetTransport::_wing_loading_t[1][5] =
{
    { 110.0f, 110.0f, 110.0f, 110.0f, 110.0f }
};

float const JetTransport::_aspect_ratio_t[1][5] =
{
    {  9.3f,  9.3f, 9.3f, 7.8f, 7.8f }
};

float const JetTransport::_htail_area_t[1][5] =
{
    { 0.25f, 0.25f, 0.25f, 0.25f, 0.25f }
};

float const JetTransport::_htail_arm_t[1][5] =
{
    { 0.45f, 0.45f, 0.45f, 0.45f }
};

float const JetTransport::_vtail_area_t[1][5] =
{
    { 0.20f, 0.25f, 0.20f, 0.20f, 0.20f }
};

float const JetTransport::_vtail_arm_t[1][5] =
{ 
    { 0.45f, 0.45f, 0.45f, 0.45f, 0.45f }
};

float const JetTransport::_empty_weight_t[1][5] =
{
    { 0.55f, 0.55f, 0.55f, 0.52f, 0.49f }
};

float const JetTransport::_roskam_t[1][5][3] =
{
    {
        { 0.25f, 0.38f, 0.46f },
        { 0.25f, 0.38f, 0.46f },
        { 0.25f, 0.38f, 0.46f },
        { 0.25f, 0.36f, 0.47f },
        { 0.32f, 0.34f, 0.47f }
    }
};

float const JetTransport::_eyept_loc_t[1][5][3] =
{ 
    {
        { 0.07f, -30.00f, 70.00f },
        { 0.07f, -30.00f, 70.00f },
        { 0.07f, -30.00f, 70.00f },
        { 0.07f, -30.00f, 75.00f },
        { 0.07f, -32.00f, 80.00f }
     }
};

float const JetTransport::_gear_loc_t[1][5] =
{
    {  0.09f, 0.09f, 0.09f, 0.09f, 0.09f }
};

float const JetTransport::_fuel_weight_t[1][5] =
{
    { 0.277f, 0.277f, 0.277f, 0.338f, 0.419f }
};

float const JetTransport::_CLalpha_t[1][5] =
{
    { 4.4f, 4.4f, 4.4f, 4.4f, 4.4f }
};

float const JetTransport::_CL0_t[1][5] =
{
    { 0.20f, 0.20f, 0.20f, 0.20f, 0.20f }
};

float const JetTransport::_CLmax_t[1][5] =
{
    { 1.20f, 1.20f, 1.20f, 1.20f, 1.20f }
};

float const JetTransport::_CD0_t[1][5] =
{
    { 0.020f, 0.020f, 0.020f, 0.019f, 0.017f }
};

float const JetTransport::_K_t[1][5] =
{
    { 0.043f, 0.043f, 0.043f, 0.042f, 0.042f }
};

float const JetTransport::_Mcrit_t[1][5] =
{
    { 0.79f, 0.79f, 0.79f, 0.79f, 0.79f }
};

float const JetTransport::_Cmalpha_t[1][5] =
{
    { -0.6f, -0.6f, -0.6f, -0.6f, -0.7f }
};

float const JetTransport::_Cmde_t[1][5] =
{
    { -1.2f, -1.2f, -1.2f, -1.2f, -1.3f }
};

float const JetTransport::_Cmq_t[1][5] =
{
    { -17.0f, -17.0f, -17.0f, -17.0f, -21.0f }
};

float const JetTransport::_Cmadot_t[1][5] =
{
    {  -6.0f,  -6.0f,  -6.0f,  -6.0f,  -4.0f }
};

float const JetTransport::_Clda_t[1][5] =
{
    { 0.10f, 0.10f, 0.10f, 0.10f, 0.10f }
};

float const JetTransport::_Cnda_t[1][5] =
{
    {  0.000f,  0.000f,  0.000f,  0.000f,  0.000f }
};

} /* namespace Aeromatic */

