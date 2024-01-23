// types.cpp -- Implements a type conversions.
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

#include <iostream>
#include <sstream>
#include <string>
#include <cctype>
#include <stdlib.h>

#include "types.h"

namespace Aeromatic
{

void strCopy(char *b, std::string str)
{
    std::size_t l = str.copy(b, PARAM_MAX_STRING);
    b[l] = 0;
}

#ifdef WIN32
char*
getEnv(const char*name)
{
   static char _key[256] = "";
   char *rv = NULL;
   DWORD res, err;

   res = GetEnvironmentVariable(name, (LPSTR)&_key, 256);
   err = GetLastError();
   if (res || !err) {
       rv = (char*)&_key;
   }

   return rv;
}
#endif

template <>
Param::Param(const char* n, const char *h,  bool& v, const bool& c, unsigned t) :
    _name(n),
    _help(h ? h : _unspecified),
    _ptype(PARAM_BOOL),
    _convert(c),
    _utype(t)
{
    _value.b = &v;
}

template <>
Param::Param(const char* n, const char *h, unsigned& v, const bool& c, unsigned t) :
    _name(n),
    _help(h ? h : _unspecified),
    _ptype(PARAM_INT),
    _convert(c),
    _utype(t)
{ 
    maxval = -1;
    _value.i = &v; 
}

Param::Param(const char* n, const char *h, unsigned& v, unsigned mv, const bool& c, unsigned t) :
    _name(n),
    _help(h ? h : _unspecified),
    _ptype(PARAM_INT),
    _convert(c),
    _utype(t)
{
    maxval = mv-1;
    _value.i = &v;
}

template <>
Param::Param(const char* n, const char *h, float& v, const bool& c, unsigned t) :
    _name(n),
    _help(h ? h : _unspecified),
    _ptype(PARAM_FLOAT),
    _convert(c),
    _utype(t)
{
    v = 0.0f;
    _value.f = &v;
}

template <>
Param::Param(const char* n, const char *h, char* v, const bool& c, unsigned t) :
    _name(n),
    _help(h ? h : _unspecified),
    _ptype(PARAM_STRING),
    _convert(c),
    _utype(t)
{
    _value.s = v;
}

void Param::set(std::string& v)
{
    switch(_ptype)
    {
    case PARAM_BOOL:
        if (v == "y" || v == "yes" || v == "true") *_value.b = true;
        else if (v == "n" || v == "no" || v == "false") *_value.b = false;
        else *_value.b = (strtol(v.c_str(), NULL, 10) == 0) ? false : true;
        break;
    case PARAM_INT:
        *_value.i = strtol(v.c_str(), NULL, 10);
        if (*_value.i > maxval) *_value.i = maxval;
        break;
    case PARAM_FLOAT:
#if (_MSC_VER)
        *_value.f = stof(v, NULL);
#else
        *_value.f = strtof(v.c_str(), NULL);
#endif
        if (_convert) *_value.f = (*_value.f * _cvt_t[_utype].fact);
        break;
    case PARAM_STRING:
        strCopy(_value.s, v);
        break;
    case PARAM_UNSUPPORTED:
    default:
        break;
    }
}

std::string Param::get(float fact)
{
    std::ostringstream oss;
    std::string str;

    oss.precision(2);
    oss.flags(std::ios::right);
    oss << std::fixed << std::showpoint;

    switch(_ptype)
    {
    case PARAM_BOOL:
        str = (*_value.b == false) ? "no" : "yes";
        break;
    case PARAM_INT:
        oss << (*_value.i);
        str = oss.str();
        break;
    case PARAM_FLOAT:
        if (_convert) fact /= _cvt_t[_utype].fact;
        oss << (*_value.f * fact);
        str = oss.str();
        break;
    case PARAM_STRING:
        str = _value.s;
        break;
    case PARAM_UNSUPPORTED:
    default:
        break;
    }
    return str;
}

std::string Param::get(float value, unsigned utype, bool convert)
{
    std::ostringstream oss;
    std::string str;
    float fact;

    fact = 1.0f;
    if (convert) fact /= _cvt_t[utype].fact;

    oss.precision(2);
    oss.flags(std::ios::right);
    oss << std::fixed << std::showpoint;
    oss << (value * fact);
    str = oss.str();

    return str;
}


std::string Param::get_unit(bool upper, unsigned utype, bool convert)
{
    std::string str = _cvt_t[utype].name[convert];
    if (upper) {
        std::transform(str.begin(), str.end(), str.begin(),
                       [](unsigned char c){ return std::toupper(c); }
                      );
    }
    return str;
}

std::string Param::get_nice()
{
    std::string str = get() + " " + get_unit(false, _utype, _convert);
    return str;
}

std::string Param::get_nice(float value, unsigned utype, bool convert, bool upper)
{
    std::string str = get(value, utype, convert) + " " + get_unit(upper, utype, convert);
    return str;
}

// ---------------------------------------------------------------------------

bool const Param::_false = false;
std::string const Param::_unspecified = "not available";

Param::__cvt const Param::_cvt_t[MAX_UNITS] =
{
    { 1.0f,                     {          "",       "" } },	// UNSPECIFIED
    { KG_TO_LBS,		{       "lbs",     "kg" } },	// WEIGHT
    { KGM2_TO_SLUGFT2,		{  "slug*ft2",  "kg*m2" } } ,	// INERTIA
    { METER_TO_FEET,		{        "ft",      "m" } },	// LENGTH
    { M2_TO_FT2,		{       "ft2",     "m2" } },	// AREA
    { LITER_TO_CUBIC_INCH,	{       "in3",      "l" } },	// VOLUME
    { KM_H_TO_KNOTS,		{        "kt",   "km/h" } },	// SPEED
    { KW_TO_HP,			{        "hp",     "kW" } },	// POWER
    { KNEWTON_TO_LBS,		{       "lbs",     "kN" } }, 	// THRUST
    { N_M2_TO_PSF,		{ "lbs/sq-ft",   "N/m2" } },	// LOADING
    { N_M_TO_LBS_FT, 		{     "lbs/ft",    "N/m" } },	// SPRING
    { N_M_TO_LBS_FT,		{ "lbs/ft/sec", "N/m/sec"} } 	// DAMPING
};

} /* namespace Aeromatic */


