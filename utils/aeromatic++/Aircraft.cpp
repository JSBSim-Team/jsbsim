// Aircraft.cpp -- Implements a Aeromatic Aircraft type.
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

#include <math.h>
#include <time.h>

#include <locale>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <Systems/Systems.h>
#include "Aircraft.h"
#include "types.h"

namespace Aeromatic
{

Aircraft::Aircraft(Aeromatic *p) :
    _subtype(0),
    _overwrite(true),
    _subdir(true),
    _engines(0),
    _aircraft(p)
{
                    /* general information */
#ifdef WIN32
    std::string dir(getEnv("HOMEPATH"));
#else
    std::string dir(getEnv("HOME"));
#endif
    strCopy(_path, dir);
    _general.push_back(new Param("Output directory", "Specify the output directory for the configuration files", _path));

    _general.push_back(new Param("Create a subdirectory?", "Set to yes to create a new subdirectory with the same name as the aircraft", _subdir));

    _general.push_back(new Param("Overwrite?", "Overwrite files that are already present?", _overwrite));

    strCopy(_name, "my_aircraft");
    _general.push_back(new Param("Aircraft name", "This defines the name and filename of the aircraft", _name));
}

Aircraft::~Aircraft()
{
    for (auto it: _general) {
        delete it;
    }
    _general.clear();

    for (auto it: _systems) {
        delete it;
    }
    _systems.clear();
}

const char*
Aircraft::get_verbose_description(int no_engines)
{
    static char desc[1024];
    size_t num = _subclasses.size();
    std::string rv;

    if (no_engines < 0 || num == 0) {
        rv = _description;
        if (num) rv += " (";
    }

    if (num)
    {
        for (size_t i=0; i<num; i++) {
            rv += _subclasses[i];
            if (i < (num-1)) rv += ", ";
        }
        if (no_engines < 0) rv += ')';
    }

    snprintf(desc, 1024, "%s", rv.c_str());
    return desc;
}

Aeromatic::Aeromatic() : Aircraft()
{
    _inertia[0] = _inertia[1] = _inertia[2] = 0.0;
    _payload = _max_weight;
    _stall_weight = _max_weight;

    _wing.span = 40.0f;
    // Historically speaking most aircraft are modern since the
    // number of aircraft types has exploded since the late sixties.
    if (_atype == LIGHT) {
        _wing.Ktf = 0.87f; // for NACA-6
    } else {
        _wing.Ktf = 0.95f; // for supercritical
    }
    _htail.flap_ratio = 0.27f;	// elevator
    _vtail.flap_ratio = 0.25f;	// rudder

    /* general information */
    _general.push_back(new Param("Use dedicates System files?", "Select no to keep all systems in the aircraft configuration file", _system_files));
    Param* units = new Param("Select a system of measurement", "The options affects all units for length, surface area, speed and thrust/power", _metric);
    _general.push_back(units);
    units->add_option("English (feet, pounds)");
    units->add_option("Metric (meters, kilograms)");

    /* performance, weight and balance */
    _weight_balance.push_back(new Param("Stall speed VS1 (clean, no flaps)", "The stall speed at maximum takeoff weight", _stall_speed, _metric, SPEED));
    _weight_balance.push_back(new Param("Maximum takeoff weight", 0, _max_weight, _metric, WEIGHT));
    _weight_balance.push_back(new Param("Empty weight", _estimate, _empty_weight, _metric, WEIGHT));
    _weight_balance.push_back(new Param("Inertia Ixx", _estimate, _inertia[X], _metric, INERTIA));
    _weight_balance.push_back(new Param("Inertia Iyy", _estimate, _inertia[Y], _metric, INERTIA));
    _weight_balance.push_back(new Param("Inertia Izz", _estimate, _inertia[Z], _metric, INERTIA));

    /* geometry */
    _geometry.push_back(new Param("Length", 0, _length, _metric, LENGTH));
    Param* wingshape = new Param("Select a wing shape", "Wing shapes determaine the lift and drag of the aircraft", _wing.shape);
    _geometry.push_back(wingshape);
    wingshape->add_option("Straight");
    wingshape->add_option("Elliptical");
    wingshape->add_option("Delta");
//  wingshape->add_option("Variable sweep");

    _geometry.push_back(new Param("Wing span", 0, _wing.span, _metric, LENGTH));
    _geometry.push_back(new Param("Wing area", _estimate, _wing.area, _metric, AREA));
    _geometry.push_back(new Param("Wing aspect ratio", _estimate, _wing.aspect));
    _geometry.push_back(new Param("Wing taper ratio", _estimate, _wing.taper));
    _geometry.push_back(new Param("Wing root chord", _estimate, _wing.chord_mean, _metric, LENGTH));
    _geometry.push_back(new Param("Wing incidence", _estimate, _wing.incidence));
    _geometry.push_back(new Param("Wing dihedral", _estimate, _wing.dihedral));
    _geometry.push_back(new Param("Wing sweep (quarter chord)", _estimate, _wing.sweep));
    _geometry.push_back(new Param("Htail area", _estimate, _htail.area, _metric, AREA));
    _geometry.push_back(new Param("Htail arm", _estimate, _htail.arm, _metric, LENGTH));
    _geometry.push_back(new Param("Vtail area", _estimate, _vtail.area, _metric, AREA));
    _geometry.push_back(new Param("Vtail arm", _estimate, _vtail.arm, _metric, LENGTH));

    Param *param = new Param("Type of aircraft", "Select closest aerodynamic type", _atype, MAX_AIRCRAFT);
    _general.push_back(param);

    _aircraft.push_back(new Light(this));
    param->add_option(_aircraft[0]->get_verbose_description());
    _aircraft.push_back(new Performance(this));
    param->add_option(_aircraft[1]->get_verbose_description());
    _aircraft.push_back(new Fighter(this));
    param->add_option(_aircraft[2]->get_verbose_description());
    _aircraft.push_back(new JetTransport(this));
    param->add_option(_aircraft[3]->get_verbose_description());
    _aircraft.push_back(new PropTransport(this));
    param->add_option(_aircraft[4]->get_verbose_description());

    Aircraft::_aircraft = this;


    _CL0 = 0.0f; _CLde = 0.0f; _CLq = 0.0f; _CLadot = 0.0f;
    _CD0 = 0.0f; _CDde = 0.0f; _CDbeta = 0.0f;
    _Kdi = 0.0f; _Mcrit = 0.0f;
    _CYbeta = 0.0f; _CYr = 0.0f; _CYdr = 0.0f;
    _Clp = 0.0f; _Clda = 0.0f; _Cldr = 0.0f;
    _Cmalpha = 0.0f; _Cmde = 0.0f; _Cmq = 0.0f; _Cmadot = 0.0f;
    _Cnbeta = 0.0f; _Cnr = 0.0f; _Cndr = 0.0f; _Cnda = 0.0f;

    _Re.resize(4, 0.0f);
    _alpha.resize(4, 0.0f);

    _CLalpha.resize(3, 0.0f);
    _CLmax.resize(3, 0.0f);

    _CDalpha.resize(4, 0.0f);
    _CYp.resize(4, 0.0f);
    _Clbeta.resize(9, 0.0f);
    _Clr.resize(9, 0.0f);
    _Cnp.resize(4, 0.0f);

    _Cna.resize(8, 0.0f);
    _Cna.at(0) = -1.0f;
    _Cna.at(1) = 1.0f;

    _CLaw.resize(3, 0.0f);
    _CLah.resize(3, 0.0f);
    _CLav.resize(3, 0.0f);
}

Aeromatic::~Aeromatic()
{
    for (auto it : _weight_balance) {
        delete it;
    }
    _weight_balance.clear();

    for (auto it : _geometry) {
        delete it;
    }
    _geometry.clear();

    for (auto it : _aircraft) {
        delete it;
    }
    _aircraft.clear();
}

bool Aeromatic::fdm()
{
    Aircraft *aircraft = _aircraft[_atype];
    systems = _aircraft[_atype]->get_systems();
    _engines = _MIN(_no_engines, 4);
    aircraft->_engines = _engines;


//***** METRICS ***************************************
    _payload = _max_weight;
    _stall_weight = _max_weight;

    // first, estimate wing loading in psf
    wing_loading = aircraft->get_wing_loading();

    // if no wing area given, use wing loading to estimate
    if (_wing.area == 0)
    {
        wingarea_input = false;
        _wing.area = _max_weight / wing_loading;
    }
    else
    {
        wingarea_input = true;
        wing_loading = _max_weight / _wing.area;
    }

    // calculate wing chord
    if (_wing.aspect == 0) {
        _wing.aspect = aircraft->get_aspect_ratio();
    } else {
        _user_wing_data++;
    }

    if (_wing.taper == 0) {
        if (_wing.shape == DELTA) {
            _wing.taper = 2.0f*_wing.span/_wing.area;
        } else {
            _wing.taper = 1.0f;
        }
    }

    if (_wing.chord_mean == 0)
    {
        if (_wing.aspect > 0) {
            _wing.chord_mean = _wing.span / _wing.aspect;
        } else {
            _wing.chord_mean = _wing.area / _wing.span;
        }
    }
    else
    {
        float TR = _wing.taper;

        _wing.chord_mean = 2.0f*_wing.chord_mean*(1.0f+TR-(TR/(1.0f+TR)))/3.0f;
        _user_wing_data++;
    }

    // calculate aspect ratio
    if (_wing.aspect == 0) {
        _wing.aspect = (_wing.span*_wing.span) / _wing.area;
    } else {
        _user_wing_data++;
    }

    if (_wing.de_da == 0) {
        _wing.de_da = 4.0f/(_wing.aspect+2.0f);
    }

    // leading edge sweep
    if (_wing.sweep_le == 0)
    {
        float half_span = 0.5f*_wing.span;		// one wing side
        _wing.sweep_le = atanf((1.0f-_wing.taper)/half_span);
        if (_wing.shape != DELTA) {
            _wing.sweep_le *= 0.5f;	// same for leading and tailing edge
        }
        _wing.sweep_le *= RAD_TO_DEG;
        _wing.sweep_le += _wing.sweep;
    }

    if (_wing.thickness == 0)
    {
        // Hofman equation for t/c
//      float Ws = _stall_weight;
        float Vs = _stall_speed * KNOTS_TO_FPS;
        if (Vs > 0)
        {
            float sweep = _wing.sweep * DEG_TO_RAD;
            float TC = 0.051f * _wing.area * powf(cosf(sweep), 5.0f)/Vs;
            _wing.thickness = TC * _wing.chord_mean;
        }
        else {
            _wing.thickness = 0.15f * _wing.chord_mean;
        }
    }

    // estimate horizontal tail area
    if (_htail.area == 0) {
        _htail.area = _wing.area * aircraft->get_htail_area();
    }

    // estimate distance from CG to horizontal tail aero center
    if (_htail.arm == 0) {
        _htail.arm = _length * aircraft->get_htail_arm();
    }

    if (_htail.aspect == 0) {
        _htail.aspect = 5.0f;	// ht_w * _wing.aspect;
    }
    if (_htail.taper == 0) {
        _htail.taper = 0.5f;
    }

    float ht_w = 0.33f; // sqrtf(_htail.area / _wing.area);
    if (_htail.span == 0) {
        _htail.span = ht_w * _wing.span;
    }

    if (_htail.chord_mean == 0) {
        _htail.chord_mean = _htail.span / _htail.aspect;

        float TR = _htail.taper;
        _htail.chord_mean = 2.0f*_htail.chord_mean*(1.0f+TR-(TR/(1.0f+TR)))/3.0f;
    }

    if (_htail.sweep_le == 0) {
        _htail.sweep_le = 1.05f * _wing.sweep_le;
    }

    if (_htail.thickness == 0) {
        _htail.thickness = 0.085f * _htail.chord_mean;
    }

    if (_htail.de_da == 0) {
        _htail.de_da = 4.0f/(_htail.aspect+2.0f);
    }

    // estimate vertical tail area
    if (_vtail.area == 0) {
        _vtail.area = _wing.area * aircraft->get_vtail_area();
    }

    // estimate distance from CG to vertical tail aero center
    if (_vtail.arm == 0) {
        _vtail.arm = _length * aircraft->get_vtail_arm();
    }

    float vt_w = 0.15f; // sqrtf(_vtail.area / _wing.area*0.5f);
    if (_vtail.span == 0) {
        _vtail.span = vt_w * _wing.span;
    }
    if (_vtail.aspect == 0) {
        _vtail.aspect = 1.7f;	// vt_w * _wing.aspect;
    }
    if (_vtail.taper == 0) {
        _vtail.taper = 0.7f;
    }

    if (_vtail.chord_mean == 0) {
        _vtail.chord_mean = _vtail.span / _vtail.aspect;

        float TR = _vtail.taper;
        _vtail.chord_mean = 2.0f*_vtail.chord_mean*(1.0f+TR-(TR/(1.0f+TR)))/3.0f;
    }

    if (_vtail.sweep_le == 0) {
        _vtail.sweep_le = 1.25f * _wing.sweep_le;
    }

    if (_vtail.thickness == 0) {
        _vtail.thickness = 0.085f * _vtail.chord_mean;
    }

    if (_vtail.de_da == 0) {
        _vtail.de_da = 4.0f/(_vtail.aspect+2.0f);
    }

//***** EMPTY WEIGHT *********************************

    // estimate empty weight, based on max weight
    if (_empty_weight == 0) {
        _empty_weight = _max_weight * aircraft->get_empty_weight();
    }

//***** MOMENTS OF INERTIA ******************************

    // use Roskam's formulae to estimate moments of inertia
    if (_inertia[X] == 0.0f && _inertia[Y] == 0.0f && _inertia[Z] == 0.0f)
    {
        float slugs = (_empty_weight / 32.2f);	// sluggishness
        const float *R = aircraft->get_roskam();

        // These are for an empty airplane
        _inertia[X] = slugs * powf((R[X] * _wing.span / 2), 2);
        _inertia[Y] = slugs * powf((R[Y] * _length / 2), 2);
        _inertia[Z] = slugs * powf((R[Z] * ((_wing.span + _length)/2)/2), 2);
    }

//***** PILOT EYEPOINT *********************************

    // place pilot's eyepoint based on airplane type
    const float *_eyept_loc = aircraft->get_eyept_loc();
    eyept_loc[X] = (_length * _eyept_loc[X]) * FEET_TO_INCH;
    eyept_loc[Y] = _eyept_loc[Y];
    eyept_loc[Z] = _eyept_loc[Z];

//***** AERO REFERENCE POINT **************************

    _aero_rp[X] = (_length - _htail.arm) * FEET_TO_INCH;
    _aero_rp[Y] = 0;
    _aero_rp[Z] = 0;

//***** CG LOCATION ***********************************
    // http://www.rcgroups.com/forums/showatt.php?attachmentid=1651636
    float TR = _wing.taper;
    float Sw = _wing.area;
    float R = _wing.chord_mean;
    float Sh = _htail.area;
    float L = _htail.arm;
    float T = R * TR;
    float P = L*Sh/(3.0f*Sw) - ((R*R + R*T + T*T)/(R+T))/15.0f;

    _cg_loc[X] = _aero_rp[X] - P * FEET_TO_INCH;
    _cg_loc[Y] = 0;
    _cg_loc[Z] = -(_length / 40.0f) * FEET_TO_INCH;
    aircraft->set_cg(_cg_loc, _aero_rp);

//***** PAYLOAD ***************************************

    // A point mass will be placed at the CG weighing
    // 1/2 of the usable aircraft load.
    payload_loc[X] = _cg_loc[X];
    payload_loc[Y] = _cg_loc[Y];
    payload_loc[Z] = _cg_loc[Z];
    _payload -= _empty_weight;

//***** COEFFICIENTS **********************************
    aircraft->set_lift();
    aircraft->set_drag();
    aircraft->set_side();
    aircraft->set_roll();
    aircraft->set_pitch();
    aircraft->set_yaw();

//***** SYSTEMS ***************************************
    // Systems may make use of coefficients
    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled()) {
            systems[i]->set(_cg_loc);
        }
    }


