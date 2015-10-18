// Propuslion.h -- Implements the Aircraft Propulsion types.
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

#ifndef __ENGINE_H
#define __ENGINE_H

#include <stdio.h>

#include "Systems.h"
#include "Thruster.h"

namespace Aeromatic
{

class Propulsion;

class Engine : public System
{
public:
    Engine(Aeromatic *a, Propulsion *p) : System(a, true),
        _propulsion(p),
        _thruster(0),
        _power(1000.0f) {}

    ~Engine() {
        delete _thruster;
    }

    void set(const float* cg_loc) {}
    virtual std::string comment() { return ""; }
    virtual std::string fdm() { return ""; }
    virtual std::string mass_balance() { return ""; }
    virtual std::string system() { return ""; }
    virtual std::string engine() { return ""; }

    virtual std::string lift() { return ""; }
    virtual std::string drag() { return ""; }
    virtual std::string side() { return ""; }
    virtual std::string roll() { return ""; }
    virtual std::string pitch() { return ""; }
    virtual std::string yaw() { return ""; }

    virtual std::string get_propulsion() { return ""; }
    virtual std::string propulsion() { return ""; }

    virtual std::string get_thruster() { return ""; }
    virtual std::string thruster() { return ""; }

public:
    Propulsion *_propulsion;
    Thruster *_thruster;
    float _power;
};

class PistonEngine : public Engine
{
public:
    PistonEngine(Aeromatic *a, Propulsion *p) : Engine(a, p),
        _max_rpm(2400.0f)
    {
        _description.push_back("Piston Engine");
        _inputs.push_back(new Param("Engine Power", &_power));
        _inputs.push_back(new Param("Maximum Engine RPM", &_max_rpm));
        _thruster = new Propeller(this);
    }

    ~PistonEngine() {}

    std::string engine();

    std::string lift() { return ""; }
    std::string drag() { return ""; }
    std::string side() { return ""; }
    std::string roll() { return ""; }
    std::string pitch() { return ""; }
    std::string yaw() { return ""; }

    virtual std::string add_propulsion() { return ""; }

    void set_augmentation(bool v) {}
    void set_water_injection(bool v) {}

public:
    float _max_rpm;
};



/*
 * http://www.pprune.org/archive/index.php/t-10124.html
 * The Metro 23 (Garrett TPE331-12U turbo prop engines) uses a water/methyl
 * alcohol mixture for high density altitude take-offs.
 * It's a mixture of 40% methyl alcohol & 60% distilled or demineralised water.
 */
class TurbopropEngine : public Engine
{
public:
    TurbopropEngine(Aeromatic *a, Propulsion *p) : Engine(a, p),
        _water_injection(false)
    {
        _description.push_back("Turboprop Engine");
        _inputs.push_back(new Param("Engine Power", &_power));
        _thruster = new Direct(this);
    }

    ~TurbopropEngine() {}

    std::string engine();

    std::string lift() { return ""; }
    std::string drag() { return ""; }
    std::string side() { return ""; }
    std::string roll() { return ""; }
    std::string pitch() { return ""; }
    std::string yaw() { return ""; }

    std::string add_propulsion() { return ""; }

    void set_augmentation(bool v) {}

    void set_water_injection(bool v) {
        _water_injection = v;
    }

private:
    bool _water_injection;
};


class TurbineEngine : public Engine
{
public:
    TurbineEngine(Aeromatic *a, Propulsion *p) : Engine(a, p),
        _injected(false),
        _augmented(false)
    {
        _description.push_back("Turbine Engine");
        _inputs.push_back(new Param("Engine Thrust", &_power));
        _thruster = new Direct(this);
    }

    ~TurbineEngine() {}

    std::string engine();

    std::string lift() { return ""; }
    std::string drag() { return ""; }
    std::string side() { return ""; }
    std::string roll() { return ""; }
    std::string pitch() { return ""; }
    std::string yaw() { return ""; }

    std::string add_propulsion() { return ""; }

private:
    bool _injected;
    bool _augmented;
};

class RocketEngine : public Engine
{
public:
    RocketEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
    {
        _description.push_back("Rocket Engine");
        _inputs.push_back(new Param("Engine Thrust", &_power));
        _thruster = new Direct(this);
    }

    ~RocketEngine() {}

    std::string engine();

    std::string lift() { return ""; }
    std::string drag() { return ""; }
    std::string side() { return ""; }
    std::string roll() { return ""; }
    std::string pitch() { return ""; }
    std::string yaw() { return ""; }

    std::string add_propulsion() { return ""; }

    void set_augmentation(bool v) {}
    void set_water_injection(bool v) {}
};

class ElectricEngine : public Engine
{
public:
    ElectricEngine(Aeromatic *a, Propulsion *p) : Engine(a, p)
   {
        _description.push_back("Electric Engine");
        _inputs.push_back(new Param("Engine Power", &_power));
        _inputs.push_back(new Param("Maximum Engine RPM", &_max_rpm));
        _thruster = new Propeller(this);
    }

    ~ElectricEngine() {}

    std::string engine();

    std::string lift() { return ""; }
    std::string drag() { return ""; }
    std::string side() { return ""; }
    std::string roll() { return ""; }
    std::string pitch() { return ""; }
    std::string yaw() { return ""; }

    std::string add_propulsion() { return ""; }

    void set_augmentation(bool v) {}
    void set_water_injection(bool v) {}

public:
    float _max_rpm;
};


class Propulsion : public Engine
{
public:
    Propulsion(Aeromatic *p);
    ~Propulsion();

    void set(const float* cg_loc);
    std::string comment();
    std::string fdm();
    std::string mass_balance();
    std::string system();

    std::string lift() {
        return _propulsion[_ptype]->lift();
    }
    std::string drag() {
        return _propulsion[_ptype]->drag();
    }
    std::string side() {
        return _propulsion[_ptype]->side();
    }
    std::string roll() {
        return _propulsion[_ptype]->roll();
    }
    std::string pitch() {
        return _propulsion[_ptype]->pitch();
    }
    std::string yaw() {
        return _propulsion[_ptype]->yaw();
    }

    std::string get_propulsion() {
        return _engine_name;
    }

    std::string propulsion() {
         return _propulsion[_ptype]->engine();
    }

    std::string get_thruster() {
        return _propulsion[_ptype]->_thruster->get_name();
    }

    std::string thruster() {
         return _propulsion[_ptype]->_thruster->thruster();
    }

    void param_reset();
    Param* param_next();

    char _engine_name[PARAM_MAX_STRING+1];
private:
    Engine *_propulsion[MAX_PROPULSION];
    unsigned _ptype;

    /* engines */
    unsigned _layout;

    float _eng_loc[8][3];
    float _eng_orient[8][3];
    float _thruster_loc[8][3];
    float _thruster_orient[8][3];

    float _tank_loc[3];
//  float _tank_radius;
    float _tank_capacity;
    float _tank_contents;
    float _fuel_weight;
};

} // namespace Aeromatic

#endif /* __ENGINE_H */

