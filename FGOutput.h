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
#  ifdef FG_HAVE_STD_INCLUDES
#    include <iostream>
#    include <fstream>
#  else
#    include <iostream.h>
#    include <fstream.h>
#  endif
#else
#  include <iostream>
#  include <fstream>
#endif

#include "FGfdmSocket.h"

#define ID_OUTPUT "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGOutput.h,v 1.8 2000/10/16 12:32:46 jsb Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGOutput : public FGModel
{
public:
  FGOutput(FGFDMExec*);
  ~FGOutput(void);

  bool Run(void);

  void DelimitedOutput(void);
  void DelimitedOutput(string);
  void SocketOutput(void);
  void SocketStatusOutput(string);
  void SetFilename(string fn) {Filename = fn;}
  void SetType(string);
  void SetSubsystems(int tt) {SubSystems = tt;}
  inline void Enable(void) { enabled = true; }
  inline void Disable(void) { enabled = false; }

protected:

private:
  bool sFirstPass, dFirstPass, enabled;
  int SubSystems;
  string Filename;
  enum {otNone, otCSV, otTab, otSocket, otTerminal, otUnknown} Type;
  ofstream datafile;
  FGfdmSocket* socket;
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

