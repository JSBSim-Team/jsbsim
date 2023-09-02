// AeroFighterJet.cpp -- Implements a Aeromatic Fighter Aircraft type.
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

Fighter::Fighter(Aeromatic *p) : Aircraft(p)
{
    _description = "Fighter Jet";

    _systems.push_back(new Propulsion(_aircraft));
    _systems.push_back(new Controls(_aircraft));
    _systems.push_back(new LandingGear(_aircraft));
    _systems.push_back(new Flaps(_aircraft));
    _systems.push_back(new Spoilers(_aircraft));
    _systems.push_back(new Speedbrake(_aircraft));
    _systems.push_back(new ArrestorHook(_aircraft));
    _systems.push_back(new DragChute(_aircraft));
    _systems.push_back(new Catapult(_aircraft));
}

void Fighter::set_lift()
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

void Fighter::set_drag()
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

void Fighter::set_side()
{
    if (_aircraft->_CYbeta == 0.0f) {
        _aircraft->_CYbeta = -1.0f;
    }
}

void Fighter::set_roll()
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

void Fighter:: set_pitch()
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

void Fighter::set_yaw()
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

float const Fighter::_fuselage_diameter_t[1][5] =
{
    { 2.75f, 3.85f, 4.5f, 4.75f, 5.25f }
};

float const Fighter::_wing_loading_t[1][5] =
{
    {  95.0f,  95.0f, 100.0f, 100.0f, 100.0f }
};

float const Fighter::_aspect_ratio_t[1][5] =
{
    {  3.2f,  3.2f, 3.5f, 4.3f, 4.3f }
};

float const Fighter::_htail_area_t[1][5] =
{
    { 0.20f, 0.20f, 0.20f, 0.20f, 0.20f }
};

float const Fighter::_htail_arm_t[1][5] =
{
    { 0.40f, 0.40f, 0.40f, 0.40f }
};

float const Fighter::_vtail_area_t[1][5] =
{
    { 0.12f, 0.12f, 0.18f, 0.18f, 0.18f }
};

float const Fighter::_vtail_arm_t[1][5] =
{ 
    { 0.40f, 0.40f, 0.40f, 0.40f, 0.40f }
};

float const Fighter::_empty_weight_t[1][5] =
{
    { 0.53f, 0.53f, 0.50f, 0.50f, 0.50f }
};

float const Fighter::_roskam_t[1][5][3] =
{
    {
        { 0.27f, 0.35f, 0.40f },
        { 0.27f, 0.35f, 0.40f },
        { 0.29f, 0.34f, 0.41f },
        { 0.29f, 0.34f, 0.41f },
        { 0.29f, 0.34f, 0.41f }
    }
};

float const Fighter::_eyept_loc_t[1][5][3] =
{ 
    {
        { 0.20f,   0.00f, 36.00f },
        { 0.20f,   0.00f, 36.00f },
        { 0.20f,   0.00f, 38.00f },
        { 0.20f,   0.00f, 38.00f },
        { 0.20f,   0.00f, 38.00f }
    }
};

float const Fighter::_gear_loc_t[1][5] =
{
    {  0.09f, 0.09f, 0.09f, 0.09f, 0.09f }
};

float const Fighter::_fuel_weight_t[1][5] =
{
    { 0.162f, 0.162f, 0.207f, 0.207f, 0.207f }
};

float const Fighter::_CLalpha_t[1][5] =
{
    { 3.5f, 3.5f, 3.6f, 3.6f, 3.6f }
};

float const Fighter::_CL0_t[1][5] =
{
    { 0.08f, 0.08f, 0.08f, 0.08f, 0.08f }
};

float const Fighter::_CLmax_t[1][5] =
{
    { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f }
};

float const Fighter::_CD0_t[1][5] =
{
    { 0.021f, 0.021f, 0.024f, 0.024f, 0.024f }
};

float const Fighter::_K_t[1][5] =
{
    { 0.120f, 0.120f, 0.120f, 0.120f, 0.120f }
};

float const Fighter::_Mcrit_t[1][5] =
{
    { 0.81f, 0.81f, 0.81f, 0.81f, 0.81f }
};

float const Fighter::_Cmalpha_t[1][5] =
{
    { -0.3f, -0.3f, -0.3f, -0.3f, -0.3f }
};

float const Fighter::_Cmde_t[1][5] =
{
    { -0.8f, -0.8f, -0.8f, -0.8f, -0.8f }
};

float const Fighter::_Cmq_t[1][5] =
{
    { -18.0f, -18.0f, -18.0f, -18.0f, -18.0f }
};

float const Fighter::_Cmadot_t[1][5] =
{
    {  -9.0f,  -9.0f,  -9.0f,  -9.0f,  -9.0f }
};

float const Fighter::_Clda_t[1][5] =
{
    { 0.11f, 0.11f, 0.12f, 0.12f, 0.12f }
};

float const Fighter::_Cnda_t[1][5] =
{
    {  0.000f,  0.000f,  0.000f,  0.000f,  0.000f }
};

} /* namespace Aeromatic */

