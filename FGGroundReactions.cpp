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

static const char *IdSrc = "$Id: FGGroundReactions.cpp,v 1.12 2001/08/07 23:05:46 jberndt Exp $";
static const char *IdHdr = ID_GROUNDREACTIONS;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGGroundReactions::FGGroundReactions(FGFDMExec* fgex) : FGModel(fgex),
                                                        vForces(3),
                                                        vMoments(3),
                                                        vMaxStaticGrip(3),
                                                        vMaxMomentResist(3)
{
  Name = "FGGroundReactions";

  GearUp = false;
  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGroundReactions:: Run(void)
{
  int wow_count=0;
  float steerAngle = 0.0;
  float xForces = 0.0, yForces = 0.0;

  if (!FGModel::Run()) {
    vForces.InitMatrix();
    vMoments.InitMatrix();

    // Only execute gear force code below 300 feet
    if ( !GearUp && Position->GetDistanceAGL() < 300.0 ) {
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

      // Only execute this code when the aircraft ground speed is very, very small.
      if (fabs(Translation->GetUVW(eX)) < 0.1 &&
          fabs(Translation->GetUVW(eZ)) < 0.1)
      {
        // Initialize the comparison matrices.
        vMaxStaticGrip.InitMatrix();
        vMaxMomentResist.InitMatrix();
        iGear = lGear.begin();
        // For each gear that is touching the ground (which had better be all of them!)
        // calculate the X and Y direction maximum "gripping" power. Also, keep track
        // of the number of gear that have weight on wheels. This is probably unnecessary.
        while (iGear != lGear.end()) {
          // calculate maximum gripping power for each gear here based on brake
          // and steering settings
          // also calculate total number of wheels with WOW set true?
          if (iGear->GetWOW()) {
            steerAngle = iGear->GetSteerAngle();
            vMaxStaticGrip(eX) += (iGear->GetBrakeFCoeff()*cos(steerAngle) - 
                 iGear->GetstaticFCoeff()*sin(steerAngle))*iGear->GetCompForce();
            vMaxStaticGrip(eY) += iGear->GetBrakeFCoeff()*sin(steerAngle) + 
                  iGear->GetstaticFCoeff()*cos(steerAngle)*iGear->GetCompForce();
            vMaxStaticGrip(eZ)  = 0.0;
//            vMaxMomentResist += 1;
            wow_count++;
          }
          iGear++;
        }

        // Calculate the X and Y direction non-gear forces to counteract if needed.
        xForces =  -1.0 * ( Aerodynamics->GetForces(eX)
                          + Propulsion->GetForces(eX)
                          + Inertial->GetForces(eX));

        yForces =  -1.0 * ( Aerodynamics->GetForces(eY)
                          + Propulsion->GetForces(eY)
                          + Inertial->GetForces(eY));

        if (fabs(xForces) < fabs(vMaxStaticGrip(eX))) { // forces exceed gear power
          vForces(eX) = xForces;
        }

        if (fabs(yForces) < fabs(vMaxStaticGrip(eY))) { // forces exceed gear power
          vForces(eY) = yForces;
        }

        vMoments(eZ) = -(Aerodynamics->GetMoments(eZ) + Propulsion->GetMoments(eZ));
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

