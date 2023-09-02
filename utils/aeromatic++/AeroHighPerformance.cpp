// AeroHighPerformance.cpp -- Implements a Aeromatic Performance Aircraft type.
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

Performance::Performance(Aeromatic *p) : Aircraft(p)
{
    _description = "High Performance";

    _subclasses.push_back("WWII Fighter");
    _subclasses.push_back("Military Trainer");
    _subclasses.push_back("Aerobatic");
    _subclasses.push_back("Air Racer");

    _systems.push_back(new Propulsion(_aircraft));
    _systems.push_back(new CableControls(_aircraft));
    _systems.push_back(new LandingGear(_aircraft));
    _systems.push_back(new Flaps(_aircraft));
    _systems.push_back(new Spoilers(_aircraft));
}

void Performance::set_lift()
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

void Performance::set_drag()
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

void Performance::set_side()
{
    if (_aircraft->_CYbeta == 0.0f) {
        _aircraft->_CYbeta = -1.0f;
    }
}

void Performance::set_roll()
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

void Performance::set_pitch()
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

void Performance::set_yaw()
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

float const Performance::_fuselage_diameter_t[1][5] =
{
    { 2.75f, 3.25f, 3.75f, 4.25f, 4.5f }
};

float const Performance::_wing_loading_t[1][5] =
{
    {  45.0f,  45.0f,  45.0f,  45.0f,  45.0f }
};

float const Performance::_aspect_ratio_t[1][5] =
{
    {  5.5f,  6.7f, 10.7f, 10.7f, 10.7f }
};

float const Performance::_htail_area_t[1][5] =
{
    { 0.17f, 0.17f, 0.17f, 0.17f, 0.17f }
};

float const Performance::_htail_arm_t[1][5] =
{
    { 0.60f, 0.60f, 0.60f, 0.60f }
};

float const Performance::_vtail_area_t[1][5] =
{
    { 0.10f, 0.10f, 0.10f, 0.10f, 0.10f }
};

float const Performance::_vtail_arm_t[1][5] =
{ 
    { 0.60f, 0.60f, 0.60f, 0.60f, 0.60f }
};

float const Performance::_empty_weight_t[1][5] =
{
    { 0.61f, 0.61f, 0.61f, 0.61f, 0.61f }
};

float const Performance::_roskam_t[1][5][3] =
{
    {
        { 0.27f, 0.36f, 0.42f },
        { 0.27f, 0.36f, 0.42f },
        { 0.27f, 0.36f, 0.42f },
        { 0.27f, 0.36f, 0.42f },
        { 0.27f, 0.36f, 0.42f }
    },
};

float const Performance::_eyept_loc_t[1][5][3] =
{ 
    {
        { 0.28f,   0.00f, 40.00f },
        { 0.28f,   0.00f, 40.00f },
        { 0.28f,   0.00f, 40.00f },
        { 0.28f,   0.00f, 40.00f },
        { 0.28f,   0.00f, 40.00f }
    },
};

float const Performance::_gear_loc_t[1][5] =
{
    {  0.15f, 0.15f, 0.15f, 0.15f, 0.15f }
};

float const Performance::_fuel_weight_t[1][5] =
{
    { 0.122f, 0.122f, 0.122f, 0.122f, 0.122f }
};

float const Performance::_CLalpha_t[1][5] =
{
    { 4.5f, 4.5f, 4.5f, 4.5f, 4.5f }
};

float const Performance::_CL0_t[1][5] =
{
    { 0.17f, 0.17f, 0.17f, 0.17f, 0.17f }
};

float const Performance::_CLmax_t[1][5] =
{
    { 1.20f, 1.20f, 1.20f, 1.20f, 1.20f }
};

float const Performance::_CD0_t[1][5] =
{
    { 0.020f, 0.020f, 0.020f, 0.020f, 0.020f }
};

float const Performance::_K_t[1][5] =
{
    { 0.060f, 0.060f, 0.060f, 0.060f, 0.060f }
};

float const Performance::_Mcrit_t[1][5] =
{
    { 0.75f, 0.75f, 0.75f, 0.75f, 0.75f }
};

float const Performance::_Cmalpha_t[1][5] =
{
    { -0.5f, -0.5f, -0.5f, -0.5f, -0.5f }
};

float const Performance::_Cmde_t[1][5] =
{
    { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f }
};

float const Performance::_Cmq_t[1][5] =
{
    { -15.0f, -15.0f, -15.0f, -15.0f, -15.0f }
};

float const Performance::_Cmadot_t[1][5] =
{
    {  -7.0f,  -7.0f,  -7.0f,  -7.0f,  -7.0f }
};

float const Performance::_Clda_t[1][5] =
{
    { 0.18f, 0.18f, 0.18f, 0.18f, 0.18f }
};

float const Performance::_Cnda_t[1][5] =
{
    { -0.003f, -0.003f, -0.003f, -0.003f, -0.003f }
};

} /* namespace Aeromatic */

