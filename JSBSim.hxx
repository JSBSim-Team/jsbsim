// JSBsim.hxx -- interface to the "JSBsim" flight model
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id: JSBSim.hxx,v 1.2 2000/10/13 11:11:29 jsb Exp $


#ifndef _JSBSIM_HXX
#define _JSBSIM_HXX

#include <FDM/JSBSim/FGFDMExec.h>
#undef MAX_ENGINES

#include <Aircraft/aircraft.hxx>


class FGJSBsim: public FGInterface {

    // The aircraft for this instance
    FGFDMExec FDMExec;
    bool trimmed;
    float trim_elev;
    float trim_throttle;

public:

    // copy FDM state to LaRCsim structures
    int copy_to_JSBsim();

    // copy FDM state from LaRCsim structures
    int copy_from_JSBsim();

    // reset flight params to a specific position 
    int init( double dt );

    // update position based on inputs, positions, velocities, etc.
    int update( int multiloop );
};


#endif // _JSBSIM_HXX


