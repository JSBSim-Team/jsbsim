// Thruster.h -- Implements the Propulsion Thruster types.
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


#ifndef __THRUSTER_H
#define __THRUSTER_H

#include <stdio.h>
#include <string>

#include <types.h>
#include "Propulsion.h"

namespace Aeromatic
{

class Engine;
class Propulsion;

class Thruster
{
public:
    Thruster(Propulsion *p);
    virtual ~Thruster();

    virtual void set_thruster(float mrpm = 0.0f) {}
    virtual std::string thruster() { return ""; }

    virtual std::string lift() { return ""; }
    virtual std::string pitch() { return ""; }
    virtual std::string roll() { return ""; }
    virtual std::string json() { return ""; }

    const char* get_name() {
        return _thruster_name;
    }

    void param_reset() {
        _param = 0;
    }

    virtual Param* param_next() {
        return (_param < _inputs.size()) ? _inputs[_inputs_order[_param++]] : 0;
    }

protected:
    char _thruster_name[PARAM_MAX_STRING+1] = "";
    std::vector<std::string> _inputs_order;
    std::map<std::string,Param*> _inputs;
    unsigned _param = 0;
    Propulsion *_propulsion = nullptr;
};


class Direct : public Thruster
{
public:
    Direct(Propulsion *p);
    ~Direct() {}

    std::string thruster() override;
};


class Propeller : public Thruster
{
public:
    Propeller(Propulsion *p);
    ~Propeller() {}

    void set_thruster(float mrpm) override;
    std::string thruster() override;

    std::string lift() override;
    std::string pitch() override;
    std::string roll() override;
    std::string json() override;

    float max_rpm() { return _max_rpm; }
    float Cp0() { return _Cp0; }
    float Ct0() { return _Ct0; }

private:
    bool _fixed_pitch = true;
    unsigned _blades = 2;
    float _diameter = 8.0f;
    float _density_factor = 1.0f;
    float _specific_weight = 172.0f;
    float _engine_rpm = 2700.0f;
    float _max_rpm = 2100.0f;
    float _max_chord = 0.0f;
    float _pitch = 22.0f;

    float _gear_ratio = 1.0f;
    float _static_thrust = 0.0f;
    float _max_thrust = 0.0f;
    float _max_torque = 0.0f;
    float _ixx = 0.0f;
    float _Ct0;
    float _Cp0;

    float _dCLT0;
    float _dCLTalpha;
    float _dCLTmax;
    float _prop_span_left;
    float _prop_span_right;

    struct _performance_t {
        _performance_t(float _J, float _CT, float _CP) :
            J(_J), CT(_CT), CP(_CP) {}

        float J;
        float CT;
        float CP;
    };
    std::vector<_performance_t> _performance;
    unsigned _pitch_levels = 0;

    static float const _CL_t[180];
    static float const _CD_t[180];
//  static float const _thrust_t[23][9];
//  static float const _power_t[23][9];

    void bladeElement();
};

class Nozzle : public Thruster
{
public:
    Nozzle(Propulsion *p);
    ~Nozzle() {}

    std::string thruster() override;

private:
//  float _pe;
    float _diameter = 3.25f;
};

} /* namespace Aeromatic */

#endif /* __THRUSTER_H */

