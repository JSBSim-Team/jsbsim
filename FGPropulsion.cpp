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

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPropulsion.cpp,v 1.11 2000/11/22 23:49:02 jsb Exp $";
static const char *IdHdr = ID_PROPULSION;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGPropulsion::FGPropulsion(FGFDMExec* exec) : FGModel(exec)
{

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion:: Run(void) {

  if (!FGModel::Run()) {

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::LoadPropulsion(FGConfigFile* AC_cfg)
{
  string token, tag;
  string engineName, fullpath;
  string parameter;
  string enginePath = FDMExec->GetEnginePath();
  int numEngines=0, numTanks=0;
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
        return false;
      }

    } else if (parameter == "AC_TANK") {

      Tanks.push_back(*(new FGTank(AC_cfg)));
      switch(Tanks[numTanks].GetType()) {
      case FGTank::ttFUEL:
//        numSelectedFuelTanks++;
        break;
      case FGTank::ttOXIDIZER:
//        numSelectedOxiTanks++;
        break;
      }
      numTanks++;
    }
  }
  return true;
}