//************************************************
//*                                              *
//*  Print out xml document                      *
//*                                              *
//************************************************
    bool rv = write_XML();
    if (rv) {
        write_JSON();
    }

    return rv;
}

bool
Aeromatic::write_XML()
{
    char str[64];
    time_t t;

    time(&t);
    struct tm ti;
#if defined(_MSC_VER) || defined(__MINGW32__)
    localtime_s(&ti, &t);
#else
    localtime_r(&t, &ti);
#endif
    strftime(str, sizeof(str), "%d %b %Y", &ti);

    _dir = _subdir ? create_dir(_path, _name) : _path;
    if (_dir.empty()) {
        std::cout << "Unable to create directory: " << _path << "/" << _name << std::endl;
        return false;
    }

    std::string systems_dir;
    if (_system_files)
    {
        systems_dir = create_dir(_dir, "Systems");
        if (systems_dir.empty())
        {
            std::cout << "Unable to create directory: " << _dir<< "/Systems" << std::endl;
            _system_files = false;
        }
    }

    std::string fname = _dir + "/" + std::string(_name) + ".xml";

    std::string version = AEROMATIC_VERSION_STR;

    if (!_overwrite && overwrite(fname)) {
        std::cout << "File already exists: " << fname << std::endl;
        return false;
    }

    std::ofstream file;
    file.open(fname.c_str());
    if (file.fail() || file.bad())
    {
        file.close();
        return false;
    }

    file.precision(2);
    file.flags(std::ios::right);
    file << std::fixed << std::showpoint;

    file << "<?xml version=\"1.0\"?>" << std::endl;
    file << "<?xml-stylesheet type=\"text/xsl\" href=\"http://jsbsim.sourceforge.net/JSBSim.xsl\"?>" << std::endl;
    file << std::endl;
    file << "<fdm_config name=\"" << _name << "\" version=\"2.0\" release=\"ALPHA\"" << std::endl;
    file << "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
    file << "   xsi:noNamespaceSchemaLocation=\"http://jsbsim.sourceforge.net/JSBSim.xsd\">" << std::endl;
    file << std::endl;
    file << " <fileheader>" << std::endl;
    file << "  <author> Aeromatic v " << version << " </author>" << std::endl;
    file << "  <filecreationdate> " << str << " </filecreationdate>" << std::endl;
    file << "  <version>$Revision: 1.80 $</version>" << std::endl;
    file << "  <description> Models a " << _name << ". </description>" << std::endl;
    file << " </fileheader>" << std::endl;
    file << std::endl;
    file << "<!--\n  File:     " << _name << ".xml" << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    name:          " << _name << std::endl;
    file << "    type:          ";
    if (_no_engines == 0) file << "No engine ";
    else if (_no_engines == 1) file << "Single engine ";
    else file << "Multi-engine ";
    file << _aircraft[_atype]->get_verbose_description(_no_engines) << std::endl;
    file << "    stall speed:   ";
    if (_stall_speed > 0.5f) {
        file << _stall_speed << "kts" << std::endl;
    } else {
        file << "unspecified" << std::endl;
    }
    file << "    max weight:    " << _max_weight << " lb" << std::endl;
    file << "    length:        " << _length << " ft" << std::endl;
    file << "    wing: " << std::endl;
    file << "     span:         " << _wing.span << " ft" << std::endl;
    file << "     area:         ";
    if (wingarea_input) {
        file << _wing.area << " sq-ft" << std::endl;
    } else {
        file << "unspecified" << std::endl;
    }
    file << "     mean chord:   " << _wing.chord_mean << " ft" << std::endl;
    file << "     aspect ratio: " << _wing.aspect << ":1" << std::endl;
    file << "     taper ratio:  " << _wing.taper << ":1" << std::endl;
    file << "     incidence:    " << _wing.incidence << " degrees" << std::endl;
    file << "     dihedral:     " << _wing.dihedral << " degrees" << std::endl;
    file << "     sweep:        " << _wing.sweep << " degrees" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled()) {
            std::string comment = systems[i]->comment();
            if (!comment.empty()) {
                file << comment << std::endl;
            }
        }
    }

    file << "  Outputs:" << std::endl;
    file << "    wing loading:       " << wing_loading << " lb/sq-ft" << std::endl;
    file << "     - thickness ratio: " << (_wing.thickness/_wing.chord_mean)*100 << "%"  << std::endl;
    file << "    payload:            " << _payload << " lbs" << std::endl;
    file << "    CL-alpha:           " << _CLalpha[0] << " per radian" << std::endl;
    file << "    CL-0:               " << _CL0 << std::endl;
    file << "    CL-max:             " << _CLmax[0] << std::endl;
    file << "    CD-0:               " << _CD0 << std::endl;
    file << "    K:                  " << _Kdi << std::endl;
    file << "    Mcrit:              " << _Mcrit << std::endl << std::endl;

    float rho = 0.0023769f;
    float V = 1.1f*_stall_speed*KNOTS_TO_FPS;
    float qbar = rho*V*V;
    float L = _CLmax[0]*qbar*_wing.area;
    float n = L/_stall_weight;
    float lfg = G*sqrtf(n*n - 1.0f);
    file << "    min. turn radius    " << (V*V/lfg) << " ft" << std::endl;
    file << "    max. turn rate:     " << (lfg/V) << " deg/s" << std::endl;
    file << "-->" << std::endl;
    file << std::endl;

