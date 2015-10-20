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

#if (win32)
#else
# include <sys/stat.h>
#endif
#include <math.h>

#include <fstream>
#include <sstream>

#include <types.h>
#include <Aircraft.h>
#include "Propulsion.h"

namespace Aeromatic
{

Propulsion::Propulsion(Aeromatic *p) : Engine(p, this),
    _layout(FWD_FUSELAGE)
{
    _description.push_back("Propulsion");
    _inputs.push_back(new Param(_description[0].c_str(), _enabled));

    snprintf(_engine_name, PARAM_MAX_STRING, "my_engine");
    _inputs.push_back(new Param("Engine name", _engine_name));

    _inputs.push_back(new Param("Number of engines", _aircraft->_no_engines));
    Param *layout = new Param("Engine layout", _layout);
    _inputs.push_back(layout);
    layout->add_option("fwd fuselage");
    layout->add_option("mid fuselage");
    layout->add_option("aft fuselage");
    layout->add_option("wings");
    layout->add_option("wings and tail");
    layout->add_option("wings and nose");

    Param *type = new Param("Engine type", _ptype);
    _inputs.push_back(type);
    _propulsion[0] = new PistonEngine(p, this);
    type->add_option(_propulsion[0]->get_description());
    _propulsion[1] = new TurbopropEngine(p, this);
    type->add_option(_propulsion[1]->get_description());
    _propulsion[2] = new TurbineEngine(p, this);
    type->add_option(_propulsion[2]->get_description());
    _propulsion[3] = new RocketEngine(p, this);
    type->add_option(_propulsion[3]->get_description());
    _propulsion[4] = new ElectricEngine(p, this);
    type->add_option(_propulsion[4]->get_description());
}

Propulsion::~Propulsion()
{
    for (unsigned i=0; i<MAX_PROPULSION; ++i) {
        delete _propulsion[i];
    }

    std::vector<Param*>::iterator it;
    for(it = _inputs.begin(); it != _inputs.end(); ++it) {
        delete *it;
    }
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
            rv = _inputs[_param++];
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
    std::stringstream file;

    // Create Engines directory
    std::string dir = std::string(_aircraft->_path) + "/Engines";
#if (win32)
    if (!PathFileExists(dir) {
        if (CreateDirectory(dir, NULL) == 0) {
            return file.str();
        }
    }
#else
    struct stat sb;
    if (stat(dir.c_str(), &sb))
    {
        int mode = strtol("0755", 0, 8);
        if (mkdir(dir.c_str(), mode) == -1) {
            return file.str();
        }
    }
#endif

    // open egnine file
    std::string efname = dir + "/" + get_propulsion() + ".xml";
    std::ofstream efile;

    efile.open(efname.c_str());
    if (!efile.fail() && !efile.bad())
    {
        efile << "<?xml version=\"1.0\"?>" << std::endl;
        efile << std::endl;
        efile << propulsion();
    }
    efile.close();

    // open thruster file
    std::string tfname = dir + "/" + get_thruster() + ".xml";
    std::ofstream tfile;

    tfile.open(tfname.c_str());
    if (!tfile.fail() && !tfile.bad())
    {
        tfile << "<?xml version=\"1.0\"?>" << std::endl;
        tfile << std::endl;
        tfile << thruster();
    }
    tfile.close();

    return file.str();
}

void Propulsion::set(const float* cg_loc)
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
        }
        break;
    }
    case WINGS:			// wing engines (odd one goes in middle)
    case WINGS_AND_TAIL:	// wing and tail engines
    case WINGS_AND_NOSE:	// wing and nose engines
    default:
    {
        unsigned i, halfcount = no_engines / 2;
        unsigned remainder = no_engines - (halfcount * 2);

        for (i=0; i<halfcount; ++i)		// left wing
        {
            _eng_loc[i][X] = cg_loc[X];
            _eng_loc[i][Y] = _aircraft->_wing_span * -2.0f;	// span/-2/3*12
            _eng_loc[i][Z] = -40.0f;
        }
        switch (_layout)
        {
        case WINGS:                             // center
            for (; i<halfcount+remainder; ++i)
            {
                _eng_loc[i][X] = cg_loc[X];
                _eng_loc[i][Y] = 0.0f;
                _eng_loc[i][Z] = -20.0f;
            }
            break;
        case WINGS_AND_TAIL:			// tail
            for (; i<halfcount+remainder; ++i)
            {
                _eng_loc[i][X] = _aircraft->_length - 60;
                _eng_loc[i][Y] = 0.0f;
                _eng_loc[i][Z] = 60.0f;
            }
            break;
        case WINGS_AND_NOSE:			// nose
        default:
            for (; i<halfcount+remainder; ++i)
            {
                _eng_loc[i][X] = 36.0f;
                _eng_loc[i][Y] = 0.0f;
                _eng_loc[i][Z] = 0.0f;
            }
            break;
        }
        for (; i<no_engines; ++i)		// right wing
        {
            _eng_loc[i][X] = cg_loc[X];
            _eng_loc[i][Y] = _aircraft->_wing_span * 2.0f;	// span/2/3*12
            _eng_loc[i][Z] = -40.0f;
        }
        break;
    }
    }

    // thruster goes where engine is
    for (unsigned i=0; i<no_engines; ++i)
    {
        _eng_orient[i][PITCH] = 0.0f;
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

    file << "   <pointmass name=\"Fuel\">" << std::endl;
    file << "    <description> " << _fuel_weight << " fuel contents </description>" << std::endl;
    file << "    <weight unit=\"LBS\"> " << _fuel_weight << " </weight>" << std::endl;
    file << "    <location name=\"POINTMASS\" unit=\"IN\">" << std::endl;
    file << "     <x> " << _tank_loc[X] << " </x>" << std::endl;
    file << "     <y> " << _tank_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << _tank_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "  </pointmass>" << std::endl;

    return file.str();
}

