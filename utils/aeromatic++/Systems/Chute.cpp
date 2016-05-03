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

std::string Chute::system()
{
    std::stringstream file;

    file << "  <property value=\"0\">systems/chute/chute-cmd-norm</property>" << std::endl;
    if (_aircraft->_atype >= FIGHTER) {
        file << "  <property value=\"0\">systems/chute/chute-released</property>" << std::endl;
    }
    file << std::endl;
    file << "  <channel name=\"" + _description[_subtype] + "\">" << std::endl;
    file << "   <kinematic name=\"" + _description[_subtype] + " Control\">" << std::endl;
    file << "     <input>systems/chute/chute-cmd-norm</input>" << std::endl;
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
    file << "     <output>systems/chute/chute-reef-pos-norm</output>" << std::endl;
    file << "   </kinematic>" << std::endl;

    if (_aircraft->_atype >= FIGHTER)
    {
        file << std::endl;
        file << "   <switch name=\"" + _description[_subtype] + " Availability\">" << std::endl;
        file << "     <default value=\"0\"/>" << std::endl;
        file << "     <test logic=\"OR\" value=\"1\">" << std::endl;
        file << "       <test logic=\"AND\" value=\"1\">" << std::endl;
        file << "          systems/chute/chute-available eq 1" << std::endl;
        file << "          systems/chute/chute-reef-pos-norm lt 1" << std::endl;
        file << "       </test>" << std::endl;
        file << "       <test logic=\"AND\" value=\"1\">" << std::endl;
        file << "          systems/chute/chute-released eq 0" << std::endl;
        file << "          systems/chute/chute-reef-pos-norm eq 1" << std::endl;
        file << "       </test>" << std::endl;
        file << "     </test>" << std::endl;
        file << "     <output>systems/chute/chute-available</output>" << std::endl;
        file << "   </switch>" << std::endl;
        file << std::endl;
        file << "   <switch name=\"" + _description[_subtype] + " Released Inverted\">" << std::endl;
        file << "     <default value=\"1\"/>" << std::endl;
        file << "     <test logic=\"AND\" value=\"0\">" << std::endl;
        file << "         systems/chute/chute-released eq 1" << std::endl;
        file << "     </test>" << std::endl;
        file << "     <output>systems/chute/chute-not-released</output>" << std::endl;
        file << "   </switch>" << std::endl;
        file << std::endl;
        file << "   <switch name=\"Drogue " + _description[_subtype] + " Deployed\">" << std::endl;
        file << "     <default value=\"0\"/>" << std::endl;
        file << "     <test logic=\"OR\" value=\"1\">" << std::endl;
        file << "       <test logic=\"AND\" value=\"1\">" << std::endl;
        file << "          gear/unit[1]/WOW eq 1" << std::endl;
        file << "          gear/unit[2]/WOW eq 1" << std::endl;
        file << "          systems/chute/chute-available eq 1" << std::endl;
        file << "       </test>" << std::endl;
        file << "       systems/chute/drogue-chute-deployed eq 1" << std::endl;
        file << "     </test>" << std::endl;
        file << "     <output>systems/chute/drogue-chute-deployed</output>" << std::endl;
        file << "   </switch>" << std::endl;
        file << std::endl;
        file << "   <switch name=\"" + _description[_subtype] + " Deployed\">" << std::endl;
        file << "     <default value=\"0\"/>" << std::endl;
        file << "     <test logic=\"OR\" value=\"1\">" << std::endl;
        file << "       <test logic=\"AND\" value=\"1\">" << std::endl;
        file << "          gear/unit[0]/WOW eq 1" << std::endl;
        file << "          systems/chute/drogue-chute-deployed eq 1" << std::endl;
        file << "       </test>" << std::endl;
        file << "       systems/chute/drag-chute-deployed eq 1" << std::endl;
        file << "     </test>" << std::endl;
        file << "     <output>systems/chute/drag-chute-deployed</output>" << std::endl;
        file << "   </switch>" << std::endl;
        file << std::endl;
        file << "   <summer name=\"" + _description[_subtype] + " Position\">" << std::endl;
        file << "      <input>systems/chute/drag-chute-deployed</input>" << std::endl;
        file << "      <bias>0.111111111</bias>" << std::endl;
        file << "      <output>systems/chute/drag-chute-offset</output>" << std::endl;
        file << "   </summer>" << std::endl;
        file << std::endl;
        file << "   <fcs_function name=\"" + _description[_subtype] + " Scaling\">" << std::endl;
        file << "     <function>" << std::endl;
        file << "       <product name=\"" + _description[_subtype] + " Scaling\">" << std::endl;
        file << "         <property>systems/chute/drag-chute-offset</property>" << std::endl;
        file << "         <property>systems/chute/chute-not-released</property>" << std::endl;
        file << "         <value>0.9</value>" << std::endl;
        file << "       </product>" << std::endl;
        file << "      </function>" << std::endl;
        file << "     <output>systems/chute/drag-chute-pos-norm</output>" << std::endl;
        file << "   </fcs_function>" << std::endl;
        file << std::endl;
        file << "   <kinematic name=\"Drogue " + _description[_subtype] + " Control\">" << std::endl;
        file << "     <input>systems/chute/drag-chute-pos-norm</input>" << std::endl;
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
        file << "     <output>systems/chute/chute-size-factor</output>" << std::endl;
        file << "   </kinematic>" << std::endl;
    }

    file << "  </channel>" << std::endl;

    return file.str();
}

std::string Chute::external_force()
{
    float CDchute = _CDchute_t[_aircraft->_atype][_aircraft->_engines];
    float Area = _ChuteArea_t[_aircraft->_atype][_aircraft->_engines];
    std::stringstream file;

    file << "  <property value=\"0\">systems/chute/chute-reef-pos-norm</property>" << std::endl;
    file << "  <property value=\"0\">systems/chute/chute-size-factor</property>" << std::endl;
    file << std::endl;
    file << "  <force name=\"chute\" frame=\"WIND\">" << std::endl;
    file << "   <function>" << std::endl;
    file << "    <product>" << std::endl;
    file << "     <property>aero/qbar-psf</property>" << std::endl;
    file << "     <property>systems/chute/chute-reef-pos-norm</property>" << std::endl;
    file << "     <property>systems/chute/chute-size-factor</property>" << std::endl;
    file << "     <value> " << CDchute << " </value>" << std::endl;
    file << "     <value> " << Area << " </value>" << std::endl;
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
    file << "    <x>-1 </x>" << std::endl;
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

// LIGHT chutes are rescue chutes of 55ft or 65ft
// FIGHTER chutes are drag chutes of 24ft (+ shaped)
// TRANSPORT chutes are 2 or 3 FIGHTER drag chutes
// 
// Given radiuses are laying flat on the ground:
// Project their surface area on half a sphere and recalculate
// the diameter and new surface area from there.
float const Chute::_ChuteArea_t[MAX_AIRCRAFT][5] =
{
    {  600.f,  600.f,  830.f,  830.f,  830.f },		// LIGHT
    {    0.f,    0.f,    0.f,    0.f,    0.f },		// PERFORMANCE
    {  115.f,  115.f,  115.f,  115.f,  115.f },		// FIGHTER
    {  230.f,  230.f,  230.f,  345.f,  345.f },		// JET_TRANSPORT
    {    0.f,    0.f,    0.f,    0.f,    0.f }
};
 
} /* namespace Aeromatic */