//***** METRICS **********************************

    file << " <metrics>" << std::endl;
    file << "   <wingarea  unit=\"FT2\"> " << std::setw(8) << _wing.area << " </wingarea>" << std::endl;
    file << "   <wingspan  unit=\"FT\" > " << std::setw(8) << _wing.span << " </wingspan>" << std::endl;
    file << "   <wing_incidence unit=\"DEG\"> " << std::setw(2) << _wing.incidence << " </wing_incidence>" << std::endl;
    file << "   <chord     unit=\"FT\" > " << std::setw(8) << _wing.chord_mean << " </chord>" << std::endl;
    file << "   <htailarea unit=\"FT2\"> " << std::setw(8) << _htail.area << " </htailarea>" << std::endl;
    file << "   <htailarm  unit=\"FT\" > " << std::setw(8) << _htail.arm << " </htailarm>" << std::endl;
    file << "   <vtailarea  unit=\"FT2\">" << std::setw(8) << _vtail.area << " </vtailarea>" << std::endl;
    file << "   <vtailarm  unit=\"FT\" > " << std::setw(8) << _vtail.arm << " </vtailarm>" << std::endl;
    file << "   <location name=\"AERORP\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << _aero_rp[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << _aero_rp[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << _aero_rp[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <location name=\"EYEPOINT\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << eyept_loc[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << eyept_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << eyept_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <location name=\"VRP\" unit=\"IN\">" << std::endl;
    file << "     <x>     0.0 </x>" << std::endl;
    file << "     <y>     0.0 </y>" << std::endl;
    file << "     <z>     0.0 </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << " </metrics>"<< std::endl;
    file << std::endl;
    file << " <mass_balance>" << std::endl;
    file << "   <ixx unit=\"SLUG*FT2\">  " << std::setw(8) << _inertia[X] << " </ixx>" << std::endl;
    file << "   <iyy unit=\"SLUG*FT2\">  " << std::setw(8) << _inertia[Y] << " </iyy>" << std::endl;
    file << "   <izz unit=\"SLUG*FT2\">  " << std::setw(8) << _inertia[Z] << " </izz>" << std::endl;
    file << "   <emptywt unit=\"LBS\" >  " << std::setw(8) << _empty_weight << " </emptywt>" << std::endl;
    file << "   <location name=\"CG\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << _cg_loc[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << _cg_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << _cg_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <pointmass name=\"Payload\">" << std::endl;
    file << "    <description> " << _payload << " LBS should bring model up to entered max weight </description>" << std::endl;
    file << "    <weight unit=\"LBS\"> " << (_payload* 0.5f) << " </weight>" << std::endl;
    file << "    <location name=\"POINTMASS\" unit=\"IN\">" << std::endl;
    file << "     <x> " << std::setw(8) << payload_loc[X] << " </x>" << std::endl;
    file << "     <y> " << std::setw(8) << payload_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << std::setw(8) << payload_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "  </pointmass>" << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string mass_balance = systems[i]->mass_balance();
            if (!mass_balance.empty()) {
                file << mass_balance << std::endl;
            }
        }
    }

    file << " </mass_balance>" << std::endl;
    file << std::endl;

