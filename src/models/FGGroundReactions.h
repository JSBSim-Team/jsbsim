/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGGroundReactions.h
 Author:       Jon S. Berndt
 Date started: 09/13/00

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
09/13/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGGROUNDREACTIONS_H
#define FGGROUNDREACTIONS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <vector>

#include "FGModel.h"
#include "FGLGear.h"
#include "math/FGColumnVector3.h"
#include "input_output/FGXMLElement.h"

#define ID_GROUNDREACTIONS "$Id: FGGroundReactions.h,v 1.24 2011/08/21 15:13:22 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Manages ground reactions modeling. Maintains a list of landing gear and
    ground contact points, all instances of FGLGear.  Sums their forces and
    moments so that these may be provided to FGPropagate.  Parses the 
    \<ground_reactions> section of the aircraft configuration file.
 <h3>Configuration File Format of \<ground_reactions> Section:</h3>
@code
    <ground_reactions>
        <contact>
           ... {see FGLGear for specifics of this format}
        </contact>
        ... {more contacts}
    </ground_reactions>
@endcode   


  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGGroundReactions : public FGModel
{
public:
  FGGroundReactions(FGFDMExec*);
  ~FGGroundReactions(void);

  bool InitModel(void);
  /** Runs the Ground Reactions model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);
  bool Load(Element* el);
  const FGColumnVector3& GetForces(void) const {return vForces;}
  double GetForces(int idx) const {return vForces(idx);}
  const FGColumnVector3& GetMoments(void) const {return vMoments;}
  double GetMoments(int idx) const {return vMoments(idx);}
  string GetGroundReactionStrings(string delimeter) const;
  string GetGroundReactionValues(string delimeter) const;
  bool GetWOW(void) const;

  int GetNumGearUnits(void) const { return (int)lGear.size(); }

  /** Gets a gear instance
      @param gear index of gear instance
      @return a pointer to the FGLGear instance of the gear unit requested */
  FGLGear* GetGearUnit(int gear) const { return lGear[gear]; }

  void RegisterLagrangeMultiplier(LagrangeMultiplier* lmult) { multipliers.push_back(lmult); }
  vector <LagrangeMultiplier*>* GetMultipliersList(void) { return &multipliers; }

  FGLGear::Inputs in;

private:
  vector <FGLGear*> lGear;
  FGColumnVector3 vForces;
  FGColumnVector3 vMoments;
  vector <LagrangeMultiplier*> multipliers;

  void bind(void);
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

