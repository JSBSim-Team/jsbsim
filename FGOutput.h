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
#  if defined(sgi) && !defined(__GNUC__)
#    include <iostream.h>
#    include <fstream.h>
#  else
#    include <iostream>
#    include <fstream>
#  endif
#endif

#include "FGfdmSocket.h"

#define ID_OUTPUT "$Id: FGOutput.h,v 1.20 2001/11/14 23:53:27 jberndt Exp $"

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
  void SetFilename(string fn) {Filename = fn;}
  void SetType(string);
  void SetSubsystems(int tt) {SubSystems = tt;}
  inline void Enable(void) { enabled = true; }
  inline void Disable(void) { enabled = false; }
  inline bool Toggle(void) {enabled = !enabled; return enabled;}
  bool Load(FGConfigFile* AC_cfg);

  /// Subsystem types for specifying which will be output in the FDM data logging
  enum  SubSystems {
    /** Subsystem: Simulation (= 1)          */ ssSimulation      = 1,
    /** Subsystem: Aerosurfaces (= 2)        */ ssAerosurfaces    = 2,
    /** Subsystem: Body rates (= 4)          */ ssRates           = 4,
    /** Subsystem: Velocities (= 8)          */ ssVelocities      = 8,
    /** Subsystem: Forces (= 16)             */ ssForces          = 16,
    /** Subsystem: Moments (= 32)            */ ssMoments         = 32,
    /** Subsystem: Atmosphere (= 64)         */ ssAtmosphere      = 64,
    /** Subsystem: Mass Properties (= 128)   */ ssMassProps       = 128,
    /** Subsystem: Coefficients (= 256)      */ ssCoefficients    = 256,
    /** Subsystem: Position (= 512)          */ ssPosition        = 512,
    /** Subsystem: Ground Reactions (= 1024) */ ssGroundReactions = 1024,
    /** Subsystem: FCS (= 2048)              */ ssFCS             = 2048,
    /** Subsystem: Propulsion (= 4096)       */ ssPropulsion      = 4096
  } subsystems;

private:
  bool sFirstPass, dFirstPass, enabled;
  int SubSystems;
  string Filename;
  enum {otNone, otCSV, otTab, otSocket, otTerminal, otUnknown} Type;
  ofstream datafile;
  FGfdmSocket* socket;
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