//***** FDM_CONFIG ********************************************

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string fdm = systems[i]->fdm();
            if (!fdm.empty()) {
                file << fdm << std::endl;
            }
        }
    }

//***** SYSTEMS ***********************************************

    if (_system_files == true)
    {
        for (unsigned i=0; i<systems.size(); ++i)
        {
            if (systems[i]->enabled())
            {
                std::string system = systems[i]->system();
                if (!system.empty())
                {
                    std::string sname = systems[i]->get_description();
                    std::string sfname = sname + ".xml";

                    if (!_overwrite && overwrite(sfname))
                    {
                        std::cout << "File already exists: " << fname << std::endl;
                        std::cout << "Skipping." << std::endl;
                    }
                    else
                    {
                        file << " <system file=\"" << sfname << "\"/>" << std::endl;

                        std::string sfpath = systems_dir + "/" + sfname;
                        std::ofstream sfile;
                        sfile.open(sfpath.c_str());
                        if (sfile.fail() || sfile.bad())
                        {
                            std::cout << "Error opening file: " << fname << std::endl;
                            std::cout << "Skipping." << std::endl;
                        }
                        else
                        {
                            sfile << "<?xml version=\"1.0\"?>" << std::endl;
                            sfile << "<system name=\"" << sname << "\">" << std::endl;
                            sfile << system << std::endl;
                            sfile << "</system>" << std::endl;
                        }
                        sfile.close();
                    }
                }
            }
        }
        file << std::endl;
    }

    file << " <flight_control name=\"FCS: " << _name << "\">" << std::endl;
    file << std::endl;

    if (_system_files == false)
    {
        for (unsigned i=0; i<systems.size(); ++i)
        {
            if (systems[i]->enabled())
            {
                std::string system = systems[i]->system();
                if (!system.empty()) {
                    file << system << std::endl;
                }
            }
        }
    }

    file << " </flight_control>"<< std::endl;
    file << std::endl;

