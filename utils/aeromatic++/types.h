// types.h -- Implements type conversions
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

#ifndef _TYPES_H
#define _TYPES_H 1

#include "config.h"

#ifdef _WIN32
# ifndef WIN32
#  define WIN32
# endif
#endif

#ifdef WIN32
# include <windows.h>
# include <Shlwapi.h>
#endif

#include <string>
#include <vector>
#include <algorithm>

#define DO_MKSTR(X)		#X
#define MKSTR(X)		DO_MKSTR(X)

#define AEROMATIC_VERSION_STR   MKSTR(AEROMATIC_MAJOR_VERSION)"." \
                                MKSTR(AEROMATIC_MINOR_VERSION)"." \
                                MKSTR(AEROMATIC_MICRO_VERSION)

#define G			32.0f
#define SPEED_OF_SOUND		1125.0f

#define PI			3.14159265358979323846f
#define DEG_TO_RAD		0.01745329251994329547f
#define RAD_TO_DEG		(1.0f/DEG_TO_RAD)

#define FEET_TO_INCH		12.0f
#define INCH_TO_FEET		0.08333333f

#define CUBIC_INCH_TO_LITER	61.02398343f
#define LITER_TO_CUBINC_INCH	0.016387f

#define LBS_TO_KG		0.4535f
#define KG_TO_LBS		2.205f

#define LB_TO_SLUGS		0.0310809502f
#define SLUGS_TO_LB		32.1740486f

#define KG_M2_TO_SLUG_FT2	0.737562142f
#define SLUG_FT2_TO_KG_M2	1.355817962f

#define METER_TO_FEET		3.28084f
#define FEET_TO_METER		0.3048f

#define M2_TO_FT2		10.7639104f
#define FT2_TO_M2		0.09290304f

// one horsepower equals 745.69987 Watts 
#define KW_TO_HP		1.341f
#define HP_TO_KW		0.7457f

#define KNETWON_TO_LBS		224.808943f
#define LBS_TO_KNEWTON		0.00444822162f

#define MPH_TO_KNOTS		0.868976242f
#define KNOTS_TO_MPH		1.15077945f
#define KMPH_TO_KNOTS		0.539956803f
#define KNOTS_TO_KMPH		1.852f

#define KNOTS_TO_FPS		1.68780839895013f
#define FPS_TO_KNOTS		0.592483801f

#define _MAX(a,b)		(((a)>(b)) ? (a) : (b))
#define _MIN(a,b)		(((a)<(b)) ? (a) : (b))
#define _MINMAX(a,b,c)		(((a)>(c)) ? (c) : (((a)<(b)) ? (b) : (a)))

namespace Aeromatic
{

enum indicators
{
    X = 0,
    Y,
    Z,

    MAIN = 0,
    NOSE,
    TAIL,

    PITCH = 0,
    YAW,
    ROLL
};

enum AircraftType
{
    LIGHT = 0,
    PERFORMANCE,
    FIGHTER,
    JET_TRANSPORT,
    PROP_TRANSPORT,
//  BIPLANE,

    MAX_AIRCRAFT
};

enum WingType
{
    STRAIGHT = 0,
    ELLIPTICAL,
    DELTA,
    VARIABLE_SWEEP,

    MAX_WING
};

enum ControlsType
{
    CONVENTIONAL = 0,
    YAW_DAMPER,
    FLY_BY_WIRE,

    MAX_CONTROL
};

enum EngineType
{
    PISTON = 0,
    TURBINE,
    TURBOPROP,
    ROCKET,
    ELECTRIC,

    MAX_ENGINE
};

enum EngineLayout
{
    FWD_FUSELAGE = 0,
    MID_FUSELAGE,
    AFT_FUSELAGE,
    WINGS,
    WINGS_AND_TAIL,
    WINGS_AND_NOSE,

    MAX_ENGINE_LAYOUT,

    FUSELAGE = 0,
    LEFT_WING = 1,
    RIGHT_WING = 2
};

enum SteeringType
{
    STEERING = 0,
    CASTERING,
    FIXED,

    MAX_STEERING
};

enum ParamType
{
    PARAM_BOOL = 0,
    PARAM_INT,
    PARAM_FLOAT,
    PARAM_STRING,
    PARAM_UNSUPPORTED,

    PARAM_MAX_STRING = 64
};

enum ParamUnit
{
    UNSPECIFIED = 0,
    WEIGHT,
    INERTIA,
    LENGTH,
    AREA,
    VOLUME,
    SPEED,
    POWER,
    THRUST,

    MAX_UNITS
};

void strCopy(char *b, std::string str);
#ifdef WIN32
char* getEnv(const char*);
#else
# define getEnv(a)		getenv(a)
#endif

class Param
{
public:
    template <typename T>
    Param (const char* n, const char *h, T& v, const bool& c = _false, unsigned t = 0);

    template <typename T>
    Param (const char* n, const char *h, T* v, const bool& c = _false, unsigned t = 0);

    Param (const char* n, const char *h, unsigned& v, unsigned mv, const bool& c = _false, unsigned t = 0);

    ~Param() {}

    std::string& name() { return _name; }
    std::string& help() { return _help; }

    void set(std::string& s);
    std::string get();

    enum ParamType get_type() { return ParamType(_ptype); }
    const char* get_units() {
        return _cvt_t[_utype].name[_convert];
    }

    // options add a 'one of n' selection type
    unsigned no_options() { return (unsigned)_options.size(); }
    void add_option(const char* s) { _options.push_back(s); }
    void add_option(std::string &s) { _options.push_back(s); }
    std::string& get_option(unsigned n) { return _options[n]; }

private:
    std::string _name;
    std::string _help;
    std::vector<std::string> _options;
    unsigned _ptype;
    const bool& _convert;
    unsigned _utype;
    unsigned maxval;
    union {
        unsigned* i;
        float* f;
        bool* b;
        char* s;
    } _value;

    struct __cvt
    {
        float fact;
        const char *name[2];
    };

    static __cvt const _cvt_t[MAX_UNITS];
    static std::string const _unspecified;
    static bool const _false;
};

} /* namespace Aeromatic */

#endif /* _TYPES_H */

