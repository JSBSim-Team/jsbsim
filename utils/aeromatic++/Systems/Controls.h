// Controls.h -- Implements the Aircraft Control types.
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


#ifndef __CONTROLS_H
#define __CONTROLS_H

#include <string>
#include <vector>

#include <Aircraft.h>
#include <types.h>

namespace Aeromatic
{

class CableControls : public System
{
public:
    CableControls(Aeromatic *p) : System(p, true) {
       _description.push_back("Conventional Controls");
    }
    ~CableControls() {}

    void set(const float* cg_loc);
    std::string system();

    std::string lift();
    std::string drag();
    std::string side();
    std::string roll();
    std::string pitch();
    std::string yaw();

    std::string _print_vector(std::vector<float>& C);
    void _get_CLaw(std::vector<float>& CLaw, Aeromatic::_lift_device_t &wing);
};

/* Conventional Controls with Yaw Damper */
class YawDamper : public System
{
public:
    YawDamper(Aeromatic *p) : System(p, true) {
        _control = new CableControls(p);
        _description.push_back("Conventional with Yaw Damper");
    }
    ~YawDamper() {
        delete _control;
    }

    void set(const float* cg_loc) {
        _control->set(cg_loc);
    }
    std::string system();

    std::string lift() {
        return _control->lift();
    }
    std::string drag() {
        return _control->drag();
    }
    std::string side() {
        return _control->side();
    }
    std::string roll() {
        return _control->roll();
    }
    std::string pitch() {
        return _control->pitch();
    }
    std::string yaw() {
        return _control->yaw();
    }

private:
    System *_control;
};

class FlyByWire : public System
{
public:
    FlyByWire(Aeromatic *p) : System(p) {
       _control = new CableControls(p);
       _description.push_back("Fly By Wire Controls");
    }
    ~FlyByWire() {
        delete _control;
    }

    void set(const float* cg_loc) {
        _control->set(cg_loc);
    }
    std::string system();

    std::string lift() {
        return _control->lift();
    }
    std::string drag() {
        return _control->drag();
    }
    std::string side() {
        return _control->side();
    }
    std::string roll() {
        return _control->roll();
    }
    std::string pitch() {
        return _control->pitch();
    }
    std::string yaw() {
        return _control->yaw();
    }

private:
    System *_control;
};

/* Choice between Convectional Cable Controls and Fly By Wire */
class Controls : public System
{
public:
    Controls(Aeromatic *p);
    ~Controls();

    void set(const float* cg_loc) {
        _control[_ctype]->set(cg_loc);
    }
    std::string comment();
    std::string fdm() {
        return _control[_ctype]->fdm();
    }
    std::string mass_balance() {
        return _control[_ctype]->mass_balance();
    }
    std::string system() {
        return _control[_ctype]->system();
    }

    std::string lift() {
        return _control[_ctype]->lift();
    }
    std::string drag() {
        return _control[_ctype]->drag();
    }
    std::string side() {
        return _control[_ctype]->side();
    }
    std::string roll() {
        return _control[_ctype]->roll();
    }
    std::string pitch() {
        return _control[_ctype]->pitch();
    }
    std::string yaw() {
        return _control[_ctype]->yaw();
    }

    void param_reset() 
    {
        _param = 0;
        _control[_ctype]->param_reset();
    }

    Param* param_next()
    {
        Param* rv = 0;
        if (_enabled)
        {
            if (_param < _inputs.size()) {
                rv = _inputs[_param++];
            } else {
                rv = _control[_ctype]->param_next();
            }
        }
        return rv;
    }

public:
    System *_control[MAX_CONTROL];
    unsigned _ctype;
};


} /* namespace Aeromatic */

#endif /* __CONTROLS_H */

