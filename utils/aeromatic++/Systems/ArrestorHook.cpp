// ArrestorHook.cpp -- Implements a ArrestorHook system.
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

std::string ArrestorHook::system()
{
    std::stringstream file;

    file << "  <property>systems/tailhook-cmd-norm</property>" << std::endl;
    file << "  <property value=\"6.75\">systems/hook/tailhook-length-ft</property>" << std::endl;
    file << "  <property value=\"-18.0\">systems/hook/tailhook-pos-min-deg</property>" << std::endl;
    file << "  <property value=\"42.0\">systems/hook/tailhook-pos-max-deg</property>" << std::endl;
    file << "  <property value=\"196.0\">systems/hook/tailhook-offset-x-in</property>" << std::endl;
    file << "  <property value=  \"0.0\">systems/hook/tailhook-offset-y-in</property>" << std::endl;
    file << "  <property value=\"-16.0\">systems/hook/tailhook-offset-z-in</property>" << std::endl;
    file << "  <property value=\"-18.0\">systems/hook/tailhook-pos-deg</property>" << std::endl;
    file << std::endl;
    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "   <kinematic name=\"" + _description[_subtype] + " Control\">" << std::endl;
    file << "     <input>systems/tailhook-cmd-norm</input>" << std::endl;
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
    file << "     <output>systems/hook/tailhook-pos-norm</output>" << std::endl;
    file << "   </kinematic>" << std::endl;
    file << std::endl;
    file << "   <switch name=\"systems/hook/ready\">" << std::endl;
    file << "     <default value=\"0\"/>" << std::endl;
    file << "     <test logic=\"AND\" value=\"1\">" << std::endl;
    file << "         systems/hook/tailhook-pos-norm gt 0.99" << std::endl;
    file << "     </test>" << std::endl;
    file << "   </switch>" << std::endl;
    file << std::endl;
    file << "   <scheduled_gain name=\"systems/hook/hook-decel-multiplier\">" << std::endl;
    file << "    <input>systems/hook/ready</input>" << std::endl;
    file << "    <table>" << std::endl;
    file << "      <tableData>" << std::endl;
    file << "         0     0.00" << std::endl;
    file << "         1     0.00" << std::endl;
    file << "         80    2.20" << std::endl;
    file << "      </tableData>" << std::endl;
    file << "    </table>" << std::endl;
    file << "   </scheduled_gain>" << std::endl;
    file << std::endl;
    file << "   <pure_gain name=\"systems/hook/hook-decel-force\">" << std::endl;
    file << "      <input>systems/hook/hook-decel-multiplier</input>" << std::endl;
    file << "     <gain>inertia/weight-lbs</gain>" << std::endl;
    file << "   </pure_gain>" << std::endl;
    file << std::endl;
    file << "   <summer name=\"systems/hook/force\">" << std::endl;
    file << "     <input>systems/hook/hook-decel-force</input>" << std::endl;
    file << "     <input>forces/fbx-prop-lbs</input>" << std::endl;
    file << "   </summer>" << std::endl;
    file << std::endl;
    file << "   <fcs_function name=\"systems/hook/animation-norm\">" << std::endl;
    file << "     <function>" << std::endl;
    file << "       <product>" << std::endl;
    file << "         <sum>" << std::endl;
    file << "          <property>systems/hook/tailhook-pos-deg</property>" << std::endl;
    file << "          <value>8</value>" << std::endl;
    file << "         </sum>" << std::endl;
    file << "         <value>0.02</value>" << std::endl;
    file << "       </product>" << std::endl;
    file << "     </function>" << std::endl;
    file << "     <output>gear/tailhook-pos-norm</output>" << std::endl;
    file << "   </fcs_function>" << std::endl;
    file << "  </channel>" << std::endl;

    return file.str();
}

std::string ArrestorHook::external_force()
{
    std::stringstream file;

    file << std::endl;
    file << "  <force name=\"hook\" frame=\"BODY\">" << std::endl;
    file << "   <location unit=\"FT\">" << std::endl;
    file << "    <x> " << (_aircraft->_length * 0.91f) << " </x>" << std::endl;
    file << "    <y> 0 </y>" << std::endl;
    file << "    <z> " << _cg_loc_x << "</z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <direction>" << std::endl;
    file << "    <x>-0.9995</x>" << std::endl;
    file << "    <y> 0 </y>" << std::endl;
    file << "    <z> 0.01 </z>" << std::endl;
    file << "   </direction>" << std::endl;
    file << "  </force>" << std::endl;

    return file.str();
}

} /* namespace Aeromatic */

