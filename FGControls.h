// controls.hxx -- defines a standard interface to all flight sim controls
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
// $Id: FGControls.h,v 1.1 2000/04/03 15:51:01 jsb Exp $
// (Log is kept at end of this file)


#ifndef _CONTROLS_HXX
#define _CONTROLS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

//using namespace std;

// Define a structure containing the control parameters

class FGControls {

public:

    static const int ALL_ENGINES = -1;
    static const int MAX_ENGINES = 10;

    static const int ALL_WHEELS = -1;
    static const int MAX_WHEELS = 3;

private:

    double aileron;
    double elevator;
    double elevator_trim;
    double rudder;
    double throttle[MAX_ENGINES];
    double brake[MAX_WHEELS];

public:

    FGControls();
    ~FGControls();

    // Query functions
    inline double get_aileron() const { return aileron; }
    inline double get_elevator() const { return elevator; }
    inline double get_elevator_trim() const { return elevator_trim; }
    inline double get_rudder() const { return rudder; }
    inline double get_throttle(int engine) const { return throttle[engine]; }
    inline double get_brake(int wheel) const { return brake[wheel]; }

    // Update functions
    inline void set_aileron( double pos ) {
	aileron = pos;
	if ( aileron < -1.0 ) aileron = -1.0;
	if ( aileron >  1.0 ) aileron =  1.0;
    }
    inline void move_aileron( double amt ) {
	aileron += amt;
	if ( aileron < -1.0 ) aileron = -1.0;
	if ( aileron >  1.0 ) aileron =  1.0;
    }
    inline void set_elevator( double pos ) {
	elevator = pos;
	if ( elevator < -1.0 ) elevator = -1.0;
	if ( elevator >  1.0 ) elevator =  1.0;
    }
    inline void move_elevator( double amt ) {
	elevator += amt;
	if ( elevator < -1.0 ) elevator = -1.0;
	if ( elevator >  1.0 ) elevator =  1.0;
    }
    inline void set_elevator_trim( double pos ) {
	elevator_trim = pos;
	if ( elevator_trim < -1.0 ) elevator_trim = -1.0;
	if ( elevator_trim >  1.0 ) elevator_trim =  1.0;
    }
    inline void move_elevator_trim( double amt ) {
	elevator_trim += amt;
	if ( elevator_trim < -1.0 ) elevator_trim = -1.0;
	if ( elevator_trim >  1.0 ) elevator_trim =  1.0;
    }
    inline void set_rudder( double pos ) {
	rudder = pos;
	if ( rudder < -1.0 ) rudder = -1.0;
	if ( rudder >  1.0 ) rudder =  1.0;
    }
    inline void move_rudder( double amt ) {
	rudder += amt;
	if ( rudder < -1.0 ) rudder = -1.0;
	if ( rudder >  1.0 ) rudder =  1.0;
    }
    inline void set_throttle( int engine, double pos ) {
	if ( engine == ALL_ENGINES ) {
	    for ( int i = 0; i < MAX_ENGINES; i++ ) {
		throttle[i] = pos;
		if ( throttle[i] < 0.0 ) throttle[i] = 0.0;
		if ( throttle[i] >  1.0 ) throttle[i] =  1.0;
	    }
	} else {
	    if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
		throttle[engine] = pos;
		if ( throttle[engine] < 0.0 ) throttle[engine] = 0.0;
		if ( throttle[engine] >  1.0 ) throttle[engine] =  1.0;
	    }
	}
    }
    inline void move_throttle( int engine, double amt ) {
	if ( engine == ALL_ENGINES ) {
	    for ( int i = 0; i < MAX_ENGINES; i++ ) {
		throttle[i] += amt;
		if ( throttle[i] < 0.0 ) throttle[i] = 0.0;
		if ( throttle[i] >  1.0 ) throttle[i] =  1.0;
	    }
	} else {
	    if ( (engine >= 0) && (engine < MAX_ENGINES) ) {
		throttle[engine] += amt;
		if ( throttle[engine] < 0.0 ) throttle[engine] = 0.0;
		if ( throttle[engine] >  1.0 ) throttle[engine] =  1.0;
	    }
	}
    }
    inline void set_brake( int wheel, double pos ) {
	if ( wheel == ALL_WHEELS ) {
	    for ( int i = 0; i < MAX_WHEELS; i++ ) {
		brake[i] = pos;
		if ( brake[i] < 0.0 ) brake[i] = 0.0;
		if ( brake[i] >  1.0 ) brake[i] =  1.0;
	    }
	} else {
	    if ( (wheel >= 0) && (wheel < MAX_WHEELS) ) {
		brake[wheel] = pos;
		if ( brake[wheel] < 0.0 ) brake[wheel] = 0.0;
		if ( brake[wheel] >  1.0 ) brake[wheel] =  1.0;
	    }
	}
    }
    inline void move_brake( int wheel, double amt ) {
	if ( wheel == ALL_WHEELS ) {
	    for ( int i = 0; i < MAX_WHEELS; i++ ) {
		brake[i] += amt;
		if ( brake[i] < 0.0 ) brake[i] = 0.0;
		if ( brake[i] >  1.0 ) brake[i] =  1.0;
	    }
	} else {
	    if ( (wheel >= 0) && (wheel < MAX_WHEELS) ) {
		brake[wheel] += amt;
		if ( brake[wheel] < 0.0 ) brake[wheel] = 0.0;
		if ( brake[wheel] >  1.0 ) brake[wheel] =  1.0;
	    }
	}
    }
};


extern FGControls controls;


#endif // _CONTROLS_HXX


// $Log: FGControls.h,v $
// Revision 1.1  2000/04/03 15:51:01  jsb
// Initial revision
//
// Revision 1.2  2000/03/30 15:40:04  jsb
// Further mods to FGConfigFile
//
// Revision 1.6  1999/09/07 21:15:45  curt
// Updates to get engine working.
//
// Revision 1.1  1999/02/13 01:12:03  curt
// Initial Revision.
//
// Revision 1.3  1998/12/05 16:13:13  curt
// Renamed class fgCONTROLS to class FGControls.
//
// Revision 1.2  1998/10/25 14:08:42  curt
// Turned "struct fgCONTROLS" into a class, with inlined accessor functions.
//
// Revision 1.1  1998/10/18 01:51:07  curt
// c++-ifying ...
//
// Revision 1.17  1998/09/29 14:57:00  curt
// c++-ified some comments.
//
// Revision 1.16  1998/09/29 02:01:32  curt
// Added a brake.
//
// Revision 1.15  1998/04/25 22:06:27  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.14  1998/04/22 13:26:19  curt
// C++ - ifing the code a bit.
//
// Revision 1.13  1998/04/21 17:02:35  curt
// Prepairing for C++ integration.
//
// Revision 1.12  1998/02/09 22:56:48  curt
// Removed "depend" files from cvs control.  Other minor make tweaks.
//
// Revision 1.11  1998/02/07 15:29:36  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.10  1998/01/27 00:47:52  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.9  1998/01/22 02:59:31  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.8  1998/01/19 18:40:22  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.7  1997/12/15 23:54:36  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.6  1997/12/10 22:37:41  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.5  1997/08/27 03:30:02  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.4  1997/07/23 21:52:18  curt
// Put comments around the text after an #endif for increased portability.
//
// Revision 1.3  1997/05/31 19:16:27  curt
// Elevator trim added.
//
// Revision 1.2  1997/05/23 15:40:33  curt
// Added GNU copyright headers.
//
// Revision 1.1  1997/05/16 15:59:48  curt
// Initial revision.
//
