// Chute.cpp -- Implements a Chute system.
//
// Based on Aeromatic2 PHP code by David P. Culp
// Started June 2003
//
// C++-ified and modulized by Erik Hofman, started October 2015.
//
// Copyright (C) 2003, David P. Culp <davidculp2@comcast.net>
// Copyright (C) 2015 Erik Hofman <erik@ehofman.com>
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
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <sstream>

#include <Aircraft.h>
#include "Systems.h"

namespace Aeromatic
{

std::string Chute::system()
{
    std::stringstream file;

    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "   <kinematic name=\"" + _description[_subtype] + " Control\">" << std::endl;
    file << "     <input>fcs/chute-cmd-norm</input>" << std::endl;
    file << "     <traverse>" << std::endl;
    file << "       <setting>" << std::endl;
    file << "          <position> 0 </position>" << std::endl;
    file << "          <time>     0 </time>" << std::endl;
    file << "       </setting>" << std::endl;
    file << "       <setting>" << std::endl;
    file << "          <position> 1 </position>" << std::endl;
    file << "          <time>     1.5 </time>" << std::endl;
    file << "       </setting>" << std::endl;
    file << "     </traverse>" << std::endl;
    file << "     <output>fcs/parachute_reef_pos_norm</output>" << std::endl;
    file << "   </kinematic>" << std::endl;
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string Chute::external_force()
{
    float CDchute = _CDchute_t[_aircraft->_atype][_aircraft->_engines];
    float Area = _ChuteArea_t[_aircraft->_atype][_aircraft->_engines];
    std::stringstream file;

    file << "  <property value=\"0\">fcs/parachute_reef_pos_norm</property>" << std::endl;
    file << std::endl;
    file << "  <force name=\"parachute\" frame=\"WIND\">" << std::endl;
    file << "   <function>" << std::endl;
    file << "    <product>" << std::endl;
    file << "     <p> aero/qbar-psf </p>" << std::endl;
    file << "     <p> fcs/parachute_reef_pos_norm </p>" << std::endl;
    file << "     <v> " << CDchute << " </v>" << std::endl;
    file << "     <v> " << Area << " </v>" << std::endl;
    file << "    </product>" << std::endl;
    file << "   </function>" << std::endl;
    file << "   <location unit=\"FT\">" << std::endl;
    if (_aircraft->_atype < FIGHTER) {
        file << "    <x> 0 </x>" << std::endl;
    } else {
        file << "    <x> " << (_aircraft->_length * 0.91f) << " </x>" << std::endl;
    }
    file << "    <y> 0 </y>" << std::endl;
    file << "    <z> 0 </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <direction>" << std::endl;
    file << "    <x>-1</x>" << std::endl;
    file << "    <y> 0 </y>" << std::endl;
    file << "    <z> 0 </z>" << std::endl;
    file << "   </direction>" << std::endl;
    file << "  </force>" << std::endl;

    return file.str();
}

// ----------------------------------------------------------------------------

float const Chute::_CDchute_t[MAX_AIRCRAFT][5] =
{
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f },		// LIGHT
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f },		// PERFORMANCE
    { 0.75f, 0.75f, 0.90f, 0.90f, 0.90f },		// FIGHTER
    { 1.50f, 1.50f, 1.50f, 2.20f, 2.20f },		// JET_TRANSPORT
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f }		// PROP_TRANSPORT
};

float const Chute::_ChuteArea_t[MAX_AIRCRAFT][5] =
{
    { 190.f, 190.f, 265.f, 265.f, 265.f },		// LIGHT
    { 190.f, 190.f, 265.f, 265.f, 265.f },		// PERFORMANCE
    { 265.f, 265.f, 265.f, 256.f, 256.f },		// FIGHTER
    { 500.f, 500.f, 570.f, 570.f, 570.f },		// JET_TRANSPORT
    {   0.f,   0.f,   0.f,   0.f,   0.f }
};
 
} /* namespace Aeromatic */

