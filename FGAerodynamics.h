/*******************************************************************************

 Header:       FGAerodynamics.h
 Author:       Jon S. Berndt
 Date started: 09/13/00

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
09/13/00   JSB   Created

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGAERODYNAMICS_H
#define FGAERODYNAMICS_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <vector>
#  else
#    include <vector.h>
#  endif
#else
#  include <vector>
#endif

#include "FGModel.h"

#define ID_AERODYNAMICS "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGAerodynamics.h,v 1.2 2000/10/13 19:21:01 jsb Exp $"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAerodynamics : public FGModel {

public:
  FGAerodynamics(FGFDMExec*);
  ~FGAerodynamics(void);

  bool Run(void);
  bool LoadAerodynamics(FGConfigFile* AC_cfg);
};

/******************************************************************************/
#endif

