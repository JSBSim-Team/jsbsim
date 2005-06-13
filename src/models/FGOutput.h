/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGOutput.h
 Author:       Jon Berndt
 Date started: 12/2/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGOUTPUT_H
#define FGOUTPUT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_IOSTREAM
#  include STL_FSTREAM
#else
#  if defined(sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <iostream.h>
#    include <fstream.h>
#  else
#    include <iostream>
#    include <fstream>
#  endif
#endif

#include <input_output/FGfdmSocket.h>
#include <input_output/FGXMLElement.h>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_OUTPUT "$Id: FGOutput.h,v 1.3 2005/06/13 16:59:18 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Handles simulation output.
    OUTPUT section definition

    The following specifies the way that JSBSim writes out data.

    NAME is the filename you want the output to go to

    TYPE can be:
      CSV       Comma separated data. If a filename is supplied then the data
                goes to that file. If COUT or cout is specified, the data goes
                to stdout. If the filename is a null filename the data goes to
                stdout, as well.
      SOCKET    Will eventually send data to a socket output, where NAME
                would then be the IP address of the machine the data should be
                sent to. DON'T USE THIS YET!
      TABULAR   Columnar data. NOT IMPLEMENTED YET!
      TERMINAL  Output to terminal. NOT IMPLEMENTED YET!
      NONE      Specifies to do nothing. THis setting makes it easy to turn on and
                off the data output without having to mess with anything else.

    The arguments that can be supplied, currently, are

    RATE_IN_HZ  An integer rate in times-per-second that the data is output. This
                value may not be *exactly* what you want, due to the dependence
                on dt, the cycle rate for the FDM.

    The following parameters tell which subsystems of data to output:

    SIMULATION       ON|OFF
    ATMOSPHERE       ON|OFF
    MASSPROPS        ON|OFF
    AEROSURFACES     ON|OFF
    RATES            ON|OFF
    VELOCITIES       ON|OFF
    FORCES           ON|OFF
    MOMENTS          ON|OFF
    POSITION         ON|OFF
    COEFFICIENTS     ON|OFF
    GROUND_REACTIONS ON|OFF
    FCS              ON|OFF
    PROPULSION       ON|OFF

    NOTE that Time is always output with the data.
    @version $Id: FGOutput.h,v 1.3 2005/06/13 16:59:18 ehofman Exp $
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGOutput : public FGModel
{
public:
  FGOutput(FGFDMExec*);
  ~FGOutput();

  bool Run(void);

  void DelimitedOutput(string);
  void SocketOutput(void);
  void SocketStatusOutput(string);
  void SetType(string);
  void SetSubsystems(int tt) {SubSystems = tt;}
  inline void Enable(void) { enabled = true; }
  inline void Disable(void) { enabled = false; }
  inline bool Toggle(void) {enabled = !enabled; return enabled;}
  bool Load(Element* el);

  /// Subsystem types for specifying which will be output in the FDM data logging
  enum  eSubSystems {
    /** Subsystem: Simulation (= 1)          */ ssSimulation      = 1,
    /** Subsystem: Aerosurfaces (= 2)        */ ssAerosurfaces    = 2,
    /** Subsystem: Body rates (= 4)          */ ssRates           = 4,
    /** Subsystem: Velocities (= 8)          */ ssVelocities      = 8,
    /** Subsystem: Forces (= 16)             */ ssForces          = 16,
    /** Subsystem: Moments (= 32)            */ ssMoments         = 32,
    /** Subsystem: Atmosphere (= 64)         */ ssAtmosphere      = 64,
    /** Subsystem: Mass Properties (= 128)   */ ssMassProps       = 128,
    /** Subsystem: Coefficients (= 256)      */ ssCoefficients    = 256,
    /** Subsystem: Propagate (= 512)         */ ssPropagate       = 512,
    /** Subsystem: Ground Reactions (= 1024) */ ssGroundReactions = 1024,
    /** Subsystem: FCS (= 2048)              */ ssFCS             = 2048,
    /** Subsystem: Propulsion (= 4096)       */ ssPropulsion      = 4096
  } subsystems;

private:
  bool sFirstPass, dFirstPass, enabled;
  int SubSystems;
  string output_file_name, delimeter, Filename;
  enum {otNone, otCSV, otTab, otSocket, otTerminal, otUnknown} Type;
  ofstream datafile;
  FGfdmSocket* socket;
  vector <FGPropertyManager*> OutputProperties;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

