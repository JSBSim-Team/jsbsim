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

    const char* get_name() {
        return _thruster_name;
    }

    void param_reset() {
        _param = 0;
    }

    virtual Param* param_next() {
        return (_param < _inputs.size()) ? _inputs[_param++] : 0;
    }

protected:
    char _thruster_name[PARAM_MAX_STRING+1];
    std::vector<Param*> _inputs;
    unsigned _param;
    Propulsion *_propulsion;
};


class Direct : public Thruster
{
public:
    Direct(Propulsion *p);
    ~Direct() {}

    std::string thruster();
};


class Propeller : public Thruster
{
public:
    Propeller(Propulsion *p);
    ~Propeller() {}

    void set_thruster(float mrpm);
    std::string thruster();

    std::string lift();
    std::string pitch();
    std::string roll();

    float max_rpm() { return _max_rpm; }
    float Cp0() { return _Cp0; }
    float Ct0() { return _Ct0; }

private:
    float _engine_rpm;
    bool _fixed_pitch;
    unsigned _blades;
    float _diameter;
    float _max_rpm;
    float _max_chord;
    float _pitch;

    float _gear_ratio;
    float _static_thrust;
    float _ixx;
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
    unsigned _pitch_levels;

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

    std::string thruster();

private:
    float _pe;
    float _diameter;
};

} /* namespace Aeromatic */

#endif /* __THRUSTER_H */

