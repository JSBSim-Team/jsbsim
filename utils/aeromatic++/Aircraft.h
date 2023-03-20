// Aircraft.h -- Implements a Aircraft type.
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

#ifndef __AIRCRAFT_H
#define __AIRCRAFT_H

#include <string>
#include <vector>
#include <map>

#include <Systems/Systems.h>
#include "types.h"

namespace Aeromatic
{

class Aeromatic;

class Aircraft
{
public:
    Aircraft(Aeromatic *p = 0);
    virtual ~Aircraft();

    virtual bool fdm() { return false; }

    const char* get_description() {
        return _description;
    };

    const char* get_verbose_description(int no_engines = -1);

    const std::vector<std::string>& get_warnings() {
        return _warnings;
    }

    const std::vector<std::string>& get_alerts() {
        return _alerts;
    }

    const std::vector<std::string>& get_subclasses() {
        return _subclasses;
    }

    virtual const std::vector<System*>& get_systems() {
        return _systems;
    }

    virtual float get_fuselage_diameter() { return 0.0f; }
    virtual float get_wing_loading() { return 0.0f; }
    virtual float get_aspect_ratio() { return 0.0f; }
    virtual float get_htail_area() { return 0.0f; }
    virtual float get_htail_arm() { return 0.0f; }
    virtual float get_vtail_area() { return 0.0f; }
    virtual float get_vtail_arm() { return 0.0f; }
    virtual float get_empty_weight() { return 0.0f; }
    virtual const float* get_roskam() { return 0; }
    virtual const float* get_eyept_loc() { return 0; }
    virtual float get_gear_loc() { return 0.0f; }
    virtual float get_fuel_weight() { return 0.0f; }

    virtual void set_cg(float cg[3], float aero[3]) {
        for (auto system : _systems) system->set_cg(cg, aero);
    }
    virtual void set_lift() {}
    virtual void set_drag() {}
    virtual void set_side() {}
    virtual void set_roll() {}
    virtual void set_pitch() {}
    virtual void set_yaw() {}

public:
//***** USER INPUTS ************************************

    /* general information */
    std::string _dir;
    char _path[PARAM_MAX_STRING+1];
    char _name[PARAM_MAX_STRING+1];
    unsigned _subtype = 0;
    bool _overwrite = true;
    bool _subdir = true;

//***** USER INPUTS ************************************

    std::vector<std::string> _general_order;
    std::map<std::string,Param*> _general;
    unsigned _engines = 0;

    /* FCS, Flight Control System */
    std::vector<System*> _systems;

protected:
    Aeromatic *_aircraft;
    const char* _description;
    std::vector<std::string> _subclasses;

    std::vector<std::string> _warnings;
    std::vector<std::string> _alerts;
};


/* Glider, Light Single, Light Twin */
class Light : public Aircraft
{
public:
    Light(Aeromatic *p);
    ~Light() {}

    float get_fuselage_diameter() override {
        return _fuselage_diameter_t[_subtype][_engines];
    }
    float get_wing_loading() override {
        return _wing_loading_t[_subtype][_engines];
    }
    float get_aspect_ratio() override {
        return _aspect_ratio_t[_subtype][_engines];
    }
    float get_htail_area() override {
        return _htail_area_t[_subtype][_engines];
    }
    float get_htail_arm() override {
        return _htail_arm_t[_subtype][_engines];
    }
    float get_vtail_area() override {
        return _vtail_area_t[_subtype][_engines];
    }
    float get_vtail_arm() override {
        return _vtail_arm_t[_subtype][_engines];
    }
    float get_empty_weight() override {
        return _empty_weight_t[_subtype][_engines];
    }
    const float* get_roskam() override {
        return _roskam_t[_subtype][_engines];
    }
    const float* get_eyept_loc() override {
        return _eyept_loc_t[_subtype][_engines];
    }
    float get_gear_loc() override {
        return _gear_loc_t[_subtype][_engines];
    }
    float get_fuel_weight() override {
        return _fuel_weight_t[_subtype][_engines];
    }

