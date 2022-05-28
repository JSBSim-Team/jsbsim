// Systems.h -- Implements a Aeromatic Systems.
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


#ifndef __SYSTEMS_H
#define __SYSTEMS_H

#include <stdio.h>
#include <memory>
#include <string>

#include <types.h>

namespace Aeromatic
{

class Aeromatic;

class System
{
public:
    System(Aeromatic *p, bool e = false) :
        _aircraft(p),
        _enabled(e) {}

    virtual ~System() {
        for (auto it : _inputs) {
            delete it.second;
        }
        _inputs.clear();
    }

    /* construct the configuration file(s) */
    virtual void set(const float cg_loc[3]) {}
    virtual std::string comment() { return ""; }
    virtual std::string fdm() { return ""; }
    virtual std::string json(const float cg_loc[3]) { return ""; }
    virtual std::string mass_balance() { return ""; }
    virtual std::string system() { return ""; }
    virtual std::string external_force() { return ""; }

    virtual std::string lift() { return ""; }
    virtual std::string drag() { return ""; }
    virtual std::string side() { return ""; }
    virtual std::string roll() { return ""; }
    virtual std::string pitch() { return ""; }
    virtual std::string yaw() { return ""; }

    size_t no_descriptors() {
        return _description.size();
    }

    std::string& get_description() {
        return _description[_subtype];
    }

    virtual void set_cg(float cg[3], const float aero[3]) {}

    virtual void param_reset() {
        _param = 0;
    }

    virtual Param* param_next() {
        return ((!_param || _enabled) && (_param < _inputs_order.size())) ? _inputs[_inputs_order[_param++]] : 0;
    }

    bool enabled() {
        return _enabled;
    }

public:
    Aeromatic* _aircraft;
    std::vector<std::string> _description;
    bool _enabled;

protected:
    std::vector<std::string> _inputs_order;
    std::map<std::string,Param*> _inputs;
    unsigned _param = 0;
    int _subtype = 0;

    static char const* _supported;
};


class Flaps : public System
{
public:
    Flaps(Aeromatic *p) : System(p, true) {
        _description.push_back("Flaps");
        _inputs_order.push_back("Flaps");
        _inputs["Flaps"] = new Param(_description[0].c_str(), _supported, _enabled);
    }
    ~Flaps() {}

    void set(const float cg_loc[3]);
    std::string system();

    std::string lift();
    std::string drag();

public:
    static float const _dCLflaps_t[MAX_AIRCRAFT][5];
    static float const _CDflaps_t[MAX_AIRCRAFT][5];

    float _K;
};


class LandingGear : public System
{
public:
    LandingGear(Aeromatic *p);
    ~LandingGear() {}

    void set(const float cg_loc[3]);
    std::string comment();
    std::string fdm();
    std::string json(const float cg_loc[3]);
    std::string system();

    std::string drag();

    void set_cg(float cg[3], const float aero[3]) {
        if (_taildragger) cg[X] = aero[X] + (aero[X] - cg[X]);
    }

private:
    bool _taildragger;
    bool _retractable;
    unsigned _steering;

    float _cg_loc[3];
    float _gear_loc[3][3];
    float _gear_spring[3];
    float _gear_damp[3];
    float _gear_static;
    float _gear_dynamic;
    float _gear_rolling;
    float _gear_max_steer;

    static float const _CDgear_t[MAX_AIRCRAFT][5];
    static float const _CDfixed_gear_t[MAX_AIRCRAFT][5];
};


class ArrestorHook : public System
{
public:
    ArrestorHook(Aeromatic *p) : System(p) {
        _description.push_back("Arrestor Hook");
        _inputs_order.push_back("Arrestor Hook");
        _inputs["Arrestor Hook"] = new Param(_description[0].c_str(), _supported, _enabled);
    }
    ~ArrestorHook() {}

    std::string system();
    std::string external_force();
};


/* Was called Speedbrake in Aeromatic 2 */
class Spoilers : public System
{
public:
    Spoilers(Aeromatic *p) : System(p) {
        _description.push_back("Spoilers");
        _inputs_order.push_back("Spoilers");
        _inputs["Spoilers"] = new Param(_description[0].c_str(), _supported, _enabled);
        _inputs_order.push_back("differentialSpoiler");
        _inputs["differentialSpoiler"] = new Param("Is the spoiler differential?", "Differential spoilers are used for faster roll rate", _differential);
    }
    ~Spoilers() {}

    std::string system();

    std::string lift();
    std::string drag();
    std::string roll();

private:
    bool _differential;

    static float const _dCLspoilers_t[MAX_AIRCRAFT][5];
};


class Speedbrake : public System
{
public:
    Speedbrake(Aeromatic *p) : System(p) {
        _description.push_back("Speedbrake");
        _inputs_order.push_back("Speedbrake");
        _inputs["Speedbrake"]  = new Param(_description[0].c_str(), _supported, _enabled);
    }
    ~Speedbrake() {}

    std::string system();

    std::string drag();

private:
    static float const _CDspeedbrake_t[MAX_AIRCRAFT][5];
};


class ThrustReverse : public System
{
public:
    ThrustReverse(Aeromatic *p) : System(p) {
        _description.push_back("Thrust Reverse");
        _inputs_order.push_back("Thrust Reverse");
        _inputs["Thrust Reverse"] = new Param(_description[0].c_str(), _supported, _enabled);
    }
    ~ThrustReverse() {}

    std::string system();
};


class Chute : public System
{
public:
    Chute(Aeromatic *p) : System(p) {
        _description.push_back("Chute");
        _inputs_order.push_back("Chute");
        _inputs["Chute"] = new Param(_description[0].c_str(), _supported, _enabled);
    }
    ~Chute() {}

    std::string system();
    std::string external_force();

protected:
    static float const _CDchute_t[MAX_AIRCRAFT][5];
    static float const _ChuteArea_t[MAX_AIRCRAFT][5];
};

class DragChute : public Chute
{
public:
    DragChute(Aeromatic *p) : Chute(p) {
        _description.clear();
        _description.push_back("Drag Chute");
    }
    ~DragChute() {}
};

class RescueChute : public Chute
{
public:
    RescueChute(Aeromatic *p) : Chute(p) {
        _description.clear();
        _description.push_back("Rescue Chute (Ballistic Recovery System)");
    }
    ~RescueChute() {}
};

class Catapult : public System
{
public:
    Catapult(Aeromatic *p) : System(p) {
        _description.push_back("Catapult");
        _inputs_order.push_back("Catapult");
        _inputs["Catapult"] = new Param(_description[0].c_str(), _supported, _enabled);
    }
    ~Catapult() {}

    std::string system();
    std::string external_force();
};

} /* namespace Aeromatic */

#endif /* __SYSTEMS_H */

