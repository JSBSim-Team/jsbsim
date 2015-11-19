// ThrustReverse.cpp -- Implements a Thrust Reverser system.
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

std::string ThrustReverse::system()
{
    std::stringstream file;

    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;

    file << "    <pure_gain name=\"" << _description[_subtype] << " Position\">" << std::endl;
    file << "      <input>systems/reverser/command</input>" << std::endl;
    file << "      <gain>2.0</gain>" << std::endl;
    file << "      <output>systems/reverser/reverser-pos-norm</output>" << std::endl;
    file << "    </pure_gain>" << std::endl;

    for (unsigned i=0; i<_aircraft->_no_engines; ++i)
    {
        file << std::endl;
        file << "    <kinematic name=\"" << _description[_subtype] << " Control Engine " << i << "\">" << std::endl;
        file << "      <input>systems/reverser/reverser-pos-norm</input>" << std::endl;
        file << "      <traverse>" << std::endl;
        file << "        <setting>" << std::endl;
        file << "          <position>0</position>" << std::endl;
        file << "          <time>0</time>" << std::endl;
        file << "        </setting>" << std::endl;
        file << "        <setting>" << std::endl;
        file << "          <position>2.0</position>" << std::endl;
        file << "          <time>1.0</time>" << std::endl;
        file << "        </setting>" << std::endl;
        file << "      </traverse>" << std::endl;
        file << "      <output>propulsion/engine[" << i << "]/reverser-angle-rad</output>" << std::endl;
        file << "    </kinematic>" << std::endl;
    }

    file << "  </channel>" << std::endl;

    return file.str();
}

} /* namespace Aeromatic */