std::string Propulsion::comment()
{
    std::stringstream file;

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
    std::stringstream file;

    file << " <propulsion>" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<no_engines; ++i)
    {
        file << "   <engine file=\"" << _engine_name << "\">" << std::endl;
        file << "    <location unit=\"IN\">" << std::endl;
        file << "      <x> " << _eng_loc[i][X] << " </x>" << std::endl;
        file << "      <y> " << _eng_loc[i][Y] << " </y>" << std::endl;
        file << "      <z> " << _eng_loc[i][Z] << " </z>" << std::endl;
        file << "    </location>" << std::endl;
        file << "    <orient unit=\"DEG\">" << std::endl;
        file << "      <pitch> " << _eng_orient[i][PITCH] << " </pitch>" << std::endl;
        file << "       <roll> "<< _eng_orient[i][ROLL] << " </roll>" << std::endl;
        file << "        <yaw> " << _eng_orient[i][YAW] << " </yaw>" << std::endl;
        file << "    </orient>" << std::endl;
        file << "    <feed> " << i << " </feed>" << std::endl;
        file << std::endl;
        file << "    <thruster file=\"" << _thruster << "\">" << std::endl;

        // TODO: open a new file for the thruster
        // file << thruster();

        file << "     <location unit=\"IN\">" << std::endl;
        file << "       <x> " << _thruster_loc[i][X] << " </x>" << std::endl;
        file << "       <y> " << _thruster_loc[i][Y] << " </y>" << std::endl;
        file << "       <z> " << _thruster_loc[i][Z] << " </z>" << std::endl;
        file << "     </location>" << std::endl;
        file << "     <orient unit=\"DEG\">" << std::endl;
        file << "       <pitch> " << _thruster_orient[i][PITCH] << " </pitch>" << std::endl;
        file << "        <roll> " << _thruster_orient[i][ROLL] << " </roll>" << std::endl;
        file << "         <yaw> " << _thruster_orient[i][YAW] << " </yaw>" << std::endl;
        file << "     </orient>" << std::endl;
        file << "    </thruster>" << std::endl;
        file << "  </engine>" << std::endl;
        file << std::endl;
    }

    for (unsigned i=0; i<=no_engines; ++i)
    {
        file << "  <tank type=\"FUEL\" number=\"" << i << "\">" << std::endl;
        file << "     <location unit=\"IN\">" << std::endl;
        file << "       <x> " << _tank_loc[X] << " </x>" << std::endl;
        file << "       <y> " << _tank_loc[Y] << " </y>" << std::endl;
        file << "       <z> " << _tank_loc[Z] << " </z>" << std::endl;
        file << "     </location>" << std::endl;
        file << "     <capacity unit=\"LBS\"> " << _tank_capacity << " </capacity>" << std::endl;
        file << "     <contents unit=\"LBS\"> " << _tank_contents << " </contents>" << std::endl;
        file << "  </tank>" << std::endl;
    }

    file << std::endl;
    file << " </propulsion>"<< std::endl;

    return file.str();
}