    void set_lift() override;
    void set_drag() override;
    void set_side() override;
    void set_roll() override;
    void set_pitch() override;
    void set_yaw() override;

protected:
    static float const _fuselage_diameter_t[1][5];
    static float const _wing_loading_t[1][5];
    static float const _aspect_ratio_t[1][5];
    static float const _htail_area_t[1][5];
    static float const _htail_arm_t[1][5];
    static float const _vtail_area_t[1][5];
    static float const _vtail_arm_t[1][5];
    static float const _empty_weight_t[1][5];
    static float const _roskam_t[1][5][3];
    static float const _eyept_loc_t[1][5][3];
    static float const _gear_loc_t[1][5];
    static float const _fuel_weight_t[1][5];

    static float const _CL0_t[1][5];
    static float const _CLalpha_t[1][5];
    static float const _CLmax_t[1][5];

    static float const _CD0_t[1][5];
    static float const _K_t[1][5];
    static float const _Mcrit_t[1][5];

    static float const _Cmalpha_t[1][5];
    static float const _Cmde_t[1][5];
    static float const _Cmq_t[1][5];
    static float const _Cmadot_t[1][5];

    static float const _Clda_t[1][5];
    static float const _Cnda_t[1][5];
};

/* WWII Fighter or subsonic racer/aerobatic */
class Performance : public Aircraft
{
public:
    Performance(Aeromatic *p);
    ~Performance() {}

    float get_fuselage_diameter() override {
        return _fuselage_diameter_t[_subtype][_engines];
    }
    float get_wing_loading() override {
        return _wing_loading_t[_subtype][_engines];
    }
    float get_aspect_ratio() override {
        return _aspect_ratio_t[_subtype][_engines];
    }
    float get_htail_area() override {
        return _htail_area_t[_subtype][_engines];
    }
    float get_htail_arm() override {
        return _htail_arm_t[_subtype][_engines];
    }
    float get_vtail_area() override {
        return _vtail_area_t[_subtype][_engines];
    }
    float get_vtail_arm() override {
        return _vtail_arm_t[_subtype][_engines];
    }
    float get_empty_weight() override {
        return _empty_weight_t[_subtype][_engines];
    }
    const float* get_roskam() override {
        return _roskam_t[_subtype][_engines];
    }
    const float* get_eyept_loc() override {
        return _eyept_loc_t[_subtype][_engines];
    }
    float get_gear_loc() override {
        return _gear_loc_t[_subtype][_engines];
    }
    float get_fuel_weight() override {
        return _fuel_weight_t[_subtype][_engines];
    }

    void set_lift() override;
    void set_drag() override;
    void set_side() override;
    void set_roll() override;
    void set_pitch() override;
    void set_yaw() override;

protected:
    static float const _fuselage_diameter_t[1][5];
    static float const _wing_loading_t[1][5];
    static float const _aspect_ratio_t[1][5];
    static float const _htail_area_t[1][5];
    static float const _htail_arm_t[1][5];
    static float const _vtail_area_t[1][5];
    static float const _vtail_arm_t[1][5];
    static float const _empty_weight_t[1][5];
    static float const _roskam_t[1][5][3];
    static float const _eyept_loc_t[1][5][3];
    static float const _gear_loc_t[1][5];
    static float const _fuel_weight_t[1][5];

    static float const _CL0_t[1][5];
    static float const _CLalpha_t[1][5];
    static float const _CLmax_t[1][5];

    static float const _CD0_t[1][5];
    static float const _K_t[1][5];
    static float const _Mcrit_t[1][5];

    static float const _Cmalpha_t[1][5];
    static float const _Cmde_t[1][5];
    static float const _Cmq_t[1][5];
    static float const _Cmadot_t[1][5];

    static float const _Clda_t[1][5];
    static float const _Cnda_t[1][5];
};

/* Transonic or Supersonic Fighter */
class Fighter : public Aircraft
{
public:
    Fighter(Aeromatic *p);
    ~Fighter() {}

