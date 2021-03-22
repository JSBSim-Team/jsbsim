// Flaps.cpp -- Implements the Flaps system.
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

#include <math.h>
#include <sstream>
#include <iomanip>

#include <Aircraft.h>
#include "Systems.h"

namespace Aeromatic
{

void Flaps::set(const float cg_loc[3])
{
    // http://adg.stanford.edu/aa241/highlift/clmaxest.html
    // K(sweep) is an empirically-derived sweep-correction factor.
    float sweep = _aircraft->_wing.sweep * DEG_TO_RAD;
    float sweep_le = _aircraft->_wing.sweep_le * DEG_TO_RAD;
    float csweep_te = cosf(sweep - (sweep_le-sweep));
    _K = (1.0f - 0.08f*powf(csweep_te, 2.0f))*powf(csweep_te, 0.75f);
}

std::string Flaps::system()
{
    std::stringstream file;

    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "   <kinematic name=\"" + _description[_subtype] + " Control\">" << std::endl;
    file << "    <input>fcs/flap-cmd-norm</input>" << std::endl;
    file << "    <traverse>" << std::endl;
    file << "      <setting>" << std::endl;
    file << "         <position>  0 </position>" << std::endl;
    file << "         <time>      0 </time>" << std::endl;
    file << "      </setting>" << std::endl;
    file << "      <setting>" << std::endl;
    file << "         <position> 15 </position>" << std::endl;
    file << "         <time>      4 </time>" << std::endl;
    file << "      </setting>" << std::endl;
    file << "      <setting>" << std::endl;
    file << "         <position> 30 </position>" << std::endl;
    file << "         <time>      3 </time>" << std::endl;
    file << "      </setting>" << std::endl;
    file << "    </traverse>" << std::endl;
    file << "    <output>fcs/flap-pos-deg</output>" << std::endl;
    file << "   </kinematic>" << std::endl;
    file << std::endl;
    file << "   <aerosurface_scale name=\"Flap Normalization\">" << std::endl;
    file << "    <input>fcs/flap-pos-deg</input>" << std::endl;
    file << "    <domain>" << std::endl;
    file << "      <min>  0 </min>" << std::endl;
    file << "      <max> 30 </max>" << std::endl;
    file << "    </domain>" << std::endl;
    file << "    <range>" << std::endl;
    file << "      <min> 0 </min>" << std::endl;
    file << "      <max> 1 </max>" << std::endl;
    file << "    </range>" << std::endl;
    file << "    <output>fcs/flap-pos-norm</output>" << std::endl;
    file << "   </aerosurface_scale>" << std::endl;
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string Flaps::lift()
{
    float dCLflaps = _dCLflaps_t[_aircraft->_atype][_aircraft->_engines];
    std::stringstream file;

    file.precision(4);
    file << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Lift_flap\">" << std::endl;
    file << "       <description>Delta Lift due to flaps</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>fcs/flap-pos-deg</property>" << std::endl;
    file << "           <value> " << (_K*dCLflaps/30.0f) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>\n" << std::endl;

    return file.str();
}

std::string Flaps::drag()
{
    float CDflaps = _CDflaps_t[_aircraft->_atype][_aircraft->_engines];
    std::stringstream file;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Drag_flap\">" << std::endl;
    file << "       <description>Drag due to flaps</description>" << std::endl;
    file << "         <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>fcs/flap-pos-deg</property>" << std::endl;
    file << "           <value> " << (_K*CDflaps/30.0f) << " </value>"<< std::endl;
    file << "         </product>" << std::endl;
    file << "    </function>\n" << std::endl;


    return file.str();
}

#if 0
std::string Flaps:pitch()
{
    float dCLflaps = _dCLflaps_t[_aircraft->_atype][_aircraft->_engines];

    file.precision(4);
    file << std::fixed << std::showpoint;
    file << "    <function name=\"aero/moment/Pitch_flap\">" << std::endl;
    file << "       <description>Pitch moment  due to flaps</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>metrics/cbarw-ft</property>" << std::endl;
    file << "           <property>fcs/flap-pos-deg</property>" << std::endl;
    file << "           <value> " << -0.327f*(_K*dCLflaps/30.0f) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>\n" << std::endl;

    return file.str();
}
#endif
 
// ----------------------------------------------------------------------------

float const Flaps::_dCLflaps_t[MAX_AIRCRAFT][5] =
{
    { 0.20f, 0.40f, 0.40f, 0.40f, 0.40f },		// LIGHT
    { 0.30f, 0.30f, 0.30f, 0.30f, 0.30f },		// PERFORMANCE
    { 0.35f, 0.35f, 0.35f, 0.35f, 0.35f },		// FIGHTER
    { 1.50f, 1.50f, 1.50f, 1.50f, 1.50f },		// JET_TRANSPORT
    { 0.60f, 0.60f, 0.60f, 0.60f, 0.60f }		// PROP_TRANSPORT
};

float const Flaps::_CDflaps_t[MAX_AIRCRAFT][5] =
{
    { 0.024f, 0.030f, 0.039f, 0.039f, 0.039f },		// LIGHT
    { 0.040f, 0.040f, 0.040f, 0.040f, 0.040f },		// PERFORMANCE
    { 0.080f, 0.080f, 0.075f, 0.075f, 0.075f },		// FIGHTER
    { 0.059f, 0.059f, 0.059f, 0.057f, 0.055f },		// JET_TRANSPORT
    { 0.035f, 0.035f, 0.035f, 0.035f, 0.035f }		// PROP_TRANSPORT
};

} /* namespace Aeromatic */

