/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGPropulsion.cpp
 Author:       Jon S. Berndt
 Date started: 08/20/00
 Purpose:      Encapsulates the set of engines, tanks, and thrusters associated
               with this aircraft

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
The Propulsion class is the container for the entire propulsion system, which is
comprised of engines, tanks, and "thrusters" (the device that transforms the
engine power into a force that acts on the aircraft, such as a nozzle or
propeller). Once the Propulsion class gets the config file, it reads in
information which is specific to a type of engine. Then:

1) The appropriate engine type instance is created
2) At least one thruster object is instantiated, and is linked to the engine
3) At least one tank object is created, and is linked to an engine.

Note: Thusters can be linked to more than one engine and engines can be linked
to more than one thruster. It is the same with tanks - a many to many
relationship can be established.

At Run time each engines Calculate() method is called to return the excess power
generated during that iteration. The drag from the previous iteration is sub-
tracted to give the excess power available for thrust this pass. That quantity
is passed to the thrusters associated with a particular engine - perhaps with a
scaling mechanism (gearing?) to allow the engine to give its associated thrust-
ers specific distributed portions of the excess power.

HISTORY
--------------------------------------------------------------------------------
08/20/00   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGPropulsion.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPropulsion.cpp,v 1.13 2000/11/27 07:34:03 jsb Exp $";
static const char *IdHdr = ID_PROPULSION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGPropulsion::FGPropulsion(FGFDMExec* exec) : FGModel(exec)
{
  numSelectedFuelTanks = numSelectedOxiTanks = 0;
  numTanks = numEngines = 0;
  numOxiTanks = numFuelTanks = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropulsion::~FGPropulsion(void)
{
  Engines.clear();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion:: Run(void) {
  float tot_thrust;
  iEngine = Engines.begin();

  if (!FGModel::Run()) {
    while (iEngine != Engines.end()) {
      tot_thrust = tot_thrust + iEngine->Calculate();
      iEngine++;
    }

    return false;
  } else {
    return true;
  }
}

/*
  // UPDATE TANK CONTENTS
  //
  // For each engine, cycle through the tanks and draw an equal amount of
  // fuel (or oxidizer) from each active tank. The needed amount of fuel is
  // determined by the engine in the FGEngine class. If more fuel is needed
  // than is available in the tank, then that amount is considered a shortage,
  // and will be drawn from the next tank. If the engine cannot be fed what it
  // needs, it will be considered to be starved, and will shut down.

  float Oshortage, Fshortage;

  for (unsigned int e=0; e<numEngines; e++) {
    Fshortage = Oshortage = 0.0;
    for (t=0; t<numTanks; t++) {
      switch(Engine[e]->GetType()) {
      case FGEngine::etRocket:

        switch(Tank[t]->GetType()) {
        case FGTank::ttFUEL:
          if (Tank[t]->GetSelected()) {
            Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/
                                         numSelectedFuelTanks)*(dt*rate) + Fshortage);
          }
          break;
        case FGTank::ttOXIDIZER:
          if (Tank[t]->GetSelected()) {
            Oshortage = Tank[t]->Reduce((Engine[e]->CalcOxidizerNeed()/
                                         numSelectedOxiTanks)*(dt*rate) + Oshortage);
          }
          break;
        }
        break;

      case FGEngine::etPiston:
      case FGEngine::etTurboJet:
      case FGEngine::etTurboProp:

        if (Tank[t]->GetSelected()) {
          Fshortage = Tank[t]->Reduce((Engine[e]->CalcFuelNeed()/
                                       numSelectedFuelTanks)*(dt*rate) + Fshortage);
        }
        break;
      }
    }
    if ((Fshortage <= 0.0) || (Oshortage <= 0.0)) Engine[e]->SetStarved();
    else Engine[e]->SetStarved(false);
  }
*/
*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::LoadPropulsion(FGConfigFile* AC_cfg)
{
  string token, tag;
  string engineName, fullpath;
  string parameter;
  string enginePath = FDMExec->GetEnginePath();
  float xLoc, yLoc, zLoc, engPitch, engYaw;

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/PROPULSION") {
    *AC_cfg >> parameter;

    if (parameter == "AC_ENGINE") {

      *AC_cfg >> engineName;

# ifndef macintosh
      fullpath = enginePath + "/" + engineName + ".xml";
# else
      fullpath = enginePath + ";" + engineName + ".xml";
# endif


      cout << "    Reading engine: " << engineName << " from file: " << fullpath << endl;
      FGConfigFile Eng_cfg(fullpath);

      if (Eng_cfg.IsOpen()) {
        Eng_cfg >> tag;

        if (tag == "FG_ROCKET") {
          Engines.push_back(*(new FGRocket(FDMExec, &Eng_cfg)));
        } else if (tag == "FG_PISTON") {
          Engines.push_back(*(new FGPiston(FDMExec, &Eng_cfg)));
        } else if (tag == "FG_TURBOJET") {
          Engines.push_back(*(new FGTurboJet(FDMExec, &Eng_cfg)));
        } else if (tag == "FG_TURBOSHAFT") {
          Engines.push_back(*(new FGTurboShaft(FDMExec, &Eng_cfg)));
        } else if (tag == "FG_TURBOPROP") {
          Engines.push_back(*(new FGTurboProp(FDMExec, &Eng_cfg)));
        }

        *AC_cfg >> xLoc >> yLoc >> zLoc;
        *AC_cfg >> engPitch >> engYaw;

        Engines[numEngines].SetPlacement(xLoc, yLoc, zLoc, engPitch, engYaw);
        Engines[numEngines].SetName(engineName);
        
        numEngines++;
      } else {
        cerr << "Could not read engine config file: " << fullpath << endl;
        return false;
      }

    } else if (parameter == "AC_TANK") {

      Tanks.push_back(*(new FGTank(AC_cfg)));
      switch(Tanks[numTanks].GetType()) {
      case FGTank::ttFUEL:
        numSelectedFuelTanks++;
        numFuelTanks++;
        break;
      case FGTank::ttOXIDIZER:
        numSelectedOxiTanks++;
        numOxiTanks++;
        break;
      }
      numTanks++;
    }
  }
  return true;
}
