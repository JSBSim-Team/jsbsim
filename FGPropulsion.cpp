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

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGPropulsion.cpp,v 1.36 2001/03/20 16:11:06 jberndt Exp $";
static const char *IdHdr = ID_PROPULSION;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGPropulsion::FGPropulsion(FGFDMExec* exec) : FGModel(exec)
{
  Name = "FGPropulsion";
  numSelectedFuelTanks = numSelectedOxiTanks = 0;
  numTanks = numEngines = numThrusters = 0;
  numOxiTanks = numFuelTanks = 0;
  Forces  = new FGColumnVector(3);
  Moments = new FGColumnVector(3);

  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPropulsion::~FGPropulsion()
{
  for (unsigned int i=0; i<Engines.size(); i++) delete Engines[i];
  Engines.clear();
  if (Forces) delete Forces;
  if (Moments) delete Moments;
  if (debug_lvl & 2) cout << "Destroyed:    FGPropulsion" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::Run(void) {
  float PowerAvailable;
  dt = State->Getdt();

  Forces->InitMatrix();
  Moments->InitMatrix();

  if (!FGModel::Run()) {
    for (unsigned int i=0; i<numEngines; i++) {
      Thrusters[i]->SetdeltaT(dt*rate);
      PowerAvailable = Engines[i]->Calculate(Thrusters[i]->GetPowerRequired());
      Thrusters[i]->Calculate(PowerAvailable);
      *Forces  += Thrusters[i]->GetBodyForces();  // sum body frame forces
      *Moments += Thrusters[i]->GetMoments();     // sum body frame moments
    }

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::GetSteadyState(void) {
  float PowerAvailable;
  float currentThrust = 0, lastThrust=-1;
  dt = State->Getdt();

  Forces->InitMatrix();
  Moments->InitMatrix();

  if (!FGModel::Run()) {
    for (unsigned int i=0; i<numEngines; i++) {
      Engines[i]->SetTrimMode(true);
      Thrusters[i]->SetdeltaT(dt*rate);
      while (pow(currentThrust - lastThrust, 2.0) > currentThrust*0.00010) {
        PowerAvailable = Engines[i]->Calculate(Thrusters[i]->GetPowerRequired());
        lastThrust = currentThrust;
        currentThrust = Thrusters[i]->Calculate(PowerAvailable);
      }
      *Forces  += Thrusters[i]->GetBodyForces();  // sum body frame forces
      *Moments += Thrusters[i]->GetMoments();     // sum body frame moments
      Engines[i]->SetTrimMode(false);
    }

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGPropulsion::LoadPropulsion(FGConfigFile* AC_cfg)
{
  string token, fullpath;
  string engineFileName, engType;
  string thrusterFileName, thrType;
  string parameter;
  string enginePath = FDMExec->GetEnginePath();
  float xLoc, yLoc, zLoc, Pitch, Yaw;
  int Feed;

# ifndef macintosh
      fullpath = enginePath + "/";
# else
      fullpath = enginePath + ";";
# endif

  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != "/PROPULSION") {

    if (token == "AC_ENGINE") {                   // ============ READING ENGINES

      engineFileName = AC_cfg->GetValue("FILE");

      cout << "\n    Reading engine from file: " << fullpath
                                                   + engineFileName + ".xml"<< endl;
      FGConfigFile Eng_cfg(fullpath + engineFileName + ".xml");

      if (Eng_cfg.IsOpen()) {
        engType = Eng_cfg.GetValue();

        FCS->AddThrottle();

        if (engType == "FG_ROCKET") {
          Engines.push_back(new FGRocket(FDMExec, &Eng_cfg));
        } else if (engType == "FG_PISTON") {
          Engines.push_back(new FGPiston(FDMExec, &Eng_cfg));
        } else if (engType == "FG_TURBOJET") {
          Engines.push_back(new FGTurboJet(FDMExec, &Eng_cfg));
        } else if (engType == "FG_TURBOSHAFT") {
          Engines.push_back(new FGTurboShaft(FDMExec, &Eng_cfg));
        } else if (engType == "FG_TURBOPROP") {
          Engines.push_back(new FGTurboProp(FDMExec, &Eng_cfg));
        } else {
          cerr << "    Unrecognized engine type: " << engType << " found in config file.\n";
        }

        AC_cfg->GetNextConfigLine();
        while ((token = AC_cfg->GetValue()) != "/AC_ENGINE") {
          *AC_cfg >> token;
	        if (token == "XLOC") { *AC_cfg >> xLoc; cout << "      X = " << xLoc << endl;}
          else if (token == "YLOC") *AC_cfg >> yLoc;
          else if (token == "ZLOC") *AC_cfg >> zLoc;
          else if (token == "PITCH") *AC_cfg >> Pitch;
          else if (token == "YAW") *AC_cfg >> Yaw;
          else if (token == "FEED") {
	          *AC_cfg >> Feed;
	          Engines[numEngines]->AddFeedTank(Feed);
		        cout << "      Feed tank: " << Feed << endl;
		      } else cerr << "Unknown identifier: " << token << " in engine file: "
	                                                      << engineFileName << endl;
	      }
	      
        Engines[numEngines]->SetPlacement(xLoc, yLoc, zLoc, Pitch, Yaw);
        numEngines++;

      } else {
      
        cerr << "Could not read engine config file: " << fullpath
                                                  + engineFileName + ".xml" << endl;
        return false;
      }

    } else if (token == "AC_TANK") {              // ============== READING TANKS

      cout << "\n    Reading tank definition" << endl;
      Tanks.push_back(new FGTank(AC_cfg));
      switch(Tanks[numTanks]->GetType()) {
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

    } else if (token == "AC_THRUSTER") {          // ========== READING THRUSTERS

      thrusterFileName = AC_cfg->GetValue("FILE");

      cout << "\n    Reading thruster from file: " <<
                                       fullpath + thrusterFileName + ".xml" << endl;
      FGConfigFile Thruster_cfg(fullpath + thrusterFileName + ".xml");

      if (Thruster_cfg.IsOpen()) {
        thrType = Thruster_cfg.GetValue();

        if (thrType == "FG_PROPELLER") {
          Thrusters.push_back(new FGPropeller(FDMExec, &Thruster_cfg));
	      } else if (thrType == "FG_NOZZLE") {
          Thrusters.push_back(new FGNozzle(FDMExec, &Thruster_cfg));
	      }

        AC_cfg->GetNextConfigLine();
        while ((token = AC_cfg->GetValue()) != "/AC_THRUSTER") {
	        *AC_cfg >> token;
          if (token == "XLOC") *AC_cfg >> xLoc;
          else if (token == "YLOC") *AC_cfg >> yLoc;
          else if (token == "ZLOC") *AC_cfg >> zLoc;
          else if (token == "PITCH") *AC_cfg >> Pitch;
          else if (token == "YAW") *AC_cfg >> Yaw;
          else cerr << "Unknown identifier: " << token << " in engine file: "
	                                                      << engineFileName << endl;
	      }

        Thrusters[numThrusters]->SetLocation(xLoc, yLoc, zLoc);
        Thrusters[numThrusters]->SetAnglesToBody(0, Pitch, Yaw);
        Thrusters[numThrusters]->SetdeltaT(dt*rate);

        numThrusters++;

	    } else {
        cerr << "Could not read thruster config file: " << fullpath
                                                + thrusterFileName + ".xml" << endl;
        return false;
      }

    }
    AC_cfg->GetNextConfigLine();
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPropulsion::Debug(void)
{
    //TODO: Add your source code here
}

