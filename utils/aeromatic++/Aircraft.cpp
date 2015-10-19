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

#include <math.h>
#include <string.h>

#include <ctime>
#include <locale>
#include <iostream>
#include <fstream>

#include <Systems/Systems.h>
#include "Aircraft.h"
#include "types.h"

namespace Aeromatic
{

Aircraft::Aircraft(Aeromatic *p) :
    _subtype(0),
    _engines(0),
    _aircraft(p)
{
                    /* general information */
    snprintf(_name, PARAM_MAX_STRING, "my_aircraft");
    _general.push_back(new Param("Aircraft name", _name));
}

Aircraft::~Aircraft()
{
}

Aeromatic::Aeromatic() : Aircraft(this),
    _atype(LIGHT),
    _metric(0),
    _max_weight(10000.0),
    _empty_weight(0),
    _length(40.0),
    _wing_span(40.0),
    _wing_area(0),
    _wing_chord(0),
    _wing_incidence(0),
    _htail_area(0),
    _htail_arm(0),
    _vtail_area(0),
    _vtail_arm(0),
    _payload(10000.0),
    _no_engines(0)
{
    _inertia[0] = _inertia[1] = _inertia[2] = 0.0;
    _payload = _max_weight;

                        /* general information */
#if defined(WIN32)
    std::string dir(getenv("HOMEPATH"));
#else
    std::string dir(getenv("HOME"));
#endif
    snprintf(_path, PARAM_MAX_STRING, "%s", dir.c_str());
    _general.push_back(new Param("Output directory", _path));

                /* general information */
    Param* units = new Param("Chose a system of measurement", &_metric);
    _general.push_back(units);
    units->add_option("English (feet, pounds)");
    units->add_option("Metric (meters, kilograms)");

    /* weight and balance */
    _weight_balance.push_back(new Param("Maximum takeoff weight", &_max_weight, &_metric, WEIGHT));
    _weight_balance.push_back(new Param("Empty weight (enter 0 to use estimated value)", &_empty_weight, &_metric, WEIGHT));
    _weight_balance.push_back(new Param("Inertia Ixx (enter 0 to use estimated value)", &_inertia[X], &_metric, INERTIA));
    _weight_balance.push_back(new Param("Inertia Iyy (enter 0 to use estimated value)", &_inertia[Y], &_metric, INERTIA));
    _weight_balance.push_back(new Param("Inertia Izz (enter 0 to use estimated value)", &_inertia[Z], &_metric, INERTIA));

    /* geometry */
    _geometry.push_back(new Param("Length", &_length, &_metric, LENGTH));
    _geometry.push_back(new Param("Wing span", &_wing_span, &_metric, LENGTH));
    _geometry.push_back(new Param("Wing area (enter 0 to use estimated value)",
                            &_wing_area, &_metric, AREA));
    _geometry.push_back(new Param("Wing chord (enter 0 to use estimated value)",
                            &_wing_chord, &_metric, LENGTH));
    _geometry.push_back(new Param("Wing incidence (enter 0 to use estimated value)",
                            &_wing_incidence));
    _geometry.push_back(new Param("Htail area (enter 0 to use estimated value)",
                            &_htail_area, &_metric, AREA));
    _geometry.push_back(new Param("Htail arm (enter 0 to use estimated value)",
                            &_htail_arm, &_metric, LENGTH));
    _geometry.push_back(new Param("Vtail area (enter 0 to use estimated value)",
                            &_vtail_area, &_metric, AREA));
    _geometry.push_back(new Param("Vtail arm (enter 0 to use estimated value)",
                            &_vtail_arm, &_metric, LENGTH));

    Param *param = new Param("Type of aircraft (Select closest aerodynamic type)", &_atype);
    _general.push_back(param);
    _aircraft[0] = new Light(this);
    param->add_option(_aircraft[0]->get_description());
    _aircraft[1] = new Performance(this);
    param->add_option(_aircraft[1]->get_description());
    _aircraft[2] = new Fighter(this);
    param->add_option(_aircraft[2]->get_description());
    _aircraft[3] = new JetTransport(this);
    param->add_option(_aircraft[3]->get_description());
    _aircraft[4] = new PropTransport(this);
    param->add_option(_aircraft[4]->get_description());
}

Aeromatic::~Aeromatic()
{
    for (unsigned i=0; i<MAX_AIRCRAFT; ++i) {
        delete _aircraft[i];
    }

    std::vector<Param*>::iterator it;
    for(it = _general.begin(); it != _general.end(); ++it) {
        delete *it;
    }
    for(it = _weight_balance.begin(); it != _weight_balance.end(); ++it) {
        delete *it;
    }
    for(it = _geometry.begin(); it != _geometry.end(); ++it) {
        delete *it;
    }
}

bool Aeromatic::fdm()
{
    Aircraft *aircraft = _aircraft[_atype];
    std::vector<System*> systems = _aircraft[_atype]->get_systems();

    aircraft->_engines = _MIN(_no_engines, 4);

//***** METRICS ***************************************
    _payload = _max_weight;

    // first, estimate wing loading in psf
    float wing_loading = aircraft->get_wing_loading();

    // if no wing area given, use wing loading to estimate
    bool wingarea_input;
    if (_wing_area == 0)
    {
        wingarea_input = false;
        _wing_area = _max_weight / wing_loading;
    }
    else
    {
        wingarea_input = true;
        wing_loading = _max_weight / _wing_area;
    }

    // calculate wing chord
    _wing_chord = _wing_area / _wing_span;

    // calculate aspect ratio
//  float aspect_ratio = _wing_span / _wing_chord;

    // for now let's use a standard 2 degrees wing incidence
    if (_wing_incidence == 0) {
        _wing_incidence = 2.0;
    }

    // estimate horizontal tail area
    _htail_area = _wing_area * aircraft->get_htail_area();

    // estimate distance from CG to horizontal tail aero center
    _htail_arm = _length * aircraft->get_htail_arm();

    // estimate vertical tail area
    _vtail_area = _wing_area * aircraft->get_vtail_area();

    // estimate distance from CG to vertical tail aero center
    _vtail_arm = _length * aircraft->get_vtail_arm();

//***** EMPTY WEIGHT *********************************

    // estimate empty weight, based on max weight
    _empty_weight = _max_weight * aircraft->get_empty_weight();


//***** MOMENTS OF INERTIA ******************************

    // use Roskam's formulae to estimate moments of inertia
    if (_inertia[X] == 0.0f && _inertia[Y] == 0.0f && _inertia[Z] == 0.0f)
    {
        float slugs = (_empty_weight / 32.2);	// sluggishness
        const float *R = aircraft->get_roskam();

        // These are for an empty airplane
        _inertia[X] = slugs * powf((R[X] * _wing_span / 2), 2);
        _inertia[Y] = slugs * powf((R[Y] * _length / 2), 2);
        _inertia[Z] = slugs * powf((R[Z] * ((_wing_span + _length)/2)/2), 2);
    }

//***** CG LOCATION ***********************************

    float cg_loc[3];
    cg_loc[X] = (_length - _htail_arm) * FEET_TO_INCH;
    cg_loc[Y] = 0;
    cg_loc[Z] = -(_length / 40.0) * FEET_TO_INCH;

//***** AERO REFERENCE POINT **************************

    float aero_rp[3];
    aero_rp[X] = cg_loc[X];
    aero_rp[Y] = 0;
    aero_rp[Z] = 0;

//***** PILOT EYEPOINT *********************************

    // place pilot's eyepoint based on airplane type
    const float *_eyept_loc = aircraft->get_eyept_loc();
    float eyept_loc[3];
    eyept_loc[X] = (_length * _eyept_loc[X]) * FEET_TO_INCH;
    eyept_loc[Y] = _eyept_loc[Y];
    eyept_loc[Z] = _eyept_loc[Z];

//***** PAYLOAD ***************************************

    // A point mass will be placed at the CG weighing
    // 1/2 of the usable aircraft load.
    float payload_loc[3];
    payload_loc[X] = cg_loc[X];
    payload_loc[Y] = cg_loc[Y];
    payload_loc[Z] = cg_loc[Z];
    _payload -= _empty_weight;

//***** SYSTEMS ***************************************
    for (unsigned i=0; i<systems.size(); ++i)
    {
        if (systems[i]->enabled()) {
            systems[i]->set(cg_loc);
        }
    }

//***** COEFFICIENTS **********************************
    aircraft->set_lift();
    aircraft->set_drag();
    aircraft->set_side();
    aircraft->set_roll();
    aircraft->set_pitch();
    aircraft->set_yaw();

//************************************************
//*                                              *
//*  Print out xml document                      *
//*                                              *
//************************************************

    char str[64];
    std::time_t t = std::time(NULL);
//  std::strftime(str, sizeof(str), "%Y-%m-%d", std::localtime(&t));
    std::strftime(str, sizeof(str), "%d %b %Y", std::localtime(&t));

    std::ofstream file;
    std::string fname = std::string(_path) + "/" + std::string(_name) + ".xml";

    std::string version = AEROMATIC_VERSION_STR;

    file.open(fname.c_str());
    if (file.fail() || file.bad())
    {
        file.close();
        return false;
    }

    file << "<?xml version=\"1.0\"?>" << std::endl;
    file << "<?xml-stylesheet type=\"text/xsl\" href=\"http://jsbsim.sourceforge.net/JSBSim.xsl\"?>" << std::endl;
    file << std::endl;
    file << "<fdm_config name=\"" << _name << "\" version=\"2.0\" release=\"ALPHA\"" << std::endl;
    file << "   xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl;
    file << "   xsi:noNamespaceSchemaLocation=\"http://jsbsim.sourceforge.net/JSBSim.xsd\">" << std::endl;
    file << std::endl;
    file << " <fileheader>" << std::endl;
    file << "  <author> Aeromatic v " << version << " </author>" << std::endl;
    file << "  <filecreationdate> " << str << "</filecreationdate>" << std::endl;
    file << "  <version>$Revision: 1.6 $</version>" << std::endl;
    file << "  <description> Models a " << _name << ". </description>" << std::endl;
    file << " </fileheader>" << std::endl;
    file << std::endl;
    file << "<!--\n  File:     " << _name << ".xml" << std::endl;
    file << "  Inputs:" << std::endl;
    file << "    name:          " << _name << std::endl;
    file << "    type:          ";
    switch(_atype)
    {
    case LIGHT:
        if (_no_engines == 0) {
            file << "glider" << std::endl;
        } else {
            file << "light commuter with " << _no_engines << " engines" << std::endl;
        }
        break;
    case PERFORMANCE:
        file << "WWII fighter, subsonic sport, aerobatic" << std::endl;
        break;
    case FIGHTER:
        file << _no_engines << " engine transonic/supersonic fighter" << std::endl;
        break;
    case JET_TRANSPORT:
        file << _no_engines << " engine transonic transport" << std::endl;
        break;
    case PROP_TRANSPORT:
        file << "multi-engine prop transport" << std::endl;
        break;
    }
    file << "    max weight:    " << _max_weight << " lb" << std::endl;
    file << "    wing span:     " << _wing_span << " ft" << std::endl;
    file << "    length:        " << _length << " ft" << std::endl;
    file << "    wing area:     ";
    if (wingarea_input) {
        file << _wing_area << " sq-ft" << std::endl;
    } else {
        file << "unspecified" << std::endl;
    }
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
    file << "    wing loading:  " << wing_loading << " lb/sq-ft" << std::endl;
    file << "    payload:       " << _payload << " lbs" << std::endl;
    file << "    CL-alpha:      " << _CLalpha << " per radian" << std::endl;
    file << "    CL-0:          " << _CL0 << std::endl;
    file << "    CL-max:        " << _CLmax << std::endl;
    file << "    CD-0:          " << _CD0 << std::endl;
    file << "    K:             " << _K << std::endl;
    file << "-->" << std::endl;
    file << std::endl;

//***** METRICS **********************************

    file << " <metrics>" << std::endl;
    file << "   <wingarea  unit=\"FT2\"> " << _wing_area << " </wingarea>" << std::endl;
    file << "   <wingspan  unit=\"FT\" > " << _wing_span << " </wingspan>" << std::endl;
    file << "   <wing_incidence>       " << _wing_incidence << " </wing_incidence>" << std::endl;
    file << "   <chord     unit=\"FT\" > " << _wing_chord << " </chord>" << std::endl;
    file << "   <htailarea unit=\"FT2\"> " << _htail_area << " </htailarea>" << std::endl;
    file << "   <htailarm  unit=\"FT\" > " << _htail_arm << " </htailarm>" << std::endl;
    file << "   <vtailarea  unit=\"FT\" > " << _vtail_area << " </vtailarea>" << std::endl;
    file << "   <vtailarm  unit=\"FT\" > " << _vtail_arm << " </vtailarm>" << std::endl;
    file << "   <location name=\"AERORP\" unit=\"IN\">" << std::endl;
    file << "     <x> " << aero_rp[X] << " </x>" << std::endl;
    file << "     <y> " << aero_rp[Y] << " </y>" << std::endl;
    file << "     <z> " << aero_rp[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <location name=\"EYEPOINT\" unit=\"IN\">" << std::endl;
    file << "     <x> " << eyept_loc[X] << " </x>" << std::endl;
    file << "     <y> " << eyept_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << eyept_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <location name=\"VRP\" unit=\"IN\">" << std::endl;
    file << "     <x> 0.0 </x>" << std::endl;
    file << "     <y> 0.0 </y>" << std::endl;
    file << "     <z> 0.0 </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << " </metrics>"<< std::endl;
    file << std::endl;
    file << " <mass_balance>" << std::endl;
    file << "   <ixx unit=\"SLUG*FT2\">  " << _inertia[X] << " </ixx>" << std::endl;
    file << "   <iyy unit=\"SLUG*FT2\">  " << _inertia[Y] << " </iyy>" << std::endl;
    file << "   <izz unit=\"SLUG*FT2\">  " << _inertia[Z] << " </izz>" << std::endl;
    file << "   <emptywt unit=\"LBS\" >  " << _empty_weight << " </emptywt>" << std::endl;
    file << "   <location name=\"CG\" unit=\"IN\">" << std::endl;
    file << "     <x> " << cg_loc[X] << " </x>" << std::endl;
    file << "     <y> " << cg_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << cg_loc[Z] << " </z>" << std::endl;
    file << "   </location>" << std::endl;
    file << "   <pointmass name=\"Payload\">" << std::endl;
    file << "    <description> " << _payload << " LBS should bring model up to entered max weight </description>" << std::endl;
    file << "    <weight unit=\"LBS\"> " << _payload << " </weight>" << std::endl;
    file << "    <location name=\"POINTMASS\" unit=\"IN\">" << std::endl;
    file << "     <x> " << payload_loc[X] << " </x>" << std::endl;
    file << "     <y> " << payload_loc[Y] << " </y>" << std::endl;
    file << "     <z> " << payload_loc[Z] << " </z>" << std::endl;
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

    file << " <flight_control name=\"FCS: " << _name << "\">" << std::endl;
    file << std::endl;

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

} /* namespace Aeromatic */

