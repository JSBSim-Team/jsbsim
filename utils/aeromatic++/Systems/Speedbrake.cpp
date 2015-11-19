// Speedbrake.cpp -- Implements a Speedbrake system.
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

std::string Speedbrake::system()
{
    std::stringstream file;

    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "   <kinematic name=\"" + _description[_subtype] + " Control\">" << std::endl;
    file << "     <input>fcs/speedbrake-cmd-norm</input>" << std::endl;
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
    file << "     <output>fcs/speedbrake-pos-norm</output>" << std::endl;
    file << "   </kinematic>" << std::endl;
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string Speedbrake::drag()
{
    float CDspeedbrake = _CDspeedbrake_t[_aircraft->_atype][_aircraft->_engines];
    std::stringstream file;

    file << std::setprecision(4) << std::fixed << std::showpoint;
    file << "    <function name=\"aero/force/Drag_speedbrake\">" << std::endl;
    file << "       <description>Drag due to speedbrakes</description>" << std::endl;
    file << "         <product>" << std::endl;
    file << "           <property>aero/qbar-psf</property>" << std::endl;
    file << "           <property>metrics/Sw-sqft</property>" << std::endl;
    file << "           <property>fcs/speedbrake-pos-norm</property>" << std::endl;
    file << "           <value> " << (CDspeedbrake) << " </value>" << std::endl;
    file << "         </product>" << std::endl;
    file << "    </function>" << std::endl;

    return file.str();
}

// ----------------------------------------------------------------------------

float const Speedbrake::_CDspeedbrake_t[MAX_AIRCRAFT][5] =
{
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f },		// LIGHT
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f },		// PERFORMANCE
    { 0.06f, 0.06f, 0.08f, 0.08f, 0.08f },		// FIGHTER
    { 0.12f, 0.12f, 0.12f, 0.12f, 0.12f },		// JET_TRANSPORT
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f }		// PROP_TRANSPORT
};
 
} /* namespace Aeromatic */