    float get_fuselage_diameter() override {
        return _fuselage_diameter_t[_subtype][_engines];
    }
    float get_wing_loading() override {
        return _wing_loading_t[_subtype][_engines];
    }
    float get_aspect_ratio() override {
        return _aspect_ratio_t[_subtype][_engines];
    }
    float get_htail_area() override {
        return _htail_area_t[_subtype][_engines];
    }
    float get_htail_arm() override {
        return _htail_arm_t[_subtype][_engines];
    }
    float get_vtail_area() override {
        return _vtail_area_t[_subtype][_engines];
    }
    float get_vtail_arm() override {
        return _vtail_arm_t[_subtype][_engines];
    }
    float get_empty_weight() override {
        return _empty_weight_t[_subtype][_engines];
    }
    const float* get_roskam() override {
        return _roskam_t[_subtype][_engines];
    }
    const float* get_eyept_loc() override {
        return _eyept_loc_t[_subtype][_engines];
    }
    float get_gear_loc() override {
        return _gear_loc_t[_subtype][_engines];
    }
    float get_fuel_weight() override {
        return _fuel_weight_t[_subtype][_engines];
    }

    void set_lift() override;
    void set_drag() override;
    void set_side() override;
    void set_roll() override;
    void set_pitch() override;
    void set_yaw() override;

protected:
    static float const _fuselage_diameter_t[1][5];
    static float const _wing_loading_t[1][5];
    static float const _aspect_ratio_t[1][5];
    static float const _htail_area_t[1][5];
    static float const _htail_arm_t[1][5];
    static float const _vtail_area_t[1][5];
    static float const _vtail_arm_t[1][5];
    static float const _empty_weight_t[1][5];
    static float const _roskam_t[1][5][3];
    static float const _eyept_loc_t[1][5][3];
    static float const _gear_loc_t[1][5];
    static float const _fuel_weight_t[1][5];

    static float const _CL0_t[1][5];
    static float const _CLalpha_t[1][5];
    static float const _CLmax_t[1][5];

    static float const _CD0_t[1][5];
    static float const _K_t[1][5];
    static float const _Mcrit_t[1][5];

    static float const _Cmalpha_t[1][5];
    static float const _Cmde_t[1][5];
    static float const _Cmq_t[1][5];
    static float const _Cmadot_t[1][5];

    static float const _Clda_t[1][5];
    static float const _Cnda_t[1][5];
};

/* Transonic Transport */
class JetTransport : public Aircraft
{
public:
    JetTransport(Aeromatic *p);
    ~JetTransport() {}

    float get_fuselage_diameter() override {
        return _fuselage_diameter_t[_subtype][_engines];
    }
    float get_wing_loading() override {
        return _wing_loading_t[_subtype][_engines];
    }
    float get_aspect_ratio() override {
        return _aspect_ratio_t[_subtype][_engines];
    }
    float get_htail_area() override {
        return _htail_area_t[_subtype][_engines];
    }
    float get_htail_arm() override {
        return _htail_arm_t[_subtype][_engines];
    }
    float get_vtail_area() override {
        return _vtail_area_t[_subtype][_engines];
    }
    float get_vtail_arm() override {
        return _vtail_arm_t[_subtype][_engines];
    }
    float get_empty_weight() override {
        return _empty_weight_t[_subtype][_engines];
    }
    const float* get_roskam() override {
        return _roskam_t[_subtype][_engines];
    }
    const float* get_eyept_loc() override {
        return _eyept_loc_t[_subtype][_engines];
    }
    float get_gear_loc() override {
        return _gear_loc_t[_subtype][_engines];
    }
    float get_fuel_weight() override {
        return _fuel_weight_t[_subtype][_engines];
    }

    void set_lift() override;
    void set_drag() override;
    void set_side() override;
    void set_roll() override;
    void set_pitch() override;
    void set_yaw() override;

protected:
    static float const _fuselage_diameter_t[1][5];
    static float const _wing_loading_t[1][5];
    static float const _aspect_ratio_t[1][5];
    static float const _htail_area_t[1][5];
    static float const _htail_arm_t[1][5];
    static float const _vtail_area_t[1][5];
    static float const _vtail_arm_t[1][5];
    static float const _empty_weight_t[1][5];
    static float const _roskam_t[1][5][3];
    static float const _eyept_loc_t[1][5][3];
    static float const _gear_loc_t[1][5];
    static float const _fuel_weight_t[1][5];

    static float const _CL0_t[1][5];
    static float const _CLalpha_t[1][5];
    static float const _CLmax_t[1][5];

