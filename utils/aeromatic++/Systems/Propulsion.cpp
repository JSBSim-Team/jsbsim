// Propulsion.cpp -- Implements the Aircraft Propulsion types.
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

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <math.h>

#include <types.h>
#include <Aircraft.h>
#include "Propulsion.h"

namespace Aeromatic
{

std::string Engine::system()
{
    std::stringstream file;

    if (_aircraft->_no_engines > 1)
    {
        file << "  <channel name=\"Thruster\">" << std::endl;
        file << "   <summer name=\"Thrust Coefficient Left\">" << std::endl;

        for (unsigned i = 0; i < _aircraft->_no_engines; ++i)
        {
            if (_mount_point[i] == LEFT_WING) {
                file << "    <input>propulsion/engine[" << i << "]/thrust-coefficient</input>" << std::endl;
            }
        }
        file << "    <output>systems/propulsion/thrust-coefficient-left</output>" << std::endl;
        file << "   </summer>" << std::endl;
        file << "   <summer name=\"Thrust Coefficient Right\">" << std::endl;

        for (unsigned i = 0; i < _aircraft->_no_engines; ++i)
        {
            if (_mount_point[i] == RIGHT_WING) {
                file << "    <input>propulsion/engine[" << i << "]/thrust-coefficient</input>" << std::endl;
            }
        }
        file << "    <output>systems/propulsion/thrust-coefficient-right</output>" << std::endl;
        file << "   </summer>" << std::endl;
        file << "   <summer name=\"Thrust Coefficient Left-Right\">" << std::endl;

        file << "    <input>systems/propulsion/thrust-coefficient-left</input>" << std::endl;
        file << "    <input>-systems/propulsion/thrust-coefficient-right</input>" << std::endl;
        file << "    <output>systems/propulsion/thrust-coefficient-left-right</output>" << std::endl;
        file << "   </summer>" << std::endl;
        file << "   <summer name=\"Thrust Coefficient\">" << std::endl;

        file << "    <input>systems/propulsion/thrust-coefficient-left</input>" << std::endl;
        file << "    <input>systems/propulsion/thrust-coefficient-right</input>" << std::endl;
        file << "    <output>systems/propulsion/thrust-coefficient</output>" << std::endl;
        file << "   </summer>" << std::endl;
        file << "  </channel>" << std::endl;
    }

    return file.str();
}

Propulsion::Propulsion(Aeromatic *p) : Engine(p, 0)
{
    _description.push_back("Propulsion");
    _inputs_order.push_back("Propulsion");
    _inputs["Propulsion"] = new Param(_description[0].c_str(), _supported, _enabled);

    strCopy(_engine_name, "my_engine");
    _inputs_order.push_back("engine");
    _inputs["engine"] = new Param("Engine name", "The engine name is used for the engine configuration file name", _engine_name);

    _inputs_order.push_back("noEngines");
    _inputs["noEngines"] = new Param("Number of engines", "Engines are distributed evenly across the wing span", _aircraft->_no_engines);
    Param *layout = new Param("Engine layout", 0, _layout);
    _inputs_order.push_back("engineLayout");
    _inputs["engineLayout"] = layout;
    layout->add_option("fwd fuselage");
    layout->add_option("mid fuselage");
    layout->add_option("aft fuselage");
    layout->add_option("wings");
    layout->add_option("wings and tail");
    layout->add_option("wings and nose");

    Param *type = new Param("Engine type", 0, _ptype, MAX_ENGINE);
    _inputs_order.push_back("engineType");
    _inputs["engineType"] = type;

    _propulsion.push_back(new PistonEngine(p, this));
    type->add_option(_propulsion[0]->get_description());

    _propulsion.push_back(new TurbopropEngine(p, this));
    type->add_option(_propulsion[1]->get_description());

    _propulsion.push_back(new TurbineEngine(p, this));
    type->add_option(_propulsion[2]->get_description());

    _propulsion.push_back(new RocketEngine(p, this));
    type->add_option(_propulsion[3]->get_description());

    _propulsion.push_back(new ElectricEngine(p, this));
    type->add_option(_propulsion[4]->get_description());

    Engine::_propulsion = this;
}

Propulsion::~Propulsion()
{
    for(auto it : _propulsion) {
        delete it;
    }
    _propulsion.clear();
}

void Propulsion::param_reset()
{
    _param = 0;
    _propulsion[_ptype]->param_reset();
    _propulsion[_ptype]->_thruster->param_reset();
}

Param* Propulsion::param_next()
{
    Param* rv = 0;
    if (_enabled)
    {
        if (_param < _inputs.size()) {
            rv = _inputs[_inputs_order[_param++]];
        }
        else
        {
            rv = _propulsion[_ptype]->param_next();
            if (rv == 0) {
               rv = _propulsion[_ptype]->_thruster->param_next();
            }
        }
    }
    return rv;
}

std::string Propulsion::system()
{
    std::string file = Engine::system();

    // Create Engines directory
    std::string dir = Aeromatic::create_dir(_aircraft->_dir, "Engines");
    if (dir.empty()) {
        return file;
    }

    // open egnine file
    std::string efname = dir + "/" + get_propulsion() + ".xml";
    if (_aircraft->_overwrite || !Aeromatic::overwrite(efname))
    {
        std::ofstream efile;
        efile.open(efname.c_str());
        if (!efile.fail() && !efile.bad())
        {
            efile << "<?xml version=\"1.0\"?>" << std::endl;
            efile << std::endl;
            efile << propulsion();
        }
        else {
            std::cerr << "Failed to open file: " << efname << std::endl;
        }
        efile.close();
    }
    else {
        std::cout << "File already exsists: " << efname << std::endl;
    }

    // open thruster file
    std::string tfname = dir + "/" + get_thruster() + ".xml";
    if (_aircraft->_overwrite || !Aeromatic::overwrite(tfname))
    {
        std::ofstream tfile;
        tfile.open(tfname.c_str());
        if (!tfile.fail() && !tfile.bad())
        {
            tfile << "<?xml version=\"1.0\"?>" << std::endl;
            tfile << std::endl;
            tfile << thruster();
        }
        else {
            std::cerr << "Failed to open file: " << efname << std::endl;
        }
        tfile.close();
    }
    else {
        std::cout << "File already exsists: " << tfname << std::endl;
    }

    return file;
}

void Propulsion::set(const float cg_loc[3])
{
    unsigned no_engines = _aircraft->_no_engines;

    // forward fuselage engines
    switch(_layout)
    {
    case FWD_FUSELAGE:	// forward fuselage engines
    {
        float leftmost = (no_engines * -20.0f) + 20.0f;
        for (unsigned i=0; i<no_engines; ++i)
        {
            _eng_loc[i][X] = 36.0f;
            _eng_loc[i][Y] = leftmost + (i * 40.0f);
            _eng_loc[i][Z] = 0.0f;
            _mount_point[i] = FUSELAGE;
        }
        break;
    }
    case MID_FUSELAGE:	// mid fuselage engines
    {
        float leftmost = (no_engines * -20.0f) + 20.0f;
        for (unsigned i=0; i<no_engines; ++i)
        {
            _eng_loc[i][X] = cg_loc[X];
            _eng_loc[i][Y] = leftmost + (i * 40.0f);
            _eng_loc[i][Z] = -12.0f;
            _mount_point[i] = FUSELAGE;
        }
        break;
    }
    case AFT_FUSELAGE:	// aft fuselage engines
    {
        float leftmost = (no_engines * -20.0f) + 20.0f;
        for (unsigned i=0; i<no_engines; ++i)
        {
            _eng_loc[i][X] = (_aircraft->_length * FEET_TO_INCH) - 60.0f;
            _eng_loc[i][Y] = leftmost + (i * 40.0f);
            _eng_loc[i][Z] = 0.0f;
            _mount_point[i] = FUSELAGE;
        }
        break;
    }
    case WINGS:			// wing engines (odd one goes in middle)
    case WINGS_AND_TAIL:	// wing and tail engines
    case WINGS_AND_NOSE:	// wing and nose engines
    default:
    {
        if (no_engines > 1) {
            _aircraft->_wing_mounted_engines = true;
         }

        unsigned i, halfcount = no_engines / 2;
        unsigned remainder = no_engines - (halfcount * 2);

        for (i=0; i<halfcount; ++i)		// left wing
        {
            _eng_loc[i][X] = cg_loc[X];
            _eng_loc[i][Y] = _aircraft->_wing.span * -2.0f;	// span/-2/3*12
            _eng_loc[i][Z] = -40.0f;
            _mount_point[i] = LEFT_WING;
        }
        switch (_layout)
        {
        case WINGS:                             // center
            for (; i<halfcount+remainder; ++i)
            {
                _eng_loc[i][X] = cg_loc[X];
                _eng_loc[i][Y] = 0.0f;
                _eng_loc[i][Z] = -20.0f;
                _mount_point[i] = FUSELAGE;
            }
            break;
        case WINGS_AND_TAIL:			// tail
            for (; i<halfcount+remainder; ++i)
            {
                _eng_loc[i][X] = _aircraft->_length - 60;
                _eng_loc[i][Y] = 0.0f;
                _eng_loc[i][Z] = 60.0f;
                _mount_point[i] = FUSELAGE;
            }
            break;
        case WINGS_AND_NOSE:			// nose
        default:
            for (; i<halfcount+remainder; ++i)
            {
                _eng_loc[i][X] = 36.0f;
                _eng_loc[i][Y] = 0.0f;
                _eng_loc[i][Z] = 0.0f;
                _mount_point[i] = FUSELAGE;
            }
            break;
        }
        for (; i<no_engines; ++i)		// right wing
        {
            _eng_loc[i][X] = cg_loc[X];
            _eng_loc[i][Y] = _aircraft->_wing.span * 2.0f;	// span/2/3*12
            _eng_loc[i][Z] = -40.0f;
            _mount_point[i] = RIGHT_WING;
        }
        break;
    }
    }

    // thruster goes where engine is
    for (unsigned i=0; i<no_engines; ++i)
    {
        _eng_orient[i][PITCH] = 0.0f;
        _eng_orient[i][ROLL] = 0.0f;
        _eng_orient[i][YAW] = 0.0f;
        _thruster_loc[i][X] = _eng_loc[i][X];
        _thruster_loc[i][Y] = _eng_loc[i][Y];
        _thruster_loc[i][Z] = _eng_loc[i][Z];
        _thruster_orient[i][PITCH] = 0.0f;
        _thruster_orient[i][ROLL] = 0.0f;
        _thruster_orient[i][YAW] = 0.0f;
    }

//***** FUEL TANKS **********************************

    // an N-engined airplane will have N+1 fuel tanks
    // all tanks located at CG and are half full
    _fuel_weight = 0.0f;

    _tank_capacity = 0.0f;
    _tank_contents = 0.0f;

    _tank_loc[X] = cg_loc[X];
    _tank_loc[Y] = cg_loc[Y];
    _tank_loc[Z] = cg_loc[Z];
//  _tank_radius = 1.0f;

    if (no_engines > 0)
    {
       _fuel_weight = _aircraft->_max_weight * _aircraft->get_fuel_weight();
       _tank_capacity = _fuel_weight / (no_engines + 1);
       _tank_contents = _tank_capacity/2;
    }

    _aircraft->_payload -= _fuel_weight;
}

std::string Propulsion::mass_balance()
{
    std::stringstream file;
/*
    bool& convert = _aircraft->_metric;

    file.precision(2);
    file.flags(std::ios::right);
    file << std::fixed << std::showpoint;
    file << "   <pointmass name=\"Fuel\">" << std::endl;
    file << "    <description> " << _fuel_weight << " fuel contents </description>" << std::endl;
    file << "    <weight unit=\"" << Param::get_unit(true, WEIGHT, convert) << "\"> " << Param::get(_fuel_weight, WEIGHT, convert) << " </weight>" << std::endl;
    file << "    <location name=\"POINTMASS\" unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
    file << "     <x> " << std::setw(8) << Param::get(_tank_loc[X]*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << Param::get(_tank_loc[Y]*INCH_TO_FEET, LENGTH, convert) << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << Param::get(_tank_loc[Z]*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "  </pointmass>" << std::endl;
*/
    return file.str();
}

std::string Propulsion::comment()
{
    std::stringstream file;

    _aircraft->_ptype = _ptype;
    unsigned no_engines = _aircraft->_no_engines;

    file << "    no. engines:   " << no_engines << std::endl;
    file << "    engine type:   " << _propulsion[_ptype]->_description[0] << std::endl;
    file << "    engine layout: ";
    switch(_layout)
    {
    case FWD_FUSELAGE:
        file << "forward fuselage" << std::endl;
        break;
    case MID_FUSELAGE:
        file << "middle fuselage" << std::endl;
        break;
    case AFT_FUSELAGE:
        file << "aft fuselage" << std::endl;
        break;
    case WINGS:
        file << "wings" << std::endl;
        break;
    case WINGS_AND_TAIL:
        file << "wings and tail" << std::endl;
        break;
    case WINGS_AND_NOSE:
    default:
        file << "wings and nose" << std::endl;
        break;
    }

    return file.str();
}

std::string Propulsion::fdm()
{
    unsigned no_engines = _aircraft->_no_engines;
    bool& convert = _aircraft->_metric;
    std::stringstream file;

    file.precision(2);
    file.flags(std::ios::right);
    file << std::fixed << std::showpoint;
    file << " <propulsion>" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<no_engines; ++i)
    {
        file << "   <engine file=\"" << _engine_name << "\">" << std::endl;
        file << "    <feed> " << i << " </feed>" << std::endl;
        file << std::endl;
        file << "    <thruster file=\"" << get_thruster() << "\">" << std::endl;
        file << "     <sense> 1 </sense>" << std::endl;
        file << "     <location unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
        file << "       <x> " << std::setw(8) << Param::get(_thruster_loc[i][X]*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
        file << "       <y> " << std::setw(8) << Param::get(_thruster_loc[i][Y]*INCH_TO_FEET, LENGTH, convert) << " </y>" << std::endl;
        file << "       <z> " << std::setw(8) << Param::get(_thruster_loc[i][Z]*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
        file << "     </location>" << std::endl;
        file << "     <orient unit=\"DEG\">" << std::endl;
        file << "       <pitch> " << std::setw(8) << _thruster_orient[i][PITCH] << " </pitch>" << std::endl;
        file << "        <roll> " << std::setw(8) << _thruster_orient[i][ROLL] << " </roll>" << std::endl;
        file << "         <yaw> " << std::setw(8) << _thruster_orient[i][YAW] << " </yaw>" << std::endl;
        file << "     </orient>" << std::endl;
        file << "    </thruster>" << std::endl;
        file << "  </engine>" << std::endl;
        file << std::endl;
    }

    for (unsigned i=0; i<=no_engines; ++i)
    {
        file << "  <tank type=\"FUEL\" number=\"" << i << "\">" << std::endl;
        file << "     <location unit=\"" << Param::get_unit(true, LENGTH, convert) << "\">" << std::endl;
        file << "       <x> " << std::setw(8) << Param::get(_tank_loc[X]*INCH_TO_FEET, LENGTH, convert) << " </x>" << std::endl;
        file << "       <y> " << std::setw(8) << Param::get(_tank_loc[Y]*INCH_TO_FEET, LENGTH, convert) << " </y>" << std::endl;
        file << "       <z> " << std::setw(8) << Param::get(_tank_loc[Z]*INCH_TO_FEET, LENGTH, convert) << " </z>" << std::endl;
        file << "     </location>" << std::endl;
        file << "     <capacity unit=\"" << Param::get_unit(true, WEIGHT, convert) << "\"> " << Param::get(_tank_capacity, WEIGHT, convert) << " </capacity>" << std::endl;
        file << "     <contents unit=\"" << Param::get_unit(true, WEIGHT, convert) << "\"> " << Param::get(_tank_contents, WEIGHT, convert) << " </contents>" << std::endl;
        file << "  </tank>" << std::endl;
    }

    file << std::endl;
    file << " </propulsion>"<< std::endl;

    return file.str();
}

std::string Propulsion::json(const float cg_loc[3])
{
    unsigned no_engines = _aircraft->_no_engines;
    std::stringstream file;

    file.precision(1);
    file.flags(std::ios::left);
    file << std::fixed << std::showpoint;

    std::string param = "  \"engine\"";
    file << std::setw(12) << param << ": [ ";

    for (unsigned i=0; i<no_engines; ++i)
    {
        file << std::endl;
        file << "  {" << std::endl;
        param  = "    \"pos\"";
        file << std::setw(14) << param << ": [ "
                   << _eng_loc[i][X]-cg_loc[X] << ", "
                   << _eng_loc[i][Y]-cg_loc[Y] << ", "
                   << _eng_loc[i][Z]-cg_loc[Z] << " ],"
                   << std::endl;

        param  = "    \"dir\"";
        file << std::setw(14) << param << ": [ "
                   << _thruster_orient[i][PITCH] << ", "
                   << _thruster_orient[i][ROLL] << ", "
                   << _thruster_orient[i][YAW] << " ]";
        param = _propulsion[_ptype]->json();
        if (!param.empty())
        {
            file << "," << std::endl;
            file << std::endl;
            file << param << std::endl;;
        }
        if (i == no_engines-1) file << "  }";
        else file << "  },";
    }

    file << " ]";

    return file.str();
}


PistonEngine::PistonEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
{
    _description.push_back("Piston Engine");
    _inputs_order.push_back("pistonPower");
    _inputs["pistonPower"] = new Param("Engine power", "Providing fairly acurate engine power is critical for a good configuration", _propulsion->_power, _aircraft->_metric, POWER);
   _inputs_order.push_back("pistonMaxRPM");
   _inputs["pistonMaxRPM"] = new Param("Maximum engine rpm", "The maximum rpm is used to calculate the propeller power and thrust tables", _max_rpm);
    _thruster = new Propeller(p);
}

std::string PistonEngine::engine()
{
    std::stringstream file;

    _thruster->set_thruster(_max_rpm);

    float displacement = _propulsion->_power *1.9f;

    // Guess the area of one piston (5.125/2)^2 * PI
    float stroke = 4.375f;
    float bore   = 5.125f;

    // Guess the area of one piston (5.125/2)^2 * PI
    float bore_s = powf(bore/2, 2.0f) * PI;
    float n_cylinders = displacement /  (stroke * bore_s);
    n_cylinders = ((n_cylinders < 1) ? 1 : floorf(n_cylinders+0.5f));

    file << "<!--" << std::endl;
    file << "  File:     " << _propulsion->_engine_name << ".xml" << std::endl;
    file << "  Author:   AeromatiC++ v " << AEROMATIC_VERSION_STR << std::endl;
    file << std::endl;
    file << "  See: http://wiki.flightgear.org/JSBSim_Engines#FGPiston" << std::endl;
    file << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    name:           " << _propulsion->_engine_name << std::endl;
    file << "    type:           " << _description[0] <<  std::endl;
    file << "    power:          " << _propulsion->_power << " hp" << std::endl;
    file << "-->" << std::endl;
    file <<std::endl;
    file << "<piston_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <minmp unit=\"INHG\">         10.0 </minmp>" << std::endl;
    file << "  <maxmp unit=\"INHG\">         28.5 </maxmp>" << std::endl;
    file << "    <displacement unit=\"IN3\"> " << displacement << " </displacement>" << std::endl;
    file << "  <maxhp>        " << _propulsion->_power << " </maxhp>" << std::endl;
    file << "  <cycles>         4.0 </cycles>" << std::endl;
    file << "  <idlerpm>      700.0 </idlerpm>" << std::endl;
    file << "  <maxrpm>      2800.0 </maxrpm>" << std::endl;
    file << "  <sparkfaildrop>  0.1 </sparkfaildrop>" << std::endl;
    file << "  <volumetric-efficiency> 0.85 </volumetric-efficiency>" << std::endl;
    file << "  <man-press-lag> 0.1 </man-press-lag>" << std::endl;
    file << "  <static-friction  unit=\"HP\"> " << (_propulsion->_power * 0.005f) << " </static-friction>" << std::endl;
    file << "  <starter-torque> " << (_propulsion->_power * 0.8f) << " </starter-torque>" << std::endl;
    file << "  <starter-rpm> 1400 </starter-rpm>" << std::endl;
    file << " <!-- Defining <bsfc> over-rides the built-in horsepower calculations -->" << std::endl;
    file << " <!--<bsfc>           0.45 </bsfc>-->" << std::endl;
    file << "  <stroke unit=\"IN\">  " << stroke << " </stroke>" << std::endl;
    file << "  <bore unit=\"IN\">    " << bore << " </bore>" << std::endl;
    file << "  <cylinders>         " << n_cylinders << "  </cylinders>" << std::endl;
    file << "  <compression-ratio>  8.0 </compression-ratio>" << std::endl;
    file << "</piston_engine>" << std::endl;

    return file.str();
}

// http://web.mit.edu/16.unified/www/SPRING/propulsion/UnifiedPropulsion3/UnifiedPropulsion3.htm
// http://adg.stanford.edu/aa241/propulsion/images/tvsv.gif
// http://adg.stanford.edu/aa241/propulsion/nacelledesign.html
// http://adg.stanford.edu/aa241/propulsion/enginedata.html
// mass_eng = 0.4054 * powf(_power, 0.9255f);
// eng_length = 2.4077f * powf(_power, 0.3876f);
// eng_diameter = 1.0827f * powf(_power, 0.4134f);

TurbineEngine::TurbineEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
{
    _description.push_back("Turbine Engine");
    _inputs_order.push_back("turbineMiLThrust");
    _inputs["turbineMiLThrust"] = new Param("Engine mil. thrust", "Providing fairly acurate engine thrust is critical for a good configuration", _propulsion->_power, _aircraft->_metric, THRUST);
    _inputs_order.push_back("turbineBypassRatio");
    _inputs["turbineBypassRatio"] = new Param("Bypass ratio", "The bypass ratio is mainly used for calculating fuel consumption", _bypass_ratio);
    _inputs_order.push_back("turbinePressureRatio");
    _inputs["turbinePressureRatio"] = new Param("Overall pressure ratio", "Overall pressure ratio is used to finetune the estimated fuel consumption", _oapr);
    _inputs_order.push_back("turbineAugmentation");
    _inputs["turbineAugmentation"] = new Param("Augmented?", "Does the engine have afterburner capability?", _augmented);
    _inputs_order.push_back("turbineWaterInjection");
    _inputs["turbineWaterInjection"] = new Param("Water injection?", "Does the engine have ater injection boost?", _injected);
    _thruster = new Direct(p);
}

std::string TurbineEngine::engine()
{
    std::stringstream file;

    _thruster->set_thruster();
    float max_thrust = _propulsion->_power;
    if (_augmented) {
        max_thrust *= 1.5f;
    }

    // Figure 3.10
    float tsfc, atsfc;
    if (_bypass_ratio < 1.0f)
    {
        if (_bypass_ratio < 0.07f) _bypass_ratio = 0.07f;
        tsfc =  0.635f - 0.144f * log10f(_oapr) * log10f(_bypass_ratio);
    }
    else {
        tsfc = 0.7533f - 0.161f * log10f(0.0625f * _oapr * _bypass_ratio);
    }
    atsfc = 3.27f - 0.451f * log10f(2.9f * _oapr / _bypass_ratio);

    file.precision(1);
    file.flags(std::ios::right);
    file << std::fixed << std::showpoint;
    file << "<!--" << std::endl;
    file << "  File:     " << _propulsion->_engine_name << ".xml" << std::endl;
    file << "  Author:   AeromatiC++ v " << AEROMATIC_VERSION_STR << std::endl;
    file << std::endl;
    file << "  See: http://wiki.flightgear.org/JSBSim_Engines#FGTurbine" << std::endl;
    file << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    name:                    " << _propulsion->_engine_name << std::endl;
    file << "    type:                    " << _description[0] <<  std::endl;
    file << "    thrust:                  " << _propulsion->_power << " lbf" << std::endl;
    file << "    bypass ratio:            " << std::setprecision(3) << _bypass_ratio << std::setprecision(1) << ":1" << std::endl;
    file << "    overall pressure ratio:  " << _oapr << ":1" << std::endl;
    file << "    augmented?               " << (_augmented ? "yes" : "no") << std::endl;
    file << "    injected?                " << (_injected ? "yes" : "no") << std::endl;
     file << std::endl;
    file << "  Outputs" << std::endl;
    file << "    tsfc:                    " << tsfc << std::endl;
    file << "    engine weight:           " << _propulsion->_weight << " lbs" << std::endl;
    file << "    engine length:           " << (_propulsion->_length * (_augmented ? 2.0f : 1.0f)) << " ft" << std::endl;
    file << "    engine diameter:         " << _propulsion->_diameter << " ft" <<std::endl;
    file << "-->" << std::endl;
    file <<std::endl;
    file << "<turbine_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <milthrust> " << _propulsion->_power << " </milthrust>" << std::endl;
    if (_augmented) {
        file << "  <maxthrust> " << max_thrust << " </maxthrust>" << std::endl;
    }
    file << "  <bypassratio>     " << std::setprecision(3) << _bypass_ratio << " </bypassratio>" << std::endl;
    file << "  <tsfc>            " << tsfc << " </tsfc>" << std::endl;
    if (_augmented) {
        file << "  <atsfc>           " << atsfc << " </atsfc>" << std::endl;
    }
    file << "  <bleed>           0.03</bleed>" << std::endl;
    file << "  <idlen1>         30.0 </idlen1>" << std::endl;
    file << "  <idlen2>         60.0 </idlen2>" << std::endl;
    file << "  <maxn1>         100.0 </maxn1>" << std::endl;
    file << "  <maxn2>         100.0 </maxn2>" << std::endl;
    file << "  <augmented>         " << _augmented << " </augmented>" << std::endl;
    if (_augmented) {
        file << "<augmethod>         1 </augmethod>" << std::endl;
    }

    file << "  <injected>          " << _injected << " </injected>" << std::endl;
    file << "" << std::endl;

    file << "  <function name=\"IdleThrust\">" << std::endl;
    file << "   <table>" << std::endl;
    file << "    <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>" << std::endl;
    file << "    <tableData>" << std::endl;
    file << "         -10000     0     10000   20000   30000   40000   50000   90000" << std::endl;
    file << "     0.0  0.0430  0.0488  0.0528  0.0694  0.0899  0.1183  0.1467  0" << std::endl;
    file << "     0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.1342  0" << std::endl;
    file << "     0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.1203  0" << std::endl;
    file << "     0.6 -0.0804 -0.0804 -0.0560 -0.0237  0.0276  0.0718  0.1073  0" << std::endl;
    file << "     0.8 -0.2129 -0.2129 -0.1498 -0.1025  0.0474  0.0868  0.0900  0" << std::endl;
    file << "     1.0 -0.2839 -0.2839 -0.1104 -0.0469 -0.0270  0.0552  0.0800  0" << std::endl;
    file << "    </tableData>" << std::endl;
    file << "   </table>" << std::endl;
    file << "  </function>" << std::endl;
    file << std::endl;
    file << "  <function name=\"MilThrust\">" << std::endl;
    file << "   <table>" << std::endl;
    file << "    <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>" << std::endl;
    file << "    <tableData>" << std::endl;
    file << "          -10000       0   10000   20000   30000   40000   50000   60000   90000" << std::endl;

    for (unsigned i=0; i<8; ++i)
    {
        float M = 0.2f*i;
        file << std::setw(9) <<std::setprecision(1) << M;
        for (unsigned j=0; j<8; ++j)
        {
           file << std::fixed << std::setw(8) << std::setprecision(4);
           file << (1.0f - 0.11*M*_bypass_ratio)*_milthrust_t[i][j];
        }
        file << std::fixed << std::setw(3) << "0";
        file << std::endl;
    }

    file << "    </tableData>" << std::endl;
    file << "   </table>" << std::endl;
    file << "  </function>" << std::endl;
    file << std::endl;

    if (_augmented)
    {
        file << "  <function name=\"AugThrust\">" << std::endl;
        file << "   <table>" << std::endl;
        file << "    <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
        file << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>" << std::endl;
        file << "    <tableData>" << std::endl;
        file << "           -10000       0   10000   20000   30000   40000   50000   60000" << std::endl;
        file << "     0.0    1.1816  1.0000  0.8184  0.6627  0.5280  0.3756  0.2327  0" << std::endl;
        file << "     0.2    1.1308  0.9599  0.7890  0.6406  0.5116  0.3645  0.2258  0" << std::endl;
        file << "     0.4    1.1150  0.9474  0.7798  0.6340  0.5070  0.3615  0.2240  0" << std::endl;
        file << "     0.6    1.1284  0.9589  0.7894  0.6420  0.5134  0.3661  0.2268  0" << std::endl;
        file << "     0.8    1.1707  0.9942  0.8177  0.6647  0.5309  0.3784  0.2345  0" << std::endl;
        file << "     1.0    1.2411  1.0529  0.8648  0.7017  0.5596  0.3983  0.2467  0" << std::endl;
        file << "     1.2    1.3287  1.1254  0.9221  0.7462  0.5936  0.4219  0.2614  0" << std::endl;
        file << "     1.4    1.4365  1.2149  0.9933  0.8021  0.6360  0.4509  0.2794  0" << std::endl;
        file << "     1.6    1.5711  1.3260  1.0809  0.8700  0.6874  0.4860  0.3011  0" << std::endl;
        file << "     1.8    1.7301  1.4579  1.1857  0.9512  0.7495  0.5289  0.3277  0" << std::endl;
        file << "     2.0    1.8314  1.5700  1.3086  1.0474  0.8216  0.5786  0.3585  0" << std::endl;
        file << "     2.2    1.9700  1.6900  1.4100  1.2400  0.9100  0.6359  0.3940  0" << std::endl;
        file << "     2.4    2.0700  1.8000  1.5300  1.3400  1.0000  0.7200  0.4600  0" << std::endl;
        file << "     2.6    2.2000  1.9200  1.6400  1.4400  1.1000  0.8000  0.5200  0" << std::endl;
        file << "    </tableData>" << std::endl;
        file << "   </table>" << std::endl;
        file << "  </function>" << std::endl;
        file << std::endl;
    }

    if (_injected)
    {
        file << "  <function name=\"Injection\">" << std::endl;
        file << "   <table>" << std::endl;
        file << "    <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
        file << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>" << std::endl;
        file << "    <tableData>" << std::endl;
        file << "            0       50000" << std::endl;
        file << "     0.0    1.2000  1.2000" << std::endl;
        file << "     1.0    1.2000  1.2000" << std::endl;
        file << "    </tableData>" << std::endl;
        file << "   </table>" << std::endl;
        file << "  </function>" << std::endl;
        file << std::endl;
    }
    file << "</turbine_engine>" << std::endl;

    return file.str();
}

std::string TurbineEngine::json()
{
    std::stringstream file;

    file.precision(1);
    file.flags(std::ios::left);
    file << std::fixed << std::showpoint;

    float max_thrust = _propulsion->_power;
    if (_augmented) {
        max_thrust *= 1.5f;
    }

    std::string param  = "    \"FT_max\"";
    file << std::setw(14) << param << ": " << max_thrust;

    return file.str();
}

TurbopropEngine::TurbopropEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
{
    _description.push_back("Turboprop Engine");
    _inputs_order.push_back("turbopropPower");
    _inputs["turbopropPower"] = new Param("Engine power", "Providing fairly acurate engine power is critical for a good configuration", _propulsion->_power, _aircraft->_metric, POWER);
    _inputs_order.push_back("turbopropMaxRPM");
    _inputs["turbopropMaxRPM"] = new Param("Maximum engine rpm", "The maximum rpm is used to calculate the propeller power and thrust tables", _max_rpm);
    _inputs_order.push_back("turbopropPressureRatio");
    _inputs["turbopropPressureRatio"] = new Param("Overall pressure ratio", "Overall pressure ratio is used to finetune the estimated fuel consumption", _oapr);
    _inputs_order.push_back("turbopropITT");
    _inputs["turbopropITT"] = new Param("Turbine inlet temperature", "Turbine inlet temperature is used to finetune the engine configuration", _itt);
    _thruster = new Propeller(p);
}

// http://www.fzt.haw-hamburg.de/pers/Scholz/Airport2030/Airport2030_PUB_ICAS_12-09-23.pdf
//
// _power is in kW for the following computations:
// _oapr = overall  pressure ratio at static sea level
// Ttet = turbine  entry  temperature at static sea level in Kelvin
//
// mass_eng = 0.246f * _power;
// eng_length = 0.1068f * powf(_power, 0.4094f);
// eng_diameter = 0.1159f * powf(_power, 0.2483f);
// psfc = 2.56*10e-4 - logf(_power * _oapr * Ttet)*10e-5;
std::string TurbopropEngine::engine()
{
    Propeller *propeller = (Propeller*)_thruster;
    bool& convert = _aircraft->_metric;
    std::stringstream file;

    _thruster->set_thruster(_max_rpm);

    float max_rpm = propeller->max_rpm();
    float psfc = 0.5f;

    // calculate psfc in KG/SEC/KW
    float Ttet = _itt + 274.15f;	// in Kelvin
    psfc = 2.56e-4f - logf(_propulsion->_power*HP_TO_KW * _oapr * Ttet)*1e-5f;

    // Convert psfc to LBS/HR/HP
    psfc *= 5918.3525f;

    // estimate thrust if given power in HP
    float thrust = _propulsion->_power * 2.24f;

    // Torque = Power * 5252 / RPM
    float torque = 1.07f * _propulsion->_power * 5252.0f / max_rpm;

    file.precision(1);
    file.flags(std::ios::right);
    file << std::fixed; //  << std::showpoint;
    file << "<!--" << std::endl;
    file << "  File:     " << _propulsion->_engine_name << ".xml" << std::endl;
    file << "  Author:   AeromatiC++ v " << AEROMATIC_VERSION_STR << std::endl;
    file << std::endl;
    file << "  See: http://wiki.flightgear.org/JSBSim_Engines#FGTurboprop" << std::endl;
    file << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    name:                   " << _propulsion->_engine_name << std::endl;
    file << "    type:                   " << _description[0] <<  std::endl;
    file << "    power:                  " << _propulsion->_power << " hp" << std::endl;
    file << "    inlet temperature:      " << _itt << " degrees C"<< std::endl;
    file << "    overall pressure ratio: " << _oapr << ":1" << std::endl;
    file << std::endl;
    file << "  Outputs:" << std::endl;
    file << "    psfc:                   " << std::setprecision(3) << psfc << std::setprecision(1) << " lbs/hr/hp" << std::endl;
    file << "    engine weight:          " << Param::get_nice(0.246f * _propulsion->_power * KG_TO_LBS, WEIGHT, convert) << std::endl;
    file << "    engine length:          " << Param::get_nice(0.1068f * powf(_propulsion->_power, 0.4094f) * METER_TO_FEET, LENGTH, convert) << std::endl;
    file << "    engine diameter:        " << Param::get_nice(0.1159f * powf(_propulsion->_power, 0.2483f) * METER_TO_FEET, LENGTH, convert) << std::endl;
    file << "-->" << std::endl;
    file <<std::endl;
file << "<turboprop_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <milthrust unit=\"" << Param::get_unit(true, WEIGHT, convert) << "\">       " << Param::get(thrust, WEIGHT, convert) << "   </milthrust>" << std::endl;
    file << "  <idlen1>                       60.0   </idlen1>" << std::endl;
    file << "  <maxn1>                       100.0   </maxn1>" << std::endl;
    file << "  <maxpower unit=\"HP\">         " << std::setw(6) << _propulsion->_power << "   </maxpower>" << std::endl;
    file << "  <psfc unit=\"LBS/HR/HP\">         " << std::setprecision(3) << psfc << std::setprecision(1) << " </psfc>" << std::endl;
    file << "  <n1idle_max_delay>              1     </n1idle_max_delay>" << std::endl;
    file << "  <maxstartingtime>              20     </maxstartingtime>" << std::endl;
    file << "  <startern1>                    20     </startern1>" << std::endl;
    file << "  <ielumaxtorque unit=\"FT*LB\"> " << torque << "   </ielumaxtorque>" << std::endl;
    file << "  <itt_delay>                     0.05  </itt_delay>" << std::endl;
    file << "  <betarangeend>                 64     </betarangeend>" << std::endl;
    file << "  <reversemaxpower>              60     </reversemaxpower>" << std::endl;
    file << std::endl;
    file << "  <function name=\"EnginePowerVC\">" << std::endl;
    file << "    <table>" << std::endl;
    file << "      <description> Engine power, function of airspeed and pressure </description>" << std::endl;
    file << "      <independentVar lookup=\"row\">atmosphere/P-psf</independentVar>" << std::endl;
    file << "      <independentVar lookup=\"column\">velocities/ve-kts</independentVar>" << std::endl;
    file << "      <tableData>" << std::endl;
    file << "              0      50     100    150    200    250    300    350" << std::endl;
    file << "        503   0.357  0.380  0.400  0.425  0.457  0.486  0.517  0.550" << std::endl;
    file << "       1048   0.586  0.589  0.600  0.621  0.650  0.686  0.724  0.764"<< std::endl;
    file << "       1328   0.707  0.721  0.731  0.757  0.786  0.821  0.858  0.896" << std::endl;
    file << "       1496   0.779  0.786  0.808  0.821  0.857  0.900  0.945  0.993" << std::endl;
    file << "       1684   0.850  0.857  0.874  0.900  0.943  0.979  1.016  1.055" << std::endl;
    file << "       1896   0.914  0.929  0.946  0.971  1      1.057  1.117  1.181" << std::endl;
    file << "       2135   1      1.011  1.029  1.043  1.083  1.150  1.221  1.297" << std::endl;
    file << "       2213   1.029  1.043  1.057  1.079  1.114  1.171  1.231  1.294" << std::endl;
    file << "     </tableData>" << std::endl;
    file << "   </table>" << std::endl;
    file << "  </function>" << std::endl;
    file << std::endl;
    file << "  <table name=\"EnginePowerRPM_N1\" type=\"internal\">" << std::endl;
    file << "    <description> Engine Power, function of RPM and N1 </description>" << std::endl;
    file << "    <tableData>" << std::endl;

    file << "              0       5       60      86      94      95      96      97      98      99     100     101" << std::endl;
    for (unsigned i=0; i<6; ++i)
    {
        file << std::setw(9) << _eng_pwr_t[i][0] * max_rpm;
        for (unsigned j=1; j<13; ++j)
        {
           file << std::fixed << std::setw(8) << std::setprecision(1) << (_eng_pwr_t[i][j] * _propulsion->_power);
        }
        file << std::endl;
    }

    file << "    </tableData>" << std::endl;
    file << "  </table>" << std::endl;
    file << std::endl;
    file << "  <table name=\"ITT_N1\" type=\"internal\">" << std::endl;
    file << "    <description> Inter-Turbine Temperature ITT [deg C] depending on N1 and engine run (0=off / 1=running) </description>" << std::endl;
    file << "    <tableData>" << std::endl;
    file << "              0     1" << std::endl;
    file << "        0     0     0" << std::endl;
    file << "       15" << std::setw(8) << (0.145f * _itt) << std::setw(8) << (0.145f * _itt) << std::endl;
    file << "       60" << std::setw(8) << (0.26f * _itt) << std::setw(8) << (0.754f * _itt) << std::endl;
    file << "       96" << std::setw(8) << (0.391f * _itt) << std::setw(8) << (0.986f * _itt) << std::endl;
    file << "      100"<< std::setw(8) << (0.406f * _itt) << std::setw(8) << (1.09f * _itt) << std::endl;
    file << "    </tableData>" << std::endl;
    file << "  </table>" << std::endl;
    file << std::endl;
    file << "  <table name=\"CombustionEfficiency_N1\" type=\"internal\">" << std::endl;
    file << "    <description>Dependency of fuel efficiency coefficient on N1 (and RPM)</description>" << std::endl;
    file << "    <tableData>" << std::endl;

    // TODO: Make engine specific?
    file << "      90    0.1221" << std::endl;
    file << "      91.2  0.2834" << std::endl;
    file << "      92.2  0.5336" << std::endl;
    file << "      93.4  0.7188" << std::endl;
    file << "      94.1  0.7741" << std::endl;
    file << "      95.2  0.8471" << std::endl;
    file << "      96.5  0.9001" << std::endl;
    file << "     100    1" << std::endl;

    file << "      </tableData>" << std::endl;
    file << "  </table>" << std::endl;
    file << "</turboprop_engine>" << std::endl;

    return file.str();
}

RocketEngine::RocketEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
{
    _description.push_back("Rocket Engine");
    _inputs_order.push_back("rocketThrust");
    _inputs["rocketThrust"] = new Param("Engine thrust", "Providing fairly acurate engine thrust is critical for a good configuration", _propulsion->_power, _aircraft->_metric, THRUST);
    _thruster = new Nozzle(p);
}

std::string RocketEngine::engine()
{
    std::stringstream file;

    _thruster->set_thruster();
    file << "<!--" << std::endl;
    file << "  File:     " << _propulsion->_engine_name << ".xml" << std::endl;
    file << "  Author:   AeromatiC++ v " << AEROMATIC_VERSION_STR << std::endl;
    file << std::endl;
    file << "  See: http://wiki.flightgear.org/JSBSim_Engines#FGRocket" << std::endl;
    file << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    thrust:           " << _propulsion->_power << " lb" << std::endl;
    file << std::endl;
    file << "  Outputs:" << std::endl;
    file << "    ISP (sea level)     400.0" << std::endl;
    file << "    Fuel Flow Rate (SL)  91.5" << std::endl;
    file << "    Ox. Flow Rate (SL)  105.2" << std::endl;
    file << "-->" << std::endl;
    file << std::endl;

    file << "<rocket_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <isp>            400.00 </isp>" << std::endl;
    file << "  <minthrottle>      0.40  </minthrottle>" << std::endl;
    file << "  <maxthrottle>      1.00  </maxthrottle>" << std::endl;
    file << "  <slfuelflowmax>   91.50 </slfuelflowmax>" << std::endl;
    file << "  <sloxiflowmax>   105.20 </sloxiflowmax>" << std::endl;
    file << "</rocket_engine>" << std::endl;

    return file.str();
}

std::string RocketEngine::json()
{
    std::stringstream file;

    file.precision(1);
    file.flags(std::ios::left);
    file << std::fixed << std::showpoint;

    float max_thrust = _propulsion->_power;

    std::string param  = "    \"FT_max\"";
    file << std::setw(14) << param << ": " << max_thrust;

    return file.str();
}


ElectricEngine::ElectricEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
{
    _description.push_back("Electric Engine");
    _inputs_order.push_back("electricPower");
    _inputs["electricPower"] = new Param("Engine power", "Providing fairly acurate engine power is critical for a good configuration", _propulsion->_power, _aircraft->_metric, POWER);
    _inputs_order.push_back("electricRPM");
    _inputs["electricRPM"] = new Param("Maximum engine rpm", "The maximum rpm is used to calculate the propeller power and thrust tables", _max_rpm);
    _thruster = new Propeller(p);
}

std::string ElectricEngine::engine()
{
    std::stringstream file;

    _thruster->set_thruster(_max_rpm);
    file << "<!--" << std::endl;
    file << "  File:     " << _propulsion->_engine_name << ".xml" << std::endl;
    file << "  Author:   AeromatiC++ v " << AEROMATIC_VERSION_STR << std::endl;
    file << std::endl;
    file << "  See: http://wiki.flightgear.org/JSBSim_Engines#FGElectric" << std::endl;
    file << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    power:          " << _propulsion->_power << " hp" << std::endl;
    file << "-->" << std::endl;
    file << std::endl;
    file << "<electric_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << " <power unit=\"WATTS\">  " << (_propulsion->_power*HP_TO_KW*1000.0f) << " </power>" << std::endl;
    file << "</electric_engine>" << std::endl;
    return file.str();
}

// ---------------------------------------------------------------------------

float const TurbineEngine::_milthrust_t[8][8] =
{
   { 1.2600f, 1.0000f, 0.7400f, 0.5340f, 0.3720f, 0.2410f, 0.1490f, 0.058f },
   { 1.1710f, 0.9340f, 0.6970f, 0.5060f, 0.3550f, 0.2310f, 0.1430f, 0.040f },
   { 1.1500f, 0.9210f, 0.6920f, 0.5060f, 0.3570f, 0.2330f, 0.1450f, 0.043f },
   { 1.1810f, 0.9510f, 0.7210f, 0.5320f, 0.3780f, 0.2480f, 0.1540f, 0.047f },
   { 1.2580f, 1.0200f, 0.7820f, 0.5820f, 0.4170f, 0.2750f, 0.1700f, 0.053f },
   { 1.3690f, 1.1200f, 0.8710f, 0.6510f, 0.4750f, 0.3150f, 0.1950f, 0.063f },
   { 1.4850f, 1.2300f, 0.9750f, 0.7440f, 0.5450f, 0.3640f, 0.2250f, 0.074f },
   { 1.5941f, 1.3400f, 1.0860f, 0.8450f, 0.6280f, 0.4240f, 0.2630f, 0.090f }
};

float const TurbopropEngine::_eng_pwr_t[6][13] =
{
    { 0.000f,  0.0000f, 0.0007f, 0.0007f, 0.0007f, 0.0007f, 0.0007f, 0.0007f, 0.0007f, 0.0007f, 0.0007f, 0.0007f, 0.0007f },
    { 0.364f, 0.0000f, 0.0007f, 0.0471f, 0.2692f, 0.4711f, 0.5114f, 0.5653f, 0.6191f, 0.6729f, 0.7133f, 0.7806f, 0.8345f },
    { 0.545f, 0.0000f, 0.0007f, 0.0404f, 0.3096f, 0.5384f, 0.5787f, 0.6326f, 0.6797f, 0.7402f, 0.7941f, 0.8614f, 0.9152f },
    { 0.727f, 0.0000f, 0.0007f, 0.0067f, 0.3230f, 0.5922f, 0.6393f, 0.6864f, 0.7402f, 0.8008f, 0.8479f, 0.9152f, 0.9690f },
    { 0.909f, 0.0000f, 0.0001f, 0.0001f, 0.3028f, 0.6057f, 0.6662f, 0.7066f, 0.7604f, 0.8210f, 0.8748f, 0.9421f, 1.0027f },
    { 1.000f, 0.0000f, 0.0001f, 0.0001f, 0.2759f, 0.5922f, 0.6460f, 0.6931f, 0.7537f, 0.8143f, 0.8681f, 0.9354f, 1.0000f }
};

} /* namespace Aeromatic */

