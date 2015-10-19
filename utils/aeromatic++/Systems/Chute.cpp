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
    { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },		// LIGHT
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f },		// PERFORMANCE
    { 0.90f, 0.90f, 0.90f, 0.90f, 0.90f },		// FIGHTER
    { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f },		// JET_TRANSPORT
    { 0.00f, 0.00f, 0.00f, 0.00f, 0.00f }		// PROP_TRANSPORT
};

// LIGHT chutes are rescue chutes of 55'or 65'
// FIGHTER chutes are drag chutes of 24' (+ shaped)
// TRANSPORT chutes are 2 or 3 FIGHTER drag chutes
float const Chute::_ChuteArea_t[MAX_AIRCRAFT][5] =
{
    { 2375.f, 2375.f, 3320.f, 3320.f, 3320.f },		// LIGHT
    {    0.f,    0.f,    0.f,    0.f,    0.f },		// PERFORMANCE
    {  452.f,  452.f,  452.f,  452.f,  452.f },		// FIGHTER
    {  900.f,  900.f, 1200.f, 1200.f, 1200.f },		// JET_TRANSPORT
    {    0.f,    0.f,    0.f,    0.f,    0.f }
};
 
} /* namespace Aeromatic */

