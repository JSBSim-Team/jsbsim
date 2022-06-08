// LandingGear.cpp -- Implements the Landing Gear system.
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

#include <sstream>
#include <iomanip>

#include <Aircraft.h>
#include "Systems.h"

namespace Aeromatic
{

LandingGear::LandingGear(Aeromatic *p) : System(p, true),
    _taildragger(false),
    _retractable(true),
    _steering(0)
{

    _description.push_back("Landing Gear");
    _inputs_order.push_back("Landing Gear");
    _inputs["Landing Gear"] = new Param(_description[0].c_str(), _supported, _enabled);

    _inputs_order.push_back("retractable");
    _inputs["retractable"] = new Param("Is landing gear retractable?", 0, _retractable);

    Param *steer = new Param("Nose or tail wheel type", 0, _steering, MAX_STEERING);
    _inputs_order.push_back("steering");
    _inputs["steering"] = steer;
    steer->add_option("steering");
    steer->add_option("castering");
    steer->add_option("fixed");

    _inputs_order.push_back("taildragger");
    _inputs["taildragger"] = new Param("Is this a taildragger?", 0, _taildragger);
}

void LandingGear::set(const float cg_loc[3])
{
    _cg_loc[X] = cg_loc[X];
    _cg_loc[Y] = cg_loc[Y];
    _cg_loc[Z] = cg_loc[Z];

    // set main gear longitudinal location relative to CG
    // set main gear length (from aircraft centerline, extended)0
    bool glider = (_aircraft->_atype == LIGHT && _aircraft->_engines == 0) ? true : false;
    if (_taildragger)
    {
        _gear_loc[MAIN][X] = cg_loc[X] * 0.91f;
        _gear_loc[MAIN][Z] = -(_aircraft->_length * 0.20f) * FEET_TO_INCH;
    }
    else
    {
        _gear_loc[MAIN][X] = cg_loc[X] * 1.04f;
        _gear_loc[MAIN][Z] = -(_aircraft->_length * 0.12f) * FEET_TO_INCH;
    }
    if (glider) {
        _gear_loc[MAIN][Z] = -(_aircraft->_length / 10.0f) * FEET_TO_INCH;
    }

    // set main gear lateral location
    _gear_loc[MAIN][Y] = (_aircraft->_wing.span * _aircraft->get_gear_loc()) * FEET_TO_INCH;

    // set nose gear location
    _gear_loc[NOSE][X] = _aircraft->_length * 0.13f * FEET_TO_INCH;
    _gear_loc[NOSE][Y] = 0;
    _gear_loc[NOSE][Z] = _gear_loc[MAIN][Z];
    if (glider) {
        _gear_loc[NOSE][Z] *= 0.6f;
    }

    // set tail gear location
    _gear_loc[TAIL][X] = _aircraft->_length * 0.91f * FEET_TO_INCH;
    _gear_loc[TAIL][Y] = 0;
    _gear_loc[TAIL][Z] = _gear_loc[MAIN][Z] * 0.30f;

    // set spring and damp coefficients
    _gear_spring[MAIN] = _aircraft->_max_weight * 1.0f;
    _gear_spring[NOSE] = _aircraft->_max_weight * 0.3f;
    _gear_spring[TAIL] = _aircraft->_max_weight * 1.0f;

    _gear_damp[MAIN] = _aircraft->_max_weight * 0.5f;
    _gear_damp[NOSE] = _aircraft->_max_weight * 0.15f;
    _gear_damp[TAIL] = _aircraft->_max_weight * 0.5f;

    _gear_static = 0.8f;
    _gear_dynamic = 0.5f;
    _gear_rolling = (glider) ? 0.5f : 0.02f;

    _gear_max_steer = 5.0f;
    if (_steering == 1) {		// castering
        _gear_max_steer = 360.0f;
    } else if (_steering == 2) {	// fixed
        _gear_max_steer = 0.0f;
    }
}

std::string LandingGear::comment()
{
    std::stringstream file;

    _aircraft->_retractable = _retractable;
    _aircraft->_steering = _steering;
    file << "    gear type:     ";
    if (_taildragger) {
       file << "taildragger" << std::endl;
    } else {
       file << "tricycle" << std::endl;
    }

    file << "    steering type: ";
    if (_steering == 0) file << "steering" << std::endl;
    else if (_steering == 1) file << "castering" << std::endl;
    else file << "fixed" << std::endl;

    file << "    retractable?:  " << (_retractable ? "yes" : "no") << std::endl;

    return file.str();
}

std::string LandingGear::fdm()
{
    bool& convert = _aircraft->_metric;
    std::stringstream file;

    bool glider = (_aircraft->_atype == LIGHT && _aircraft->_engines == 0) ? true : false;

    file.precision(2);
    file.flags(std::ios::right);
    file << std::fixed << std::showpoint;
    file << " <ground_reactions>" << std::endl;
    file << std::endl;

    file << "  <contact type=\"BOGEY\" name=\"" << (_taildragger ? "TAIL" : "NOSE") << "\">" << std::endl;
    file << "    <location unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
    file << "      <x> " << std::setw(8) << Param::get((_gear_loc[_taildragger ? TAIL : NOSE][X])*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
    file << "      <y> " << std::setw(8) << Param::get((_gear_loc[_taildragger ? TAIL : NOSE][Y])*INCH_TO_FEET, LENGTH, convert) << " </y>" << std::endl;
    file << "      <z> " << std::setw(8) << Param::get((_gear_loc[_taildragger ? TAIL : NOSE][Z])*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
    file << "    </location>" << std::endl;
    file << "    <static_friction>  " << _gear_static << " </static_friction>" << std::endl;
    file << "    <dynamic_friction> " << _gear_dynamic << " </dynamic_friction>" << std::endl;
    file << "    <rolling_friction> " << _gear_rolling << " </rolling_friction>" << std::endl;
    file << "    <spring_coeff  unit=\"" << Param::get_unit(true, SPRING, convert) << "\">     " << Param::get(_gear_spring[_taildragger ? TAIL : NOSE], SPRING, convert) << " </spring_coeff>" << std::endl;
    file << "    <damping_coeff unit=\"" << Param::get_unit(true, DAMPING, convert) << "\"> " << Param::get(_gear_damp[_taildragger ? TAIL : NOSE], DAMPING, convert) << " </damping_coeff>" << std::endl;
    file << "    <max_steer unit=\"DEG\"> " << _gear_max_steer << " </max_steer>" << std::endl;
    file << "    <brake_group> NONE </brake_group>" << std::endl;
    file << "    <retractable> " << _retractable << " </retractable>" << std::endl;
    file << "  </contact>" << std::endl;
    file << std::endl;
    file << "  <contact type=\"BOGEY\" name=\"LEFT_MAIN\">" << std::endl;
    file << "    <location unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
    file << "      <x> " << std::setw(8) << Param::get(_gear_loc[MAIN][X]*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
    file << "      <y> " << std::setw(8) << Param::get(-_gear_loc[MAIN][Y]*INCH_TO_FEET, LENGTH, convert) << " </y>" << std::endl;
    file << "      <z> " << std::setw(8) << Param::get(_gear_loc[MAIN][Z]*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
    file << "    </location>" << std::endl;
    file << "    <static_friction>  " << _gear_static << " </static_friction>" << std::endl;
    file << "    <dynamic_friction> " << _gear_dynamic << " </dynamic_friction>" << std::endl;
    file << "    <rolling_friction> " << _gear_rolling << " </rolling_friction>" << std::endl;
    file << "    <spring_coeff  unit=\"" << Param::get_unit(true, SPRING, convert) << "\">     " << Param::get(_gear_spring[MAIN], SPRING, convert) << " </spring_coeff>" << std::endl;
    file << "    <damping_coeff unit=\"" << Param::get_unit(true, DAMPING, convert) << "\"> " << Param::get(_gear_damp[MAIN], DAMPING, convert) << " </damping_coeff>" << std::endl;
    file << "    <max_steer unit=\"DEG\">0</max_steer>" << std::endl;
    file << "    <brake_group> " << (glider ? "NONE" : "LEFT") << " </brake_group>" << std::endl;
    file << "    <retractable> " << _retractable << " </retractable>" << std::endl;
    file << "  </contact>" << std::endl;
    file << std::endl;
    file << "  <contact type=\"BOGEY\" name=\"RIGHT_MAIN\">" << std::endl;
    file << "    <location unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
    file << "      <x> " << std::setw(8) << Param::get(_gear_loc[MAIN][X]*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
    file << "      <y> " << std::setw(8) << Param::get(_gear_loc[MAIN][Y]*INCH_TO_FEET, LENGTH, convert) << " </y>" << std::endl;
    file << "      <z> " << std::setw(8) << Param::get(_gear_loc[MAIN][Z]*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
    file << "    </location>" << std::endl;
    file << "    <static_friction>  " << _gear_static << " </static_friction>" << std::endl;
    file << "    <dynamic_friction> " << _gear_dynamic << " </dynamic_friction>" << std::endl;
    file << "    <rolling_friction> " << _gear_rolling << " </rolling_friction>" << std::endl;
    file << "    <spring_coeff  unit=\"" << Param::get_unit(true, SPRING, convert) << "\">     " << Param::get(_gear_spring[MAIN], SPRING, convert) << " </spring_coeff>" << std::endl;
    file << "    <damping_coeff unit=\"" << Param::get_unit(true, DAMPING, convert) << "\"> " << Param::get(_gear_damp[MAIN], DAMPING, convert) << " </damping_coeff>" << std::endl;
    file << "    <max_steer unit=\"DEG\">0</max_steer>" << std::endl;
    file << "    <brake_group> " << (glider ? "NONE" : "RIGHT") << " </brake_group>" << std::endl;
    file << "    <retractable> " << _retractable << " </retractable>" << std::endl;
    file << "  </contact>" << std::endl;
    file << std::endl;
    file << "  <contact type=\"STRUCTURE\" name=\"LEFT_WING\">" << std::endl;
    file << "    <location unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
    file << "     <x> " << std::setw(8) << Param::get(_cg_loc[X]*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << Param::get(-_aircraft->_wing.span/2, LENGTH, convert) << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << Param::get(_cg_loc[Z]*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
    file << "    </location>" << std::endl;
    file << "   <static_friction>  1 </static_friction>" << std::endl;
    file << "   <dynamic_friction> 1 </dynamic_friction>" << std::endl;
    file << "    <spring_coeff  unit=\"" << Param::get_unit(true, SPRING, convert) << "\">     " << Param::get(_gear_spring[MAIN], SPRING, convert) << " </spring_coeff>" << std::endl;
    file << "    <damping_coeff unit=\"" << Param::get_unit(true, DAMPING, convert) << "\"> " << Param::get(_gear_damp[MAIN], DAMPING, convert) << " </damping_coeff>" << std::endl;
    file << "  </contact>" << std::endl;
    file << std::endl;
    file << "  <contact type=\"STRUCTURE\" name=\"RIGHT_WING\">" << std::endl;
    file << "    <location unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
    file << "     <x> " << std::setw(8) << Param::get(_cg_loc[X]*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << Param::get(_aircraft->_wing.span/2, LENGTH, convert) << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << Param::get(_cg_loc[Z]*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
    file << "    </location>" << std::endl;
    file << "   <static_friction>  1 </static_friction>" << std::endl;
    file << "   <dynamic_friction> 1 </dynamic_friction>" << std::endl;
    file << "    <spring_coeff  unit=\"" << Param::get_unit(true, SPRING, convert) << "\">     " << Param::get(_gear_spring[MAIN], SPRING, convert) << " </spring_coeff>" << std::endl;
    file << "    <damping_coeff unit=\"" << Param::get_unit(true, DAMPING, convert) << "\"> " << Param::get(_gear_damp[MAIN], DAMPING, convert) << " </damping_coeff>" << std::endl;
    file << "  </contact>" << std::endl;
    file << std::endl;
    file << " </ground_reactions>" << std::endl;

    return file.str();
}

std::string LandingGear::json(const float cg_loc[3])
{
    std::stringstream file;

    file.precision(1);
    file.flags(std::ios::left);
    file << std::fixed << std::showpoint;

    std::string param = "  \"gear\"";
    file << std::setw(12) << param << ": [ " << std::endl;

    // NOSE, TAIL
    file << "  {" << std::endl;
    param  = "    \"pos\"";
    file << std::setw(14) << param << ": [ "
               << _gear_loc[_taildragger ? TAIL : NOSE][X]-cg_loc[X] << ", "
               << _gear_loc[_taildragger ? TAIL : NOSE][Y]-cg_loc[Y] << ", "
               << _gear_loc[_taildragger ? TAIL : NOSE][Z]-cg_loc[Z] << " ],"
               << std::endl;

    param  = "    \"spring\"";
    file << std::setw(14) << param << ": "
         << _gear_spring[_taildragger ? TAIL : NOSE]  << ", " << std::endl;

    param  = "    \"damp\"";
    file << std::setw(14) << param << ": "
         << _gear_damp[_taildragger ? TAIL : NOSE]  << std::endl;
    file << "  }," << std::endl;

    // LEFT MAIN
    file << "  {" << std::endl;
    param  = "    \"pos\"";
    file << std::setw(14) << param << ": [ "
               << _gear_loc[MAIN][X]-cg_loc[X] << ", "
               << -_gear_loc[MAIN][Y]-cg_loc[Y] << ", "
               << _gear_loc[MAIN][Z]-cg_loc[Z] << " ],"
               << std::endl;

    param  = "    \"spring\"";
    file << std::setw(14) << param << ": "
         << _gear_spring[MAIN]  << ", " << std::endl;

    param  = "    \"damp\"";
    file << std::setw(14) << param << ": "
         << _gear_damp[MAIN]  << std::endl;
    file << "  }," << std::endl;

    // RIGHT MAIN
    file << "  {" << std::endl;
    param  = "    \"pos\"";
    file << std::setw(14) << param << ": [ "
               << _gear_loc[MAIN][X]-cg_loc[X] << ", "
               << _gear_loc[MAIN][Y]-cg_loc[Y] << ", "
               << _gear_loc[MAIN][Z]-cg_loc[Z] << " ],"
               << std::endl;

    param  = "    \"spring\"";
    file << std::setw(14) << param << ": "
         << _gear_spring[MAIN]  << ", " << std::endl;

    param  = "    \"damp\"";
    file << std::setw(14) << param << ": "
         << _gear_damp[MAIN]  << std::endl;
    file << "  } ]";

    return file.str();
}

std::string LandingGear::system()
{
    std::stringstream file;

    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "   <switch name=\"fcs/gear-no-wow\">" << std::endl;
    file << "     <default value=\"1\"/>" << std::endl;
    file << "     <test logic=\"AND\" value=\"0\">" << std::endl;
    file << "         gear/unit[1]/WOW eq 1" << std::endl;
    file << "         gear/unit[2]/WOW eq 1" << std::endl;
    file << "     </test>" << std::endl;
    file << "   </switch>" << std::endl;

    if (_retractable)
    {
        file <<  std::endl;
        file << "   <kinematic name=\"" + _description[_subtype] + " Control\">" << std::endl;
        file << "     <input>gear/gear-cmd-norm</input>" << std::endl;
        file << "     <traverse>" << std::endl;
        file << "       <setting>" << std::endl;
        file << "          <position> 0 </position>" << std::endl;
        file << "          <time>     0 </time>" << std::endl;
        file << "       </setting>" << std::endl;
        file << "       <setting>" << std::endl;
        file << "          <position> 1 </position>" << std::endl;
        file << "          <time>     5 </time>" << std::endl;
        file << "       </setting>" << std::endl;
        file << "     </traverse>" << std::endl;
        file << "     <output>gear/gear-pos-norm</output>" << std::endl;
        file << "   </kinematic>" << std::endl;
    }
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string LandingGear::drag()
{
    std::stringstream file;
    float CDgear;

    if(_retractable) {
        CDgear = _CDgear_t[_aircraft->_atype][_aircraft->_engines];
    } else {
        CDgear = _CDfixed_gear_t[_aircraft->_atype][_aircraft->_engines];
    }

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Drag_gear\">" << std::endl;
    file << "       <description>Drag due to gear</description>" << std::endl;
    file << "         <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    if (_retractable) {
        file << "           <property>gear/gear-pos-norm</property>" << std::endl;
    }
    file << "           <value> " << (CDgear) << " </value>" << std::endl;
    file << "         </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

// ----------------------------------------------------------------------------

float const LandingGear::_CDgear_t[MAX_AIRCRAFT][5] =
{
    { 0.012f, 0.030f, 0.030f, 0.030f, 0.030f },		// LIGHT
    { 0.030f, 0.030f, 0.030f, 0.030f, 0.030f },		// PERFORMANCE
    { 0.020f, 0.020f, 0.020f, 0.020f, 0.020f },		// FIGHTER
    { 0.015f, 0.015f, 0.015f, 0.013f, 0.011f },		// JET_TRANSPORT
    { 0.023f, 0.023f, 0.023f, 0.023f, 0.023f }		// PROP_TRANSPORT
};

float const LandingGear::_CDfixed_gear_t[MAX_AIRCRAFT][5] =
{
    { 0.002f, 0.004f, 0.004f, 0.004f, 0.004f },		// LIGHT
    { 0.004f, 0.004f, 0.004f, 0.004f, 0.004f },		// PERFORMANCE
    { 0.005f, 0.005f, 0.005f, 0.005f, 0.005f },		// FIGHTER
    { 0.002f, 0.002f, 0.002f, 0.002f, 0.002f },		// JET_TRANSPORT
    { 0.003f, 0.003f, 0.003f, 0.003f, 0.003f }
};

} /* namespace Aeromatic */

