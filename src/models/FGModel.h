/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGModel.h
 Author:       Jon Berndt
 Date started: 11/21/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

HISTORY
--------------------------------------------------------------------------------
11/22/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMODEL_H
#define FGMODEL_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <FGJSBBase.h>
#include <input_output/FGPropertyManager.h>
#include <input_output/FGXMLElement.h>

#include <iostream>
#include <string>
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MODEL "$Id: FGModel.h,v 1.10 2009/06/09 03:23:55 jberndt Exp $"

using namespace std;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFDMExec;
class FGState;
class FGAtmosphere;
class FGFCS;
class FGPropulsion;
class FGMassBalance;
class FGAerodynamics;
class FGInertial;
class FGGroundReactions;
class FGExternalReactions;
class FGBuoyantForces;
class FGAircraft;
class FGPropagate;
class FGAuxiliary;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for all scheduled JSBSim models
    @author Jon S. Berndt
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGModel : public FGJSBBase
{
public:

  /// Constructor
  FGModel(FGFDMExec*);
  /// Destructor
  ~FGModel();

  /** Loads this model.
      @param el a pointer to the element
      @return true if model is successfully loaded*/
  virtual bool Load(Element* el);

  FGModel* NextModel;
  string Name;

  /** Runs the model; called by the Executive
      @see JSBSim.cpp documentation
      @return false if no error */
  virtual bool Run(void);
  virtual bool InitModel(void);
  virtual void SetRate(int tt) {rate = tt;}
  virtual int  GetRate(void)   {return rate;}
  FGFDMExec* GetExec(void)     {return FDMExec;}

  void SetPropertyManager(FGPropertyManager *fgpm) { PropertyManager=fgpm;}

protected:
  int exe_ctr;
  int rate;

  virtual void Debug(int from);

  FGFDMExec*         FDMExec;
  FGState*           State;
  FGAtmosphere*      Atmosphere;
  FGFCS*             FCS;
  FGPropulsion*      Propulsion;
  FGMassBalance*     MassBalance;
  FGAerodynamics*    Aerodynamics;
  FGInertial*        Inertial;
  FGGroundReactions* GroundReactions;
  FGExternalReactions* ExternalReactions;
  FGBuoyantForces*   BuoyantForces;
  FGAircraft*        Aircraft;
  FGPropagate*       Propagate;
  FGAuxiliary*       Auxiliary;
  FGPropertyManager* PropertyManager;

  vector <double*> interface_properties;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

