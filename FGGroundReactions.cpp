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

static const char *IdSrc = "$Id: FGGroundReactions.cpp,v 1.11 2001/08/07 13:00:31 jberndt Exp $";
static const char *IdHdr = ID_GROUNDREACTIONS;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGGroundReactions::FGGroundReactions(FGFDMExec* fgex) : FGModel(fgex),
                                                        vForces(3),
                                                        vMoments(3),
                                                        vMaxStaticGrip(3),
                                                        vMaxSlideResist(3),
                                                        vMaxMomentResist(3)
{
  Name = "FGGroundReactions";

  GearUp = false;
  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundReactions:: Run(void)
{
  if (!FGModel::Run()) {
    vForces.InitMatrix();
    vMoments.InitMatrix();

    if ( !GearUp && Position->GetDistanceAGL() < 300.0 ) {
      vector <FGLGear>::iterator iGear = lGear.begin();
      while (iGear != lGear.end()) {
        vForces  += iGear->Force();
        vMoments += iGear->Moment();
        iGear++;
      }
      if (fabs(Translation->GetUVW(eX)) < 0.1 &&
          fabs(Translation->GetUVW(eZ)) < 0.1)
      {
        vMaxStaticGrip.InitMatrix();
        vMaxSlideResist.InitMatrix();
        vMaxMomentResist.InitMatrix();
        iGear = lGear.begin();
        while (iGear != lGear.end()) {
          // calculate maximum gripping power for each gear here based on brake
          // and steering settings
          // also calculate total number of wheels with WOW set true?
          if (iGear->GetWOW()) {
            vMaxStaticGrip += 1;
            vMaxSlideResist += 1;
            vMaxMomentResist += 1;
          }
          iGear++;
        }

        vForces =  -1.0 * ( Aerodynamics->GetForces()
                          + Propulsion->GetForces()
                          + Inertial->GetForces());

        vMoments(1) = 0.0;
        vMoments(2) = 0.0;
        vMoments(3) = -(Aerodynamics->GetMoments(3) + Propulsion->GetMoments(3));
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

  while ((token = AC_cfg->GetValue()) != "/UNDERCARRIAGE") {
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
    GroundReactionStrings += (lGear[i].GetName() + "_compressLength, ");
    GroundReactionStrings += (lGear[i].GetName() + "_compressSpeed, ");
    GroundReactionStrings += (lGear[i].GetName() + "_Force");

    firstime = false;
  }

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
    GroundReactionValues += (string(gcvt(lGear[i].GetCompForce(), 10, buff)));

    firstime = false;
  }

  return GroundReactionValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGroundReactions::Debug(void)
{
    //TODO: Add your source code here
}

