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
        _propulsion(p) {}

    ~Engine() {
        delete _thruster;
    }

    virtual std::string system() override;
    virtual std::string engine() { return ""; }
    virtual std::string json() {
        return _thruster ? _thruster->json() : "";
    }

    virtual std::string get_thruster() {
        return _thruster->get_name();
    }

public:
    Propulsion *_propulsion = nullptr;
    Thruster *_thruster = nullptr;
    float _power = 1000.0f;

    int _mount_point[8];
};

class PistonEngine : public Engine
{
public:
    PistonEngine(Aeromatic *a, Propulsion *p);
    ~PistonEngine() {}

    std::string engine() override;

    std::string lift() override {
        return _thruster->lift();
    }
    std::string pitch() override {
        return _thruster->pitch();
    }
    std::string roll() override {
        return _thruster->roll();
    }

public:
    float _max_rpm = 2400.0f;
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
    TurbopropEngine(Aeromatic *a, Propulsion *p);
    ~TurbopropEngine() {}

    std::string engine() override;

    std::string lift() override {
        return _thruster->lift();
    }
    std::string pitch() override {
        return _thruster->pitch();
    }
    std::string roll() override {
        return _thruster->roll();
    }

public:
    float _max_rpm = 23500.0f;
    float _oapr = 16.0f;
    float _itt = 800.0f;

private:
    bool _water_injection = false;

    static float const _eng_pwr_t[6][13];
};


class TurbineEngine : public Engine
{
public:
    TurbineEngine(Aeromatic *a, Propulsion *p);
    ~TurbineEngine() {}

    std::string engine() override;
    std::string json() override;

private:
    float _oapr = 16.0f;
    float _bypass_ratio = 1.0f;
    bool _injected = false;
    bool _augmented = false;

    static float const _milthrust_t[8][8];
};

class RocketEngine : public Engine
{
public:
    RocketEngine(Aeromatic *a, Propulsion *p);
    ~RocketEngine() {}

    std::string engine() override;
    std::string json() override;
};

class ElectricEngine : public Engine
{
public:
    ElectricEngine(Aeromatic *a, Propulsion *p);
    ~ElectricEngine() {}

    std::string engine() override;

public:
    float _max_rpm = 2400.0f;
};


class Propulsion : public Engine
{
public:
    Propulsion(Aeromatic *p);
    ~Propulsion();

    void set(const float cg_loc[3]) override;
    std::string comment() override;
    std::string fdm() override;
    std::string json(const float cg_loc[3]) override;
    std::string mass_balance() override;
    std::string system() override;

    std::string lift() override {
        return _propulsion[_ptype]->lift();
    }
    std::string drag() override {
        return _propulsion[_ptype]->drag();
    }
    std::string side() override {
        return _propulsion[_ptype]->side();
    }
    std::string roll() override {
        return _propulsion[_ptype]->roll();
    }
    std::string pitch() override {
        return _propulsion[_ptype]->pitch();
    }
    std::string yaw() override {
        return _propulsion[_ptype]->yaw();
    }

    std::string get_propulsion() {
        return _engine_name;
    }

    std::string propulsion() {
         return _propulsion[_ptype]->engine();
    }

    std::string get_thruster() override {
        return _propulsion[_ptype]->_thruster->get_name();
    }

    std::string thruster() {
         return _propulsion[_ptype]->_thruster->thruster();
    }

    void param_reset() override;
    Param* param_next() override;

    char _engine_name[PARAM_MAX_STRING+1] = "";
public:
    std::vector<Engine*> _propulsion;
    unsigned _ptype = PISTON;

    /* engines */
    unsigned _layout = FWD_FUSELAGE;

    float _eng_loc[8][3];
    float _eng_orient[8][3];
    float _thruster_loc[8][3];
    float _thruster_orient[8][3];

    float _tank_loc[3];
//  float _tank_radius;
    float _tank_capacity;
    float _tank_contents;
    float _fuel_weight;

    float _weight;
    float _diameter;
    float _length;
};

} // namespace Aeromatic

#endif /* __ENGINE_H */
