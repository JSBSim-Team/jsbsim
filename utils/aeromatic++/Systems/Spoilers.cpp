// Spoilers.cpp -- Implements a Spoilers system.
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

std::string Spoilers::system()
{
    std::stringstream file;

    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "   <kinematic name=\"" + _description[_subtype] + " Control\">" << std::endl;
    file << "     <input>fcs/spoiler-cmd-norm</input>" << std::endl;
    file << "     <traverse>" << std::endl;
    file << "       <setting>" << std::endl;
    file << "          <position> 0 </position>" << std::endl;
    file << "          <time>     0 </time>" << std::endl;
    file << "       </setting>" << std::endl;
    file << "       <setting>" << std::endl;
    file << "          <position> 1 </position>" << std::endl;
    file << "          <time>     1 </time>" << std::endl;
    file << "       </setting>" << std::endl;
    file << "     </traverse>" << std::endl;
    file << "     <output>fcs/spoilers-pos-norm</output>" << std::endl;
    file << "   </kinematic>" << std::endl;
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string Spoilers::lift()
{
    float dCLspoilers = _dCLspoilers_t[_aircraft->_atype][_aircraft->_engines];
    std::stringstream file;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Lift_spoilers\">" << std::endl;
    file << "       <description>Delta Lift due to spoilers</description>" << std::endl;
    file << "       <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>fcs/spoilers-pos-norm</property>" << std::endl;
    file << "           <value> " << (dCLspoilers) << " </value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string Spoilers::drag()
{
    float CDspoilers = _aircraft->_CD0;
    std::stringstream file;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Drag_spoilers\">" << std::endl;
    file << "       <description>Drag due to spoilers</description>" << std::endl;
    file << "         <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>fcs/spoilers-pos-norm</property>" << std::endl;
    file << "           <value> " << (CDspoilers) << " </value>" << std::endl;
    file << "         </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

std::string Spoilers::roll()
{
    std::stringstream file;

    if (_differential)
    {
        float Cldsp = 0.24f;

        file.precision(4);
        file.flags(std::ios::right);
        file << std::fixed << std::showpoint;
        file << "    <function name=\"aero/moment/Roll_spoilers\">" << std::endl;
        file << "       <description>Roll moment due to spoilers</description>" << std::endl;
        file << "       <product>" << std::endl;
        file << "          <property>aero/qbar-psf</property>" << std::endl;
        file << "          <property>metrics/Sw-sqft</property>" << std::endl;
        file << "          <property>metrics/bw-ft</property>" << std::endl;
        file << "          <property>fcs/spoilers-pos-norm</property>" << std::endl;
        file << "          <property>fcs/left-aileron-pos-rad</property>" << std::endl;
        file << "          <table>" << std::endl;
        file << "            <independentVar lookup=\"row\">aero/alpha-rad</independentVar>" << std::endl;
        file << "            <tableData>" << std::endl;
        file << "             -0.175   " << (0.0) << std::endl;
        file << "              0.0     " << (Cldsp) << std::endl;
        file << "              0.175   " << (0.0) << std::endl;
        file << "            </tableData>" << std::endl;
        file << "          </table>" << std::endl;
        file << "       </product>" << std::endl;
        file << "    </function>" << std::endl;
    }

    return file.str();
}

// ----------------------------------------------------------------------------

float const Spoilers::_dCLspoilers_t[MAX_AIRCRAFT][5] =
{
    { -0.05f,  0.00f,  0.00f,  0.00f,  0.00f },		// LIGHT
    {  0.00f,  0.00f,  0.00f,  0.00f,  0.00f },		// PERFORMANCE
    {  0.00f,  0.00f,  0.00f,  0.00f,  0.00f },		// FIGHTER
    { -0.10f, -0.10f, -0.10f, -0.09f, -0.08f },		// JET_TRANSPORT
    {  0.00f,  0.00f,  0.00f,  0.00f,  0.00f },		// PROP_TRANSPORT
};

 
} /* namespace Aeromatic */