//***** AERODYNAMICS ******************************************

    file << " <aerodynamics>" << std::endl;
    file << std::endl;

    // ***** LIFT ******************************************

    file << "  <axis name=\"LIFT\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string lift = systems[i]->lift();
            if (!lift.empty()) {
                file << lift << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** DRAG ******************************************

    file << "  <axis name=\"DRAG\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string drag = systems[i]->drag();
            if (!drag.empty()) {
               file << drag << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** SIDE ******************************************

    file << "  <axis name=\"SIDE\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string side = systems[i]->side();
            if (!side.empty()) {
                file << side << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** PITCH *****************************************

    file << "  <axis name=\"PITCH\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string pitch = systems[i]->pitch();
            if (!pitch.empty()) {
                file << pitch << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** ROLL ******************************************

    file << "  <axis name=\"ROLL\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string roll = systems[i]->roll();
            if (!roll.empty()) {
                file << roll << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    // ***** YAW *******************************************

    file << "  <axis name=\"YAW\">" << std::endl;
    file << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string yaw = systems[i]->yaw();
            if (!yaw.empty()) {
                file << yaw << std::endl;
            }
        }
    }

    file << "  </axis>" << std::endl;
    file << std::endl;

    file << " </aerodynamics>" << std::endl;
    file << std::endl;

    file << " <external_reactions>" << std::endl;

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string force = systems[i]->external_force();
            if (!force.empty()) {
                file << force << std::endl;
            }
        }
    }

    file << " </external_reactions>" << std::endl;

    file << std::endl;
    file << "</fdm_config>" << std::endl;

    file.close();

    return true;
}

