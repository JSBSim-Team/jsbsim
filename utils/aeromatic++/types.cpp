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

#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include "types.h"

namespace Aeromatic
{

template <>
Param::Param<bool>(const char* n, bool* v) : _name(n), _ptype(PARAM_BOOL)
{
    _value.b = v;
}

template <>
Param::Param<unsigned>(const char* n, unsigned* v) : _name(n), _ptype(PARAM_INT)
{ 
    _value.i = v; 
}

template <>
Param::Param<float>(const char* n, float* v) : _name(n), _ptype(PARAM_FLOAT)
{
    _value.f = v;
}

template <>
Param::Param<char>(const char* n, char* v) : _name(n), _ptype(PARAM_STRING)
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
        break;
    case PARAM_FLOAT:
        *_value.f = strtof(v.c_str(), NULL);
        break;
    case PARAM_STRING:
        snprintf(_value.s, PARAM_MAX_STRING, "%s", v.c_str());
        break;
    case PARAM_UNSUPPORTED:
    default:
        break;
    }
}

std::string Param::get()
{
    char val[PARAM_MAX_STRING+1];
    std::string str;

    switch(_ptype)
    {
    case PARAM_BOOL:
        str = (*_value.b == false) ? "no" : "yes";
        break;
    case PARAM_INT:
        snprintf(val, PARAM_MAX_STRING, "%u", *_value.i);
        str = val;
        break;
    case PARAM_FLOAT:
        snprintf(val, PARAM_MAX_STRING, "%g", *_value.f);
        str = val;
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

} /* namespace Aeromatic */