    static float const _CD0_t[1][5];
    static float const _K_t[1][5];
    static float const _Mcrit_t[1][5];

    static float const _Cmalpha_t[1][5];
    static float const _Cmde_t[1][5];
    static float const _Cmq_t[1][5];
    static float const _Cmadot_t[1][5];

    static float const _Clda_t[1][5];
    static float const _Cnda_t[1][5];
};

/* Prop Transport */
class PropTransport : public Aircraft
{
public:
    PropTransport(Aeromatic *p);
    ~PropTransport() {}

    float get_fuselage_diameter() override {
        return _fuselage_diameter_t[_subtype][_engines];
    }
    float get_wing_loading() override {
        return _wing_loading_t[_subtype][_engines];
    }
    float get_aspect_ratio() override {
        return _aspect_ratio_t[_subtype][_engines];
    }
    float get_htail_area() override {
        return _htail_area_t[_subtype][_engines];
    }
    float get_htail_arm() override {
        return _htail_arm_t[_subtype][_engines];
    }
    float get_vtail_area() override {
        return _vtail_area_t[_subtype][_engines];
    }
    float get_vtail_arm() override {
        return _vtail_arm_t[_subtype][_engines];
    }
    float get_empty_weight() override {
        return _empty_weight_t[_subtype][_engines];
    }
    const float* get_roskam() override {
        return _roskam_t[_subtype][_engines];
    }
    const float* get_eyept_loc() override {
        return _eyept_loc_t[_subtype][_engines];
    }
    float get_gear_loc() override {
        return _gear_loc_t[_subtype][_engines];
    }
    float get_fuel_weight() override {
        return _fuel_weight_t[_subtype][_engines];
    }

    void set_lift() override;
    void set_drag() override;
    void set_side() override;
    void set_roll() override;
    void set_pitch() override;
    void set_yaw() override;

protected:
    static float const _fuselage_diameter_t[1][5];
    static float const _wing_loading_t[1][5];
    static float const _aspect_ratio_t[1][5];
    static float const _htail_area_t[1][5];
    static float const _htail_arm_t[1][5];
    static float const _vtail_area_t[1][5];
    static float const _vtail_arm_t[1][5];
    static float const _empty_weight_t[1][5];
    static float const _roskam_t[1][5][3];
    static float const _eyept_loc_t[1][5][3];
    static float const _gear_loc_t[1][5];
    static float const _fuel_weight_t[1][5];

    static float const _CL0_t[1][5];
    static float const _CLalpha_t[1][5];
    static float const _CLmax_t[1][5];

    static float const _CD0_t[1][5];
    static float const _K_t[1][5];
    static float const _Mcrit_t[1][5];

    static float const _Cmalpha_t[1][5];
    static float const _Cmde_t[1][5];
    static float const _Cmq_t[1][5];
    static float const _Cmadot_t[1][5];

    static float const _Clda_t[1][5];
    static float const _Cnda_t[1][5];
};


class Aeromatic : public Aircraft
{
public:
    Aeromatic();
    ~Aeromatic();

    static std::string create_dir(std::string path, std::string subdir);
    static bool overwrite(std::string path);

    const std::vector<System*>& get_systems() override {
        return _aircraft[_atype]->get_systems();
    }

    bool fdm() override;
    bool write_fgfs();
    bool write_JSON();

    bool write_XML();
    bool write_FCS(std::ofstream* file);
    bool write_aero(std::ofstream* file);
    bool write_extern(std::ofstream* file);

    float get_fuselage_diameter() override {
        return _aircraft[_atype]->get_fuselage_diameter();
    }
    float get_wing_loading() override {
        return _aircraft[_atype]->get_wing_loading();
    }
    float get_aspect_ratio() override {
        return _aircraft[_atype]->get_aspect_ratio();
    }
    float get_htail_area() override {
        return _aircraft[_atype]->get_htail_area();
    }
    float get_htail_arm() override {
        return _aircraft[_atype]->get_htail_arm();
    }
    float get_vtail_area() override {
        return _aircraft[_atype]->get_vtail_area();
    }
    float get_vtail_arm() override {
        return _aircraft[_atype]->get_vtail_arm();
    }
    float get_empty_weight() override {
        return _aircraft[_atype]->get_empty_weight();
    }
    const float* get_roskam() override {
        return _aircraft[_atype]->get_roskam();
    }
    const float* get_eyept_loc() override {
        return _aircraft[_atype]->get_eyept_loc();
    }
    float get_gear_loc() override {
        return _aircraft[_atype]->get_gear_loc();
    }
    float get_fuel_weight() override {
        return _aircraft[_atype]->get_fuel_weight();
    }