PistonEngine::PistonEngine(Aeromatic *a, Propulsion *p) : Engine(a, p),
    _max_rpm(2400.0f)
{
    _description.push_back("Piston Engine");
    _inputs.push_back(new Param("Engine Power", _power, _aircraft->_metric, POWER));
    _inputs.push_back(new Param("Maximum Engine RPM", _max_rpm));
    _thruster = new Propeller(this);
}

std::string PistonEngine::engine()
{
    std::stringstream file;

    Engine *propulsion = this;
    float displacement = propulsion->_power *1.9f;

    // Guess the area of one piston (5.125/2)^2 * PI
    float stroke = 4.375f;
    float bore   = 5.125f;

    // Guess the area of one piston (5.125/2)^2 * PI
    float bore_s = powf(bore/2, 2.0f) * PI;
    float n_cylinders = displacement /  (stroke * bore_s);
    n_cylinders = ((n_cylinders < 1) ? 1 : floorf(n_cylinders+0.5f));

    file << "<piston_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <minmp unit=\"INHG\">         10.0 </minmp>" << std::endl;
    file << "  <maxmp unit=\"INHG\">         28.5 </maxmp>" << std::endl;
    file << "    <displacement unit=\"IN3\"> " << displacement << " </displacement>" << std::endl;
    file << "  <maxhp>        " << propulsion->_power << " </maxhp>" << std::endl;
    file << "  <cycles>         4.0 </cycles>" << std::endl;
    file << "  <idlerpm>      700.0 </idlerpm>" << std::endl;
    file << "  <maxrpm>      2800.0 </maxrpm>" << std::endl;
    file << "  <sparkfaildrop>  0.1 </sparkfaildrop>" << std::endl;
    file << "  <volumetric-efficiency> 0.85 </volumetric-efficiency>" << std::endl;
    file << "  <static-friction  unit=\"HP\"> " << (_power * 0.005f) << " </static-friction>" << std::endl;
    file << "  <starter-torque> " << (_power * 0.8f) << " </starter-torque>" << std::endl;
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


TurbineEngine::TurbineEngine(Aeromatic *a, Propulsion *p) : Engine(a, p),
    _injected(false),
    _augmented(false)
{
    _description.push_back("Turbine Engine");
    _inputs.push_back(new Param("Engine Thrust", _power, _aircraft->_metric, THRUST));
    _thruster = new Direct(this);
}

std::string TurbineEngine::engine()
{
    std::stringstream file;

    float max_thrust = _power;
    if (_augmented) {
        max_thrust *= 1.5f;
    }

    file << "<turbine_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <milthrust> " << _power << " </milthrust>" << std::endl;
    if (_augmented) {
        file << "  <maxthrust> " << max_thrust << " </maxthrust>" << std::endl;
    }
    file << "  <bypassratio>     1.0 </bypassratio>" << std::endl;
    file << "  <tsfc>            0.8 </tsfc>" << std::endl;
    if (_augmented) {
        file << "  <atsfc>           1.7 </atsfc>" << std::endl;
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
    file << "         -10000     0     10000   20000   30000   40000   50000   60000" << std::endl;
    file << "     0.0  0.0430  0.0488  0.0528  0.0694  0.0899  0.1183  0.1467  0" << std::endl;
    file << "     0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.1342  0" << std::endl;
    file << "     0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.1203  0" << std::endl;
    file << "     0.6  0.0     0.0     0.0     0.0     0.0276  0.0718  0.1073  0" << std::endl;
    file << "     0.8  0.0     0.0     0.0     0.0     0.0474  0.0868  0.0900  0" << std::endl;
    file << "     1.0  0.0     0.0     0.0     0.0     0.0     0.0552  0.0800  0" << std::endl;
    file << "    </tableData>" << std::endl;
    file << "   </table>" << std::endl;
    file << "  </function>" << std::endl;
    file << std::endl;
    file << "  <function name=\"MilThrust\">" << std::endl;
    file << "   <table>" << std::endl;
    file << "    <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>" << std::endl;
    file << "    <tableData>" << std::endl;
    file << "          -10000       0   10000   20000   30000   40000   50000   60000" << std::endl;
    file << "     0.0   1.2600  1.0000  0.7400  0.5340  0.3720  0.2410  0.1490  0" << std::endl;
    file << "     0.2   1.1710  0.9340  0.6970  0.5060  0.3550  0.2310  0.1430  0" << std::endl;
    file << "     0.4   1.1500  0.9210  0.6920  0.5060  0.3570  0.2330  0.1450  0" << std::endl;
    file << "     0.6   1.1810  0.9510  0.7210  0.5320  0.3780  0.2480  0.1540  0" << std::endl;
    file << "     0.8   1.2580  1.0200  0.7820  0.5820  0.4170  0.2750  0.1700  0" << std::endl;
    file << "     1.0   1.3690  1.1200  0.8710  0.6510  0.4750  0.3150  0.1950  0" << std::endl;
    file << "     1.2   1.4850  1.2300  0.9750  0.7440  0.5450  0.3640  0.2250  0" << std::endl;
    file << "     1.4   1.5941  1.3400  1.0860  0.8450  0.6280  0.4240  0.2630  0" << std::endl;
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

TurbopropEngine::TurbopropEngine(Aeromatic *a, Propulsion *p) : Engine(a, p),
    _water_injection(false)
{
    _description.push_back("Turboprop Engine");
    _inputs.push_back(new Param("Engine Power", _power, _aircraft->_metric, POWER));
    _thruster = new Direct(this);
}

std::string TurbopropEngine::engine()
{
    std::stringstream file;

    // estimate thrust if given power in HP
    // Thrust = bhp * 550 * prop_efficiency / velocity
    // fact = 550 * 0.85 / 195 = 2.24;
    float thrust = _power * 2.24f;

    file << "<turbine_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <milthrust> " << thrust << " </milthrust>" << std::endl;
    file << "  <bypassratio>     0.0  </bypassratio>" << std::endl;
    file << "  <tsfc>            0.55 </tsfc>" << std::endl;
    file << "  <bleed>           0.03 </bleed>" << std::endl;
    file << "  <idlen1>         30.0  </idlen1>" << std::endl;
    file << "  <idlen2>         60.0  </idlen2>" << std::endl;
    file << "  <maxn1>         100.0  </maxn1>" << std::endl;
    file << "  <maxn2>         100.0  </maxn2>" << std::endl;
    file << "  <augmented>         0  </augmented>" << std::endl;
    file << "  <injected>          0  </injected>" << std::endl;
    file << std::endl;
    file << "  <function name=\"IdleThrust\">" << std::endl;
    file << "   <table>" << std::endl;
    file << "    <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>" << std::endl;
    file << "    <tableData>" << std::endl;
    file << "         -10000       0   10000   20000   30000   40000   50000" << std::endl;
    file << "     0.0  0.0430  0.0488  0.0528  0.0694  0.0899  0.1183  0.0" << std::endl;
    file << "     0.2  0.0500  0.0501  0.0335  0.0544  0.0797  0.1049  0.0" << std::endl;
    file << "     0.4  0.0040  0.0047  0.0020  0.0272  0.0595  0.0891  0.0" << std::endl;
    file << "     0.6  0.0     0.0     0.0     0.0276  0.0718  0.0430  0.0" << std::endl;
    file << "     0.8  0.0     0.0     0.0     0.0     0.0174  0.0086  0.0" << std::endl;
    file << "     1.0  0.0     0.0     0.0     0.0     0.0     0.0     0.0" << std::endl;
    file << "   </tableData>" << std::endl;
    file << "   </table>" << std::endl;
    file << "  </function>" << std::endl;
    file << std::endl;
    file << "  <function name=\"MilThrust\">" << std::endl;
    file << "   <table>" << std::endl;
    file << "    <independentVar lookup=\"row\">velocities/mach</independentVar>" << std::endl;
    file << "    <independentVar lookup=\"column\">atmosphere/density-altitude</independentVar>" << std::endl;
    file << "    <tableData>" << std::endl;
    file << "         -10000       0   10000   20000   30000   40000   50000" << std::endl;
    file << "     0.0  1.1260  1.0000  0.7400  0.5340  0.3720  0.2410  0.0" << std::endl;
    file << "     0.2  1.1000  0.9340  0.6970  0.5060  0.3550  0.2310  0.0" << std::endl;
    file << "     0.4  1.0000  0.6410  0.6120  0.4060  0.3570  0.2330  0.0" << std::endl;
    file << "     0.6  0.4430  0.3510  0.2710  0.2020  0.1780  0.1020  0.0" << std::endl;
    file << "     0.8  0.0240  0.0200  0.0160  0.0130  0.0110  0.0100  0.0" << std::endl;
    file << "     1.0  0.0     0.0     0.0     0.0     0.0     0.0     0.0" << std::endl;
    file << "    </tableData>" << std::endl;
    file << "   </table>" << std::endl;
    file << "  </function>" << std::endl;
    file << std::endl;
    file << "</turbine_engine>" << std::endl;

    return file.str();
}

RocketEngine::RocketEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
{
    _description.push_back("Rocket Engine");
    _inputs.push_back(new Param("Engine Thrust", _power, _aircraft->_metric, THRUST));
    _thruster = new Direct(this);
}

std::string RocketEngine::engine()
{
    std::stringstream file;

    file << "<rocket_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << "  <shr>              1.23 </shr>" << std::endl;
    file << "  <max_pc>       86556.00 </max_pc>" << std::endl;
    file << "  <variance>         0.10 </variance>" << std::endl;
    file << "  <prop_eff>         0.67 </prop_eff>" << std::endl;
    file << "  <maxthrottle>      1.00 </maxthrottle>" << std::endl;
    file << "  <minthrottle>      0.40 </minthrottle>" << std::endl;
    file << "  <slfuelflowmax>   91.50 </slfuelflowmax>" << std::endl;
    file << "  <sloxiflowmax>   105.20 </sloxiflowmax>" << std::endl;
    file << "</rocket_engine>" << std::endl;

    return file.str();
}


ElectricEngine::ElectricEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
{
    _description.push_back("Electric Engine");
    _inputs.push_back(new Param("Engine Power", _power, _aircraft->_metric, POWER));
    _inputs.push_back(new Param("Maximum Engine RPM", _max_rpm));
    _thruster = new Propeller(this);
}

std::string ElectricEngine::engine()
{
    std::stringstream file;

    file << "<electric_engine name=\"" << _propulsion->_engine_name << "\">" << std::endl;
    file << " <power unit=\"WATTS\">  " << (_power*HP_TO_KW*1000.0f) << " </power>" << std::endl;
    file << "</electric_engine>" << std::endl;
    return file.str();
}

} /* namespace Aeromatic */