bool
Aeromatic::write_JSON()
{
    std::string fname = _dir + "/" + std::string(_name) + ".json";

    std::string version = AEROMATIC_VERSION_STR;

    std::ofstream file;
    file.open(fname.c_str());
    if (file.fail() || file.bad())
    {
        file.close();
        return false;
    }

    file.precision(1);
    file.flags(std::ios::left);
    file << std::fixed << std::showpoint;

    file << "{" << std::endl;

    std::string param  = "  \"" + std::string(_name) + "\"";
    file << std::setw(12) << param << ": " << 1.0 << "," << std::endl;
    file << std::endl;

    param = "  \"Sw\"";
    file << std::setw(12) << param << ": " << _wing.area << "," << std::endl;

    param = "  \"cbar\"";
    file << std::setw(12) << param << ": " << _wing.chord_mean << "," << std::endl;

    param = "  \"b\"";
    file << std::setw(12) << param << ": " << _wing.span << "," << std::endl;
    file << std::endl;

    param = "  \"mass\"";
    file << std::setw(12) << param << ": " << 0.9f*_max_weight << "," << std::endl;

    param = "  \"Ixx\"";
    file << std::setw(12) << param << ": " << _inertia[X] << "," << std::endl;

    param = "  \"Iyy\"";
    file << std::setw(12) << param << ": " << _inertia[Y] << "," << std::endl;

    param = "  \"Izz\"";
    file << std::setw(12) << param << ": " << _inertia[Z] << "," << std::endl;

    param = "  \"Ixz\"";
    file << std::setw(12) << param << ": " << 0.0f << "," << std::endl;
    file << std::endl;

    param = "  \"cg\"";
    file << std::setw(12) << param << ": [ "
                          << _aero_rp[X] - _cg_loc[X] << ", "
                          << _aero_rp[Y] - _cg_loc[Y] << ", "
                          << _aero_rp[Z] - _cg_loc[Z] << " ]";

    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled())
        {
            std::string json = systems[i]->json(_cg_loc);
            if (!json.empty())
            {
                file << "," << std::endl << std::endl;
                file << json;
            }
        }
    }
    file << "," << std::endl << std::endl;

    param = "  \"de_max\"";
    file << std::setw(12) << param << ": " << 17.5 << "," << std::endl;
    param = "  \"dr_max\"";
    file << std::setw(12) << param << ": " << 20.0 << "," << std::endl;
    param = "  \"da_max\"";
    file << std::setw(12) << param << ": " << 20.0 << "," << std::endl;
    param = "  \"df_max\"";
    file << std::setw(12) << param << ": " << 40.0 << "," << std::endl;
    file << std::endl;

    file.precision(4);

    // LIFT
    param = "  \"CLmin\"";
    file << std::setw(12) << param << ": " << _CL0 << "," << std::endl;

    param = "  \"CLa\"";
    file << std::setw(12) << param << ": " << _CLalpha[0] << "," << std::endl;

    param = "  \"CLadot\"";
    file << std::setw(12) << param << ": " << _CLadot << "," << std::endl;

    param = "  \"CLq\"";
    file << std::setw(12) << param << ": " << _CLq << "," << std::endl;

    float CLdf = Flaps::_dCLflaps_t[_atype][_engines];
    param = "  \"CLdf\"";
    file << std::setw(12) << param << ": " << CLdf << "," << std::endl;
    file << std::endl;

    // DRAG
    param = "  \"CDmin\"";
    file << std::setw(12) << param << ": " << _CD0 << "," << std::endl;

    param = "  \"CDa\"";
    file << std::setw(12) << param << ": " << _CDalpha[0] << "," << std::endl;

    param = "  \"CDb\"";
    file << std::setw(12) << param << ": " << _CDbeta << "," << std::endl;

    param = "  \"CDi\"";
    file << std::setw(12) << param << ": " << _Kdi << "," << std::endl;

    float CDdf = Flaps::_CDflaps_t[_atype][_engines];
    param = "  \"CDdf\"";
    file << std::setw(12) << param << ": " << CDdf << "," << std::endl;
    file << std::endl;

    // SIDE
    param = "  \"CYb\"";
    file << std::setw(12) << param << ": " << _CYbeta << "," << std::endl;

    param = "  \"CYp\"";
    file << std::setw(12) << param << ": " << _CYp.back() << "," << std::endl;

    param = "  \"CYr\"";
    file << std::setw(12) << param << ": " << _CYr << "," << std::endl;

    param = "  \"CYdr\"";
    file << std::setw(12) << param << ": " << _CYdr << "," << std::endl;
    file << std::endl;

    // ROLL
    param = "  \"Clb\"";
    file << std::setw(12) << param << ": " << _Clbeta.back() << "," << std::endl;

    param = "  \"Clp\"";
    file << std::setw(12) << param << ": " << _Clp << "," << std::endl;

    param = "  \"Clr\"";
    file << std::setw(12) << param << ": " << _Clr.back() << "," << std::endl;

    param = "  \"Clda\"";
    file << std::setw(12) << param << ": " << _Clda << "," << std::endl;

    param = "  \"Cldr\"";
    file << std::setw(12) << param << ": " << _Cldr << "," << std::endl;
    file << std::endl;

    // PITCH
    param = "  \"Cma\"";
    file << std::setw(12) << param << ": " << _Cmalpha << "," << std::endl;

    param = "  \"Cmadot\"";
    file << std::setw(12) << param << ": " << _Cmadot << "," << std::endl;

    param = "  \"Cmq\"";
    file << std::setw(12) << param << ": " << _Cmq << "," << std::endl;

    param = "  \"Cmde\"";
    file << std::setw(12) << param << ": " << _Cmde << "," << std::endl;
    file << std::endl;

    // YAW
    param = "  \"Cnb\"";
    file << std::setw(12) << param << ": " << _Cnbeta << "," << std::endl;

    param = "  \"Cnp\"";
    file << std::setw(12) << param << ": " << _Cnp.back() << "," << std::endl;

    param = "  \"Cnr\"";
    file << std::setw(12) << param << ": " << _Cnr << "," << std::endl;

    param = "  \"Cndr\"";
    file << std::setw(12) << param << ": " << _Cndr << std::endl;

    file << "}" << std::endl;

    file.close();

    return true;
}


// ----------------------------------------------------------------------------

char const* Aeromatic::_estimate = "enter 0 to use estimated value";

#ifdef WIN32
#else
# include <sys/stat.h>
#endif

std::string Aeromatic::create_dir(std::string path, std::string subdir)
{
    // Create Engines directory
    std::string dir = path + "/" + subdir;
#ifdef WIN32
    if (!PathFileExists(dir.c_str())) {
        if (CreateDirectory(dir.c_str(), NULL) == 0) {
            dir.clear();
        }
    }
#else
    struct stat sb;
    if (stat(dir.c_str(), &sb))
    {
        int mode = strtol("0755", 0, 8);
        if (mkdir(dir.c_str(), mode) == -1) {
            dir.clear();
        }
    }
#endif

    return dir;
}

bool Aeromatic::overwrite(std::string path)
{
    bool rv = true;

#ifdef WIN32
    if (!PathFileExists(path.c_str())) {
        rv = false;
    }
#else
    struct stat sb;
    if (stat(path.c_str(), &sb) != 0) {
        rv = false;
    }
#endif

    return rv;
}

} /* namespace Aeromatic */