    void set_lift() override {
        _aircraft[_atype]->set_lift();
    }
    void set_drag() override {
        _aircraft[_atype]->set_drag();
    }
    void set_side() override {
        _aircraft[_atype]->set_side();
    }
    void set_roll() override {
        _aircraft[_atype]->set_roll();
    }
    void set_pitch() override {
        _aircraft[_atype]->set_pitch();
    }
    void set_yaw() override {
        _aircraft[_atype]->set_yaw();
    }

public:
    std::vector<std::string> _weight_balance_order;
    std::map<std::string,Param*> _weight_balance;

    std::vector<std::string> _geometry_order;
    std::map<std::string,Param*> _geometry;

public:
    std::vector<Aircraft*> _aircraft;
    unsigned _atype = LIGHT;
    unsigned _ptype = PISTON;
    unsigned _steering = 0;
    bool _retractable = false;

    bool _system_files = true;
    bool _metric = false;
    bool _split = false;

    /* performance, weight and balance */
    float _aero_rp[3] = { 0.0f, 0.0f, 0.0f };
    float _cg_loc[3] = { 0.0f, 0.0f, 0.0f };
    float _stall_speed = 0.0f;
    float _stall_weight = 0.0f;
    float _max_weight = 10000.0f;
    float _empty_weight = 0.0f;
    float _inertia[3] = { 0.0f, 0.0f, 0.0f };	// xx, yy, zz

    /* geometry */
    float _length = 40.0f;
    float _payload = 10000.0f;

    int _user_wing_data = -2;
    struct _lift_device_t
    {
        // Inputs
        unsigned shape = STRAIGHT;
        float arm = 0.0f;
        float span = 0.0f;
        float area = 0.0f;
        float aspect = 0.0f;	// ratio
        float taper = 1.0f;	// ratio
        float chord_mean = 0.0f;
        float incidence = 2.0f;
        float dihedral = 0.0f;
        float sweep = 0.0f;
        float sweep_le = 0.0f;	// sweep leading edge
        float efficiency = 0.0f;
        float thickness = 0.0f;
        float flap_ratio = 0.0f;

        // *** currently unused **
        float twist = 0.0f;
        float camber = 0.0f;

        // Calculated
        float de_da = 0.0f;

        // Korn technology factor: 0.97 for NACA6, 0.65 for supercritical
        float Ktf = 0.0f;
    } _lift_device;

    _lift_device_t _wing;
    _lift_device_t _htail;
    _lift_device_t _vtail;

    /* array index, can not be greater than 4 */
    unsigned _no_engines = 0;
    bool _wing_mounted_engines = false;

public:
    /* Coefficients */
    std::vector<float> _Re, _alpha;

    std::vector<float> _CLalpha, _CLmax;		// for mach 0, 1 and 2
    float _CL0, _CLde, _CLq, _CLadot;

    std::vector<float> _CDalpha;
    float _CD0, _CDde, _CDbeta;
    float _Kdi, _Mcrit;

    std::vector<float> _CYp;
    float _CYbeta, _CYr, _CYdr;

    std::vector<float> _Clbeta, _Clr;
    float _Clp, _Clda, _Cldr;

    float _Cmalpha, _Cmde, _Cmq, _Cmadot;

    std::vector<float> _Cna, _Cnp;
    float _Cnbeta, _Cnr, _Cndr, _Cnda;

public:
    static char const* _estimate;
    std::vector<float> _CLaw, _CLah, _CLav;

private:
    bool wingarea_input;
    float wing_loading;
    float eyept_loc[3];
    float payload_loc[3];
    std::vector<System*> systems;
};

} /* namespace Aeromatic */

#endif /* __AIRCRAFT_H */
