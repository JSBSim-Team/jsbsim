/*******************************************************************************

 Module:       FGAerodynamics.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the aerodynamic forces (gear and collision)

 ------------- Copyright (C) 2000  Jon S. Berndt (jsb@hal-pc.org) -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------
09/13/00   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGAerodynamics.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGAerodynamics.cpp,v 1.2 2000/10/13 19:21:01 jsb Exp $";
static const char *IdHdr = ID_AERODYNAMICS;

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


FGAerodynamics::FGAerodynamics(FGFDMExec* fgex) : FGModel(fgex)
{

}


bool FGAerodynamics:: Run(void) {

  if (!FGModel::Run()) {

    return false;
  } else {
    return true;
  }
}

bool FGAerodynamics::LoadAerodynamics(FGConfigFile* AC_cfg)
{
//
}

