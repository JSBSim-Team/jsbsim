// controls.cxx -- defines a standard interface to all flight sim controls
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
// $Id: FGControls.cpp,v 1.1 2000/04/03 15:51:00 jsb Exp $
// (Log is kept at end of this file)


#include "FGControls.h"


FGControls controls;


// Constructor
FGControls::FGControls() :
    aileron( 0.0 ),
    elevator( 0.0 ),
    elevator_trim( 1.969572E-03 ),
    rudder( 0.0 )
{
  for ( int engine = 0; engine < MAX_ENGINES; engine++ ) {
    throttle[engine] = 0.0;
  }

  for ( int wheel = 0; wheel < MAX_WHEELS; wheel++ ) {
    brake[wheel] = 0.0;
  }
}


// Destructor
FGControls::~FGControls() {
}


// $Log: FGControls.cpp,v $
// Revision 1.1  2000/04/03 15:51:00  jsb
// Initial revision
//
// Revision 1.2  2000/03/30 15:40:04  jsb
// Further mods to FGConfigFile
//
// Revision 1.7  1999/12/30 17:01:59  curt
// Here is a wrap-up of the latest changes to JSBSim. It still is flaky, but
// much less so due to returning the aero reference point stuff to the config
// files. Don't know what happened there ...
//
// Additionally, I have added a new field to the config file: CFG_VERSION. A
// version number, currently 1.1, is assigned to the config file and a matching
// definition is found in FGDefs.h. The two need to match. Tony has also added
// code into FGAircraft.cpp to handle if aero reference point is not specified.
//
// Revision 1.6  1999/09/07 21:15:45  curt
// Updates to get engine working.
//
// Revision 1.1  1999/02/13 01:12:03  curt
// Initial Revision.
//
// Revision 1.1  1999/02/09 04:51:32  jon
// Initial revision
//
// Revision 1.3  1998/12/05 16:13:12  curt
// Renamed class fgCONTROLS to class FGControls.
//
// Revision 1.2  1998/10/25 14:08:41  curt
// Turned "struct fgCONTROLS" into a class, with inlined accessor functions.
//
// Revision 1.1  1998/10/18 01:51:05  curt
// c++-ifying ...
//
// Revision 1.8  1998/09/29 02:01:31  curt
// Added a brake.
//
// Revision 1.7  1998/02/07 15:29:36  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.6  1998/01/19 19:27:02  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.5  1998/01/19 18:40:22  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.4  1997/12/10 22:37:41  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.3  1997/08/27 03:30:01  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.2  1997/06/21 17:12:48  curt
// Capitalized subdirectory names.
//
// Revision 1.1  1997/05/31 19:24:04  curt
// Initial revision.
//

