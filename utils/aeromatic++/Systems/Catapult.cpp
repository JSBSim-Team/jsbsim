// Catapult.cpp -- Implements a Catapult system.
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

#include <Aircraft.h>
#include "Systems.h"

namespace Aeromatic
{

std::string Catapult::system()
{
    std::stringstream file;

    file << "  <property value=\"0\">systems/catapult/cat-pos-norm</property>" << std::endl;
    file << "  <property value=\"0\">systems/catapult/cat-launch-cmd</property>" << std::endl;
    file << "  <property value=\"0\">systems/catapult/cat-force</property>" << std::endl;
    file << std::endl;
    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "    <switch name=\"" + _description[_subtype] + " Command\">" << std::endl;
    file << "      <default value=\"0\"/>" << std::endl;
    file << "      <test logic=\"AND\" value=\"0\">" << std::endl;
    file << "          systems/catapult/cat-pos-norm gt 0.999" << std::endl;
    file << "      </test>" << std::endl;
    file << "      <test logic=\"AND\" value=\"1\">" << std::endl;
    file << "          systems/catapult/cat-launch-cmd == 1" << std::endl;
    file << "      </test>" << std::endl;
    file << "      <output>systems/catapult/cat-launch-cmd</output>" << std::endl;
    file << "    </switch>" << std::endl;
    file << std::endl;
    file << "   <kinematic name=\"" + _description[_subtype] + " Timer\">" << std::endl;
    file << "     <input>systems/catapult/cat-launch-cmd</input>" << std::endl;
    file << "     <traverse>" << std::endl;
    file << "       <setting>" << std::endl;
    file << "          <position> 0 </position>" << std::endl;
    file << "          <time>     0 </time>" << std::endl;
    file << "       </setting>" << std::endl;
    file << "       <setting>" << std::endl;
    file << "          <position> 1 </position>" << std::endl;
    file << "          <time>     2.7 </time>" << std::endl;
    file << "       </setting>" << std::endl;
    file << "     </traverse>" << std::endl;
    file << "     <output>systems/catapult/cat-pos-norm</output>" << std::endl;
    file << "   </kinematic>" << std::endl;
    file << std::endl;
    file << "   <pure_gain name=\"" + _description[_subtype] + " Force\">" << std::endl;
    file << "     <input>inertia/weight-lbs</input>" << std::endl;
    file << "     <gain>3</gain>" << std::endl;
    file << "     <output>systems/catapult/cat-force</output>" << std::endl;
    file << "   </pure_gain>" << std::endl;
    file << std::endl;
    file << "   <switch name=\"" + _description[_subtype] + " Final\">" << std::endl;
    file << "     <default value=\"0\"/>" << std::endl;
    file << "     <test logic=\"AND\" value=\"systems/catapult/cat-force\">" << std::endl;
    file << "         systems/catapult/cat-launch-cmd == 1" << std::endl;
    file << "         systems/catapult/cat-pos-norm lt 0.999" << std::endl;
    file << "         systems/catapult/cat-pos-norm gt 0.0" << std::endl;
    file << "         gear/unit[0]/WOW ne 0" << std::endl;
    file << "     </test>" << std::endl;
    file << "     <output>external_reactions/catapult/magnitude</output>" << std::endl;
    file << "   </switch>" << std::endl;
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string Catapult::external_force()
{
    std::stringstream file;

    file << "  <force name=\"catapult\" frame=\"BODY\">" << std::endl;
    file << "   <location unit=\"" << _aircraft->_geometry["length"]->get_unit() << "\">" << std::endl;
    file << "    <x> " << _aircraft->_geometry["length"]->get(0.13f) << " </x>" << std::endl;
    file << "    <y> 0 </y>" << std::endl;
    file << "    <z> " << _aircraft->_geometry["length"]->get(-0.12f) << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <direction>" << std::endl;
    file << "    <x> 1 </x>" << std::endl;
    file << "    <y> 0 </y>" << std::endl;
    file << "    <z> 0 </z>" << std::endl;
    file << "   </direction>" << std::endl;
    file << "  </force>" << std::endl;

    return file.str();
}

} /* namespace Aeromatic */

