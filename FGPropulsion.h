/*******************************************************************************

 Header:       FGPropulsion.h
 Author:       Jon S. Berndt
 Date started: 08/20/00
 
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
08/20/00   JSB   Created
 
********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGPROPULSION_H
#define FGPROPULSION_H

#declare ID_PROPULSION "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPropulsion.h,v 1.2 2000/10/13 19:21:05 jsb Exp $"

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

#include "FGEngine.h"
#include "FGTank.h"
#include "FGThruster.h"
#include "FGConfigFile.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGPropulsion : public FGModel {
  vector <FGEngine>   Engines;
  vector <FGTank>     Tanks;
  vector <FGThruster> Thrusters;
public:
  FGPropulsion(FGFDMExec*);

  bool Run(void);
  bool LoadPropulsion(FGConfigFile* AC_cfg);
};

/******************************************************************************/
#endif

