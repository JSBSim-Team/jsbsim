/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGGroundReactions.cpp
 Author:       Jon S. Berndt
 Date started: 09/13/00
 Purpose:      Encapsulates the ground reaction forces (gear and collision)

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGGroundReactions.h"
#include "FGPropertyManager.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGGroundReactions.cpp,v 1.38 2004/04/17 21:21:26 jberndt Exp $";
static const char *IdHdr = ID_GROUNDREACTIONS;

#if defined (__APPLE__)
/* Not all systems have the gcvt function */
inline char* gcvt (double value, int ndigits, char *buf) {
    /* note that this is not exactly what gcvt is supposed to do! */
    snprintf (buf, ndigits+1, "%f", value);
    return buf;
}
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGGroundReactions::FGGroundReactions(FGFDMExec* fgex) : FGModel(fgex)
{
  Name = "FGGroundReactions";

  bind();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGroundReactions::~FGGroundReactions(void)
{
  unbind();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundReactions::Run(void)
{
  if (!FGModel::Run()) {
    vForces.InitMatrix();
    vMoments.InitMatrix();

    // Only execute gear force code below 300 feet
    if ( Propagate->GetDistanceAGL() < 300.0 ) {
      vector <FGLGear>::iterator iGear = lGear.begin();
      // Sum forces and moments for all gear, here.
      // Some optimizations may be made here - or rather in the gear code itself.
      // The gear ::Run() method is called several times - once for each gear.
      // Perhaps there is some commonality for things which only need to be
      // calculated once.
      while (iGear != lGear.end()) {
        vForces  += iGear->Force();
        vMoments += iGear->Moment();
        iGear++;
      }

    } else {
      // Crash Routine
    }

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundReactions::Load(FGConfigFile* AC_cfg)
{
  string token;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/UNDERCARRIAGE")) {
    lGear.push_back(FGLGear(AC_cfg, FDMExec));
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGGroundReactions::GetGroundReactionStrings(void)
{
  string GroundReactionStrings = "";
  bool firstime = true;

  for (unsigned int i=0;i<lGear.size();i++) {
    if (!firstime) GroundReactionStrings += ", ";
    GroundReactionStrings += (lGear[i].GetName() + "_WOW, ");
    GroundReactionStrings += (lGear[i].GetName() + "_stroke, ");
    GroundReactionStrings += (lGear[i].GetName() + "_strokeVel, ");
    GroundReactionStrings += (lGear[i].GetName() + "_CompressForce, ");
    GroundReactionStrings += (lGear[i].GetName() + "_WhlSideForce, ");
    GroundReactionStrings += (lGear[i].GetName() + "_WhlVelVecX, ");
    GroundReactionStrings += (lGear[i].GetName() + "_WhlVelVecY, ");
    GroundReactionStrings += (lGear[i].GetName() + "_WhlRollForce, ");
    GroundReactionStrings += (lGear[i].GetName() + "_BodyXForce, ");
    GroundReactionStrings += (lGear[i].GetName() + "_BodyYForce, ");
    GroundReactionStrings += (lGear[i].GetName() + "_WhlSlipDegrees");

    firstime = false;
  }

  GroundReactionStrings += ", TotalGearForce_X, ";
  GroundReactionStrings += "TotalGearForce_Y, ";
  GroundReactionStrings += "TotalGearForce_Z, ";
  GroundReactionStrings += "TotalGearMoment_L, ";
  GroundReactionStrings += "TotalGearMoment_M, ";
  GroundReactionStrings += "TotalGearMoment_N";

  return GroundReactionStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGGroundReactions::GetGroundReactionValues(void)
{
  char buff[20];
  string GroundReactionValues = "";

  bool firstime = true;

  for (unsigned int i=0;i<lGear.size();i++) {
    if (!firstime) GroundReactionValues += ", ";
    GroundReactionValues += string( lGear[i].GetWOW()?"1":"0" ) + ", ";
    GroundReactionValues += (string(gcvt(lGear[i].GetCompLen(),    5, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetCompVel(),    6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetCompForce(), 10, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetWheelVel(eX), 6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetWheelVel(eY), 6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetWheelSideForce(), 6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetWheelRollForce(), 6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetBodyXForce(), 6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetBodyYForce(), 6, buff)) + ", ");
    GroundReactionValues += (string(gcvt(lGear[i].GetWheelSlipAngle(), 6, buff)));

    firstime = false;
  }

  GroundReactionValues += (", " + string(gcvt(vForces(eX), 6, buff)) + ", ");
  GroundReactionValues += (string(gcvt(vForces(eY), 6, buff)) + ", ");
  GroundReactionValues += (string(gcvt(vForces(eZ), 6, buff)) + ", ");
  GroundReactionValues += (string(gcvt(vMoments(eX), 6, buff)) + ", ");
  GroundReactionValues += (string(gcvt(vMoments(eY), 6, buff)) + ", ");
  GroundReactionValues += (string(gcvt(vMoments(eZ), 6, buff)));

  return GroundReactionValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGroundReactions::bind(void)
{
  typedef double (FGGroundReactions::*PMF)(int) const;
  PropertyManager->Tie("gear/num-units", this,
                       &FGGroundReactions::GetNumGearUnits);
  PropertyManager->Tie("moments/l-gear-lbsft", this,1,
                       (PMF)&FGGroundReactions::GetMoments);
  PropertyManager->Tie("moments/m-gear-lbsft", this,2,
                       (PMF)&FGGroundReactions::GetMoments);
  PropertyManager->Tie("moments/n-gear-lbsft", this,3,
                       (PMF)&FGGroundReactions::GetMoments);
  PropertyManager->Tie("forces/fbx-gear-lbs", this,1,
                       (PMF)&FGGroundReactions::GetForces);
  PropertyManager->Tie("forces/fby-gear-lbs", this,2,
                       (PMF)&FGGroundReactions::GetForces);
  PropertyManager->Tie("forces/fbz-gear-lbs", this,3,
                       (PMF)&FGGroundReactions::GetForces);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGroundReactions::unbind(void)
{
  PropertyManager->Untie("gear/num-units");
  PropertyManager->Untie("moments/l-gear-lbsft");
  PropertyManager->Untie("moments/m-gear-lbsft");
  PropertyManager->Untie("moments/n-gear-lbsft");
  PropertyManager->Untie("forces/fbx-gear-lbs");
  PropertyManager->Untie("forces/fby-gear-lbs");
  PropertyManager->Untie("forces/fbz-gear-lbs");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGGroundReactions::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGGroundReactions" << endl;
    if (from == 1) cout << "Destroyed:    FGGroundReactions" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
