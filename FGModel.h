/*******************************************************************************

 Header:       FGModel.h
 Author:       Jon Berndt
 Date started: 11/21/98

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
11/22/98   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGMODEL_H
#define FGMODEL_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGDefs.h"

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_STRING
#  ifdef FG_HAVE_STD_INCLUDES
#    include <iostream>
#  else
#    include <iostream.h>
#  endif
   FG_USING_STD(string);
#else
#  include <string>
#  include <iostream>
#endif

/*******************************************************************************
DEFINES
*******************************************************************************/

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFDMExec;
class FGState;
class FGAtmosphere;
class FGFCS;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPosition;
class FGAuxiliary;
class FGOutput;

class FGModel
{
public:
  FGModel(FGFDMExec*);
  virtual ~FGModel(void);

  FGModel* NextModel;
  string Name;
  virtual bool Run(void);
  virtual bool InitModel(void);
  virtual void SetRate(int tt) {rate = tt;};

protected:
  enum {eU=1, eV, eW};
  enum {eNorth=1, eEast, eDown};
  enum {eP=1, eQ, eR};
  enum {eL=1, eM, eN};
  enum {eX=1, eY, eZ};
  enum {ePhi=1, eTht, ePsi};

  int exe_ctr;
  int rate;
  
  FGFDMExec*      FDMExec;
  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGOutput*       Output;

private:
};

/******************************************************************************/
#endif
