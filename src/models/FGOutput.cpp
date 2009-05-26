/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutput.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Manage output of sim parameters to file or stdout
 Called by:    FGSimExec

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created
11/09/07   HDW   Added FlightGear Socket Interface

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGOutput.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAerodynamics.h"
#include "FGGroundReactions.h"
#include "FGExternalReactions.h"
#include "FGBuoyantForces.h"
#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGPropagate.h"
#include "FGAuxiliary.h"
#include "FGInertial.h"
#include "FGPropulsion.h"   //access to FGEngine, FGTank
#include "models/propulsion/FGPiston.h"
#include <fstream>
#include <iomanip>
#include <cstring>

#include "input_output/net_fdm.hxx"

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <windows.h>
#else
#  include <netinet/in.h>       // htonl() ntohl()
#endif

static const int endianTest = 1;
#define isLittleEndian (*((char *) &endianTest ) != 0)

namespace JSBSim {

static const char *IdSrc = "$Id: FGOutput.cpp,v 1.35 2009/05/26 05:35:42 jberndt Exp $";
static const char *IdHdr = ID_OUTPUT;

// (stolen from FGFS native_fdm.cxx)
// The function htond is defined this way due to the way some
// processors and OSes treat floating point values.  Some will raise
// an exception whenever a "bad" floating point value is loaded into a
// floating point register.  Solaris is notorious for this, but then
// so is LynxOS on the PowerPC.  By translating the data in place,
// there is no need to load a FP register with the "corruped" floating
// point value.  By doing the BIG_ENDIAN test, I can optimize the
// routine for big-endian processors so it can be as efficient as
// possible
static void htond (double &x)
{
    if ( isLittleEndian ) {
        int    *Double_Overlay;
        int     Holding_Buffer;

        Double_Overlay = (int *) &x;
        Holding_Buffer = Double_Overlay [0];

        Double_Overlay [0] = htonl (Double_Overlay [1]);
        Double_Overlay [1] = htonl (Holding_Buffer);
    } else {
        return;
    }
}

// Float version
static void htonf (float &x)
{
    if ( isLittleEndian ) {
        int    *Float_Overlay;
        int     Holding_Buffer;

        Float_Overlay = (int *) &x;
        Holding_Buffer = Float_Overlay [0];

        Float_Overlay [0] = htonl (Holding_Buffer);
    } else {
        return;
    }
}


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGOutput::FGOutput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGOutput";
  sFirstPass = dFirstPass = true;
  socket = 0;
  flightGearSocket = 0;
  runID_postfix = 0;
  Type = otNone;
  SubSystems = 0;
  enabled = true;
  StartNewFile = false;
  delimeter = ", ";
  BaseFilename = Filename = "";
  DirectivesFile = "";
  output_file_name = "";

  memset(&fgSockBuf, 0x00, sizeof(fgSockBuf));

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutput::~FGOutput()
{
  delete socket;
  delete flightGearSocket;
  OutputProperties.clear();
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::InitModel(void)
{
  char fname[1000] = "";

  if (!FGModel::InitModel()) return false;

  if (Filename.size() > 0 && StartNewFile) {
    int idx = BaseFilename.find_last_of(".");
    int len = BaseFilename.length();
    string extension = "";
    if (idx != string::npos) {
      extension = BaseFilename.substr(idx, len-idx);
      len -= extension.length();
    }
    sprintf(fname, "%s_%d%s", BaseFilename.substr(0,len).c_str(), runID_postfix++, extension.c_str());
    Filename = string(fname);
    datafile.close();
    StartNewFile = false;
    dFirstPass = true;
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Run(void)
{
  if (FGModel::Run()) return true;

  if (enabled && !State->IntegrationSuspended()&& !FDMExec->Holding()) {
    if (Type == otSocket) {
      SocketOutput();
    } else if (Type == otFlightGear) {
      FlightGearSocketOutput();
    } else if (Type == otCSV || Type == otTab) {
      DelimitedOutput(Filename);
    } else if (Type == otTerminal) {
      // Not done yet
    } else if (Type == otNone) {
      // Do nothing
    } else {
      // Not a valid type of output
    }
  }
  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SetType(string type)
{
  if (type == "CSV") {
    Type = otCSV;
    delimeter = ", ";
  } else if (type == "TABULAR") {
    Type = otTab;
    delimeter = "\t";
  } else if (type == "SOCKET") {
    Type = otSocket;
  } else if (type == "FLIGHTGEAR") {
    Type = otFlightGear;
  } else if (type == "TERMINAL") {
    Type = otTerminal;
  } else if (type != string("NONE")) {
    Type = otUnknown;
    cerr << "Unknown type of output specified in config file" << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::DelimitedOutput(string fname)
{
  streambuf* buffer;
  string scratch = "";

  if (fname == "COUT" || fname == "cout") {
    buffer = cout.rdbuf();
  } else {
    if (!datafile.is_open()) datafile.open(fname.c_str());
    buffer = datafile.rdbuf();
  }

  ostream outstream(buffer);

  outstream.precision(10);

  if (dFirstPass) {
    outstream << "Time";
    if (SubSystems & ssSimulation) {
      // Nothing here, yet
    }
    if (SubSystems & ssAerosurfaces) {
      outstream << delimeter;
      outstream << "Aileron Command (norm)" + delimeter;
      outstream << "Elevator Command (norm)" + delimeter;
      outstream << "Rudder Command (norm)" + delimeter;
      outstream << "Flap Command (norm)" + delimeter;
      outstream << "Left Aileron Position (deg)" + delimeter;
      outstream << "Right Aileron Position (deg)" + delimeter;
      outstream << "Elevator Position (deg)" + delimeter;
      outstream << "Rudder Position (deg)" + delimeter;
      outstream << "Flap Position (deg)";
    }
    if (SubSystems & ssRates) {
      outstream << delimeter;
      outstream << "P (deg/s)" + delimeter + "Q (deg/s)" + delimeter + "R (deg/s)" + delimeter;
      outstream << "P dot (deg/s^2)" + delimeter + "Q dot (deg/s^2)" + delimeter + "R dot (deg/s^2)";
    }
    if (SubSystems & ssVelocities) {
      outstream << delimeter;
      outstream << "q bar (psf)" + delimeter;
      outstream << "Reynolds Number" + delimeter;
      outstream << "V_{Total} (ft/s)" + delimeter;
      outstream << "V_{Inertial} (ft/s)" + delimeter;
      outstream << "UBody" + delimeter + "VBody" + delimeter + "WBody" + delimeter;
      outstream << "Aero V_{X Body} (ft/s)" + delimeter + "Aero V_{Y Body} (ft/s)" + delimeter + "Aero V_{Z Body} (ft/s)" + delimeter;
      outstream << "V_{North} (ft/s)" + delimeter + "V_{East} (ft/s)" + delimeter + "V_{Down} (ft/s)";
    }
    if (SubSystems & ssForces) {
      outstream << delimeter;
      outstream << "F_{Drag} (lbs)" + delimeter + "F_{Side} (lbs)" + delimeter + "F_{Lift} (lbs)" + delimeter;
      outstream << "L/D" + delimeter;
      outstream << "F_{Aero x} (lbs)" + delimeter + "F_{Aero y} (lbs)" + delimeter + "F_{Aero z} (lbs)" + delimeter;
      outstream << "F_{Prop x} (lbs)" + delimeter + "F_{Prop y} (lbs)" + delimeter + "F_{Prop z} (lbs)" + delimeter;
      outstream << "F_{Gear x} (lbs)" + delimeter + "F_{Gear y} (lbs)" + delimeter + "F_{Gear z} (lbs)" + delimeter;
      outstream << "F_{Ext x} (lbs)" + delimeter + "F_{Ext y} (lbs)" + delimeter + "F_{Ext z} (lbs)" + delimeter;
      outstream << "F_{Buoyant x} (lbs)" + delimeter + "F_{Buoyant y} (lbs)" + delimeter + "F_{Buoyant z} (lbs)" + delimeter;
      outstream << "F_{Total x} (lbs)" + delimeter + "F_{Total y} (lbs)" + delimeter + "F_{Total z} (lbs)";
    }
    if (SubSystems & ssMoments) {
      outstream << delimeter;
      outstream << "L_{Aero} (ft-lbs)" + delimeter + "M_{Aero} ( ft-lbs)" + delimeter + "N_{Aero} (ft-lbs)" + delimeter;
      outstream << "L_{Prop} (ft-lbs)" + delimeter + "M_{Prop} (ft-lbs)" + delimeter + "N_{Prop} (ft-lbs)" + delimeter;
      outstream << "L_{Gear} (ft-lbs)" + delimeter + "M_{Gear} (ft-lbs)" + delimeter + "N_{Gear} (ft-lbs)" + delimeter;
      outstream << "L_{ext} (ft-lbs)" + delimeter + "M_{ext} (ft-lbs)" + delimeter + "N_{ext} (ft-lbs)" + delimeter;
      outstream << "L_{Buoyant} (ft-lbs)" + delimeter + "M_{Buoyant} (ft-lbs)" + delimeter + "N_{Buoyant} (ft-lbs)" + delimeter;
      outstream << "L_{Total} (ft-lbs)" + delimeter + "M_{Total} (ft-lbs)" + delimeter + "N_{Total} (ft-lbs)";
    }
    if (SubSystems & ssAtmosphere) {
      outstream << delimeter;
      outstream << "Rho (slugs/ft^3)" + delimeter;
      outstream << "Absolute Viscosity" + delimeter;
      outstream << "Kinematic Viscosity" + delimeter;
      outstream << "Temperature (R)" + delimeter;
      outstream << "P_{SL} (psf)" + delimeter;
      outstream << "P_{Ambient} (psf)" + delimeter;
      outstream << "Turbulence Magnitude (ft/sec)" + delimeter;
      outstream << "Turbulence X Direction (rad)" + delimeter + "Turbulence Y Direction (rad)" + delimeter + "Turbulence Z Direction (rad)" + delimeter;
      outstream << "Wind V_{North} (ft/s)" + delimeter + "Wind V_{East} (ft/s)" + delimeter + "Wind V_{Down} (ft/s)";
    }
    if (SubSystems & ssMassProps) {
      outstream << delimeter;
      outstream << "I_{xx}" + delimeter;
      outstream << "I_{xy}" + delimeter;
      outstream << "I_{xz}" + delimeter;
      outstream << "I_{yx}" + delimeter;
      outstream << "I_{yy}" + delimeter;
      outstream << "I_{yz}" + delimeter;
      outstream << "I_{zx}" + delimeter;
      outstream << "I_{zy}" + delimeter;
      outstream << "I_{zz}" + delimeter;
      outstream << "Mass" + delimeter;
      outstream << "X_{cg}" + delimeter + "Y_{cg}" + delimeter + "Z_{cg}";
    }
    if (SubSystems & ssPropagate) {
      outstream << delimeter;
      outstream << "Altitude ASL (ft)" + delimeter;
      outstream << "Altitude AGL (ft)" + delimeter;
      outstream << "Phi (deg)" + delimeter + "Theta (deg)" + delimeter + "Psi (deg)" + delimeter;
      outstream << "Alpha (deg)" + delimeter;
      outstream << "Beta (deg)" + delimeter;
      outstream << "Latitude (deg)" + delimeter;
      outstream << "Longitude (deg)" + delimeter;
      outstream << "ECEF X (ft)" + delimeter + "ECEF Y (ft)" + delimeter + "ECEF Z (ft)" + delimeter;
      outstream << "EPA (deg)" + delimeter;
      outstream << "Distance AGL (ft)" + delimeter;
      outstream << "Terrain Radius (ft)";
    }
    if (SubSystems & ssCoefficients) {
      scratch = Aerodynamics->GetCoefficientStrings(delimeter);
      if (scratch.length() != 0) outstream << delimeter << scratch;
    }
    if (SubSystems & ssFCS) {
      scratch = FCS->GetComponentStrings(delimeter);
      if (scratch.length() != 0) outstream << delimeter << scratch;
    }
    if (SubSystems & ssGroundReactions) {
      outstream << delimeter;
      outstream << GroundReactions->GetGroundReactionStrings(delimeter);
    }
    if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
      outstream << delimeter;
      outstream << Propulsion->GetPropulsionStrings(delimeter);
    }
    if (OutputProperties.size() > 0) {
      for (unsigned int i=0;i<OutputProperties.size();i++) {
        outstream << delimeter << OutputProperties[i]->GetPrintableName();
      }
    }

    outstream << endl;
    dFirstPass = false;
  }

  outstream << State->Getsim_time();
  if (SubSystems & ssSimulation) {
  }
  if (SubSystems & ssAerosurfaces) {
    outstream << delimeter;
    outstream << FCS->GetDaCmd() << delimeter;
    outstream << FCS->GetDeCmd() << delimeter;
    outstream << FCS->GetDrCmd() << delimeter;
    outstream << FCS->GetDfCmd() << delimeter;
    outstream << FCS->GetDaLPos(ofDeg) << delimeter;
    outstream << FCS->GetDaRPos(ofDeg) << delimeter;
    outstream << FCS->GetDePos(ofDeg) << delimeter;
    outstream << FCS->GetDrPos(ofDeg) << delimeter;
    outstream << FCS->GetDfPos(ofDeg);
  }
  if (SubSystems & ssRates) {
    outstream << delimeter;
    outstream << (radtodeg*Propagate->GetPQR()).Dump(delimeter) << delimeter;
    outstream << (radtodeg*Propagate->GetPQRdot()).Dump(delimeter);
  }
  if (SubSystems & ssVelocities) {
    outstream << delimeter;
    outstream << Auxiliary->Getqbar() << delimeter;
    outstream << Auxiliary->GetReynoldsNumber() << delimeter;
    outstream << setprecision(12) << Auxiliary->GetVt() << delimeter;
    outstream << Propagate->GetInertialVelocityMagnitude() << delimeter;
    outstream << setprecision(12) << Propagate->GetUVW().Dump(delimeter) << delimeter;
    outstream << Auxiliary->GetAeroUVW().Dump(delimeter) << delimeter;
    outstream << Propagate->GetVel().Dump(delimeter);
    outstream.precision(10);
  }
  if (SubSystems & ssForces) {
    outstream << delimeter;
    outstream << Aerodynamics->GetvFw() << delimeter;
    outstream << Aerodynamics->GetLoD() << delimeter;
    outstream << Aerodynamics->GetForces() << delimeter;
    outstream << Propulsion->GetForces() << delimeter;
    outstream << GroundReactions->GetForces() << delimeter;
    outstream << ExternalReactions->GetForces() << delimeter;
    outstream << BuoyantForces->GetForces() << delimeter;
    outstream << Aircraft->GetForces().Dump(delimeter);
  }
  if (SubSystems & ssMoments) {
    outstream << delimeter;
    outstream << Aerodynamics->GetMoments() << delimeter;
    outstream << Propulsion->GetMoments() << delimeter;
    outstream << GroundReactions->GetMoments() << delimeter;
    outstream << ExternalReactions->GetMoments() << delimeter;
    outstream << BuoyantForces->GetMoments() << delimeter;
    outstream << Aircraft->GetMoments().Dump(delimeter);
  }
  if (SubSystems & ssAtmosphere) {
    outstream << delimeter;
    outstream << Atmosphere->GetDensity() << delimeter;
    outstream << Atmosphere->GetAbsoluteViscosity() << delimeter;
    outstream << Atmosphere->GetKinematicViscosity() << delimeter;
    outstream << Atmosphere->GetTemperature() << delimeter;
    outstream << Atmosphere->GetPressureSL() << delimeter;
    outstream << Atmosphere->GetPressure() << delimeter;
    outstream << Atmosphere->GetTurbMagnitude() << delimeter;
    outstream << Atmosphere->GetTurbDirection().Dump(delimeter) << delimeter;
    outstream << Atmosphere->GetTotalWindNED().Dump(delimeter);
  }
  if (SubSystems & ssMassProps) {
    outstream << delimeter;
    outstream << MassBalance->GetJ() << delimeter;
    outstream << MassBalance->GetMass() << delimeter;
    outstream << MassBalance->GetXYZcg();
  }
  if (SubSystems & ssPropagate) {
    outstream.precision(14);
    outstream << delimeter;
    outstream << Propagate->GetAltitudeASL() << delimeter;
    outstream << Propagate->GetDistanceAGL() << delimeter;
    outstream << (radtodeg*Propagate->GetEuler()).Dump(delimeter) << delimeter;
    outstream << Auxiliary->Getalpha(inDegrees) << delimeter;
    outstream << Auxiliary->Getbeta(inDegrees) << delimeter;
    outstream << Propagate->GetLocation().GetLatitudeDeg() << delimeter;
    outstream << Propagate->GetLocation().GetLongitudeDeg() << delimeter;
    outstream.precision(18);
    outstream << ((FGColumnVector3)Propagate->GetLocation()).Dump(delimeter) << delimeter;
    outstream.precision(14);
    outstream << Inertial->GetEarthPositionAngleDeg() << delimeter;
    outstream << Propagate->GetDistanceAGL() << delimeter;
    outstream << Propagate->GetLocalTerrainRadius();
    outstream.precision(10);
  }
  if (SubSystems & ssCoefficients) {
    scratch = Aerodynamics->GetCoefficientValues(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentValues(delimeter);
    if (scratch.length() != 0) outstream << delimeter << scratch;
  }
  if (SubSystems & ssGroundReactions) {
    outstream << delimeter;
    outstream << GroundReactions->GetGroundReactionValues(delimeter);
  }
  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
    outstream << delimeter;
    outstream << Propulsion->GetPropulsionValues(delimeter);
  }

  for (unsigned int i=0;i<OutputProperties.size();i++) {
    outstream << delimeter << OutputProperties[i]->getDoubleValue();
  }

  outstream << endl;
  outstream.flush();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SocketDataFill(FGNetFDM* net)
{
    unsigned int i;

    // Version
    net->version = FG_NET_FDM_VERSION;

    // Positions
    net->longitude = Propagate->GetLocation().GetLongitude(); // geodetic (radians)
    net->latitude  = Propagate->GetLocation().GetLatitude(); // geodetic (radians)
    net->altitude  = Propagate->GetAltitudeASL()*0.3048; // altitude, above sea level (meters)
    net->agl       = (float)(Propagate->GetDistanceAGL()*0.3048); // altitude, above ground level (meters)

    net->phi       = (float)(Propagate->GetEuler(ePhi)); // roll (radians)
    net->theta     = (float)(Propagate->GetEuler(eTht)); // pitch (radians)
    net->psi       = (float)(Propagate->GetEuler(ePsi)); // yaw or true heading (radians)

    net->alpha     = (float)(Auxiliary->Getalpha()); // angle of attack (radians)
    net->beta      = (float)(Auxiliary->Getbeta()); // side slip angle (radians)

    // Velocities
    net->phidot     = (float)(Auxiliary->GetEulerRates(ePhi)); // roll rate (radians/sec)
    net->thetadot   = (float)(Auxiliary->GetEulerRates(eTht)); // pitch rate (radians/sec)
    net->psidot     = (float)(Auxiliary->GetEulerRates(ePsi)); // yaw rate (radians/sec)
    net->vcas       = (float)(Auxiliary->GetVcalibratedFPS()); // VCAS, ft/sec
    net->climb_rate = (float)(Propagate->Gethdot());           // altitude rate, ft/sec
    net->v_north    = (float)(Propagate->GetVel(eNorth));      // north vel in NED frame, fps
    net->v_east     = (float)(Propagate->GetVel(eEast));       // east vel in NED frame, fps
    net->v_down     = (float)(Propagate->GetVel(eDown));       // down vel in NED frame, fps
//---ADD METHOD TO CALCULATE THESE TERMS---
    net->v_wind_body_north = (float)(Propagate->GetVel(eNorth)); // north vel in NED relative to airmass, fps
    net->v_wind_body_east = (float)(Propagate->GetVel(eEast)); // east vel in NED relative to airmass, fps
    net->v_wind_body_down = (float)(Propagate->GetVel(eDown)); // down vel in NED relative to airmass, fps

    // Accelerations
    net->A_X_pilot   = (float)(Auxiliary->GetPilotAccel(1));    // X body accel, ft/s/s
    net->A_Y_pilot   = (float)(Auxiliary->GetPilotAccel(2));    // Y body accel, ft/s/s
    net->A_Z_pilot   = (float)(Auxiliary->GetPilotAccel(3));    // Z body accel, ft/s/s

    // Stall
    net->stall_warning = 0.0;  // 0.0 - 1.0 indicating the amount of stall
    net->slip_deg    = (float)(Auxiliary->Getbeta(inDegrees));  // slip ball deflection, deg

    // Engine status
    net->num_engines = Propulsion->GetNumEngines(); // Number of valid engines

    for (i=0; i<net->num_engines; i++) {
       if (Propulsion->GetEngine(i)->GetRunning())
          net->eng_state[i] = 2;       // Engine state running
       else if (Propulsion->GetEngine(i)->GetCranking())
          net->eng_state[i] = 1;       // Engine state cranking
       else
          net->eng_state[i] = 0;       // Engine state off

       switch (Propulsion->GetEngine(i)->GetType()) {
       case (FGEngine::etRocket):
       break;
       case (FGEngine::etPiston):
          net->rpm[i]       = (float)(((FGPiston *)Propulsion->GetEngine(i))->getRPM());
          net->fuel_flow[i] = (float)(((FGPiston *)Propulsion->GetEngine(i))->getFuelFlow_gph());
          net->fuel_px[i]   = 0; // Fuel pressure, psi  (N/A in current model)
          net->egt[i]       = (float)(((FGPiston *)Propulsion->GetEngine(i))->GetEGT());
          net->cht[i]       = (float)(((FGPiston *)Propulsion->GetEngine(i))->getCylinderHeadTemp_degF());
          net->mp_osi[i]    = (float)(((FGPiston *)Propulsion->GetEngine(i))->getManifoldPressure_inHg());
          net->oil_temp[i]  = (float)(((FGPiston *)Propulsion->GetEngine(i))->getOilTemp_degF());
          net->oil_px[i]    = (float)(((FGPiston *)Propulsion->GetEngine(i))->getOilPressure_psi());
          net->tit[i]       = 0; // Turbine Inlet Temperature  (N/A for piston)
       break;
       case (FGEngine::etTurbine):
       break;
       case (FGEngine::etTurboprop):
       break;
       case (FGEngine::etElectric):
       break;
       case (FGEngine::etUnknown):
       break;
       }
    }


    // Consumables
    net->num_tanks = Propulsion->GetNumTanks();   // Max number of fuel tanks

    for (i=0; i<net->num_tanks; i++) {
       net->fuel_quantity[i] = (float)(((FGTank *)Propulsion->GetTank(i))->GetContents());
    }


    // Gear status
    net->num_wheels  = GroundReactions->GetNumGearUnits();

    for (i=0; i<net->num_wheels; i++) {
       net->wow[i]              = GroundReactions->GetGearUnit(i)->GetWOW();
       if (GroundReactions->GetGearUnit(i)->GetGearUnitDown())
          net->gear_pos[i]      = 1;  //gear down, using FCS convention
       else
          net->gear_pos[i]      = 0;  //gear up, using FCS convention
       net->gear_steer[i]       = (float)(GroundReactions->GetGearUnit(i)->GetSteerNorm());
       net->gear_compression[i] = (float)(GroundReactions->GetGearUnit(i)->GetCompLen());
    }


    // Environment
    net->cur_time    = (long int)1234567890;	// Friday, Feb 13, 2009, 23:31:30 UTC (not processed by FGFS anyway)
    net->warp        = 0;                       // offset in seconds to unix time
    net->visibility  = 25000.0;                 // visibility in meters (for env. effects)


    // Control surface positions (normalized values)
    net->elevator          = (float)(FCS->GetDePos(ofNorm));    // Norm Elevator Pos, --
    net->elevator_trim_tab = (float)(FCS->GetPitchTrimCmd());   // Norm Elev Trim Tab Pos, --
    net->left_flap         = (float)(FCS->GetDfPos(ofNorm));    // Norm Flap Pos, --
    net->right_flap        = (float)(FCS->GetDfPos(ofNorm));    // Norm Flap Pos, --
    net->left_aileron      = (float)(FCS->GetDaLPos(ofNorm));   // Norm L Aileron Pos, --
    net->right_aileron     = (float)(FCS->GetDaRPos(ofNorm));   // Norm R Aileron Pos, --
    net->rudder            = (float)(FCS->GetDrPos(ofNorm));    // Norm Rudder Pos, --
    net->nose_wheel        = (float)(FCS->GetDrPos(ofNorm));    // *** FIX ***  Using Rudder Pos for NWS, --
    net->speedbrake        = (float)(FCS->GetDsbPos(ofNorm));   // Norm Speedbrake Pos, --
    net->spoilers          = (float)(FCS->GetDspPos(ofNorm));   // Norm Spoiler Pos, --


    // Convert the net buffer to network format
    if ( isLittleEndian ) {
        net->version = htonl(net->version);

        htond(net->longitude);
        htond(net->latitude);
        htond(net->altitude);
        htonf(net->agl);
        htonf(net->phi);
        htonf(net->theta);
        htonf(net->psi);
        htonf(net->alpha);
        htonf(net->beta);

        htonf(net->phidot);
        htonf(net->thetadot);
        htonf(net->psidot);
        htonf(net->vcas);
        htonf(net->climb_rate);
        htonf(net->v_north);
        htonf(net->v_east);
        htonf(net->v_down);
        htonf(net->v_wind_body_north);
        htonf(net->v_wind_body_east);
        htonf(net->v_wind_body_down);

        htonf(net->A_X_pilot);
        htonf(net->A_Y_pilot);
        htonf(net->A_Z_pilot);

        htonf(net->stall_warning);
        htonf(net->slip_deg);

        for (i=0; i<net->num_engines; ++i ) {
            net->eng_state[i] = htonl(net->eng_state[i]);
            htonf(net->rpm[i]);
            htonf(net->fuel_flow[i]);
            htonf(net->fuel_px[i]);
            htonf(net->egt[i]);
            htonf(net->cht[i]);
            htonf(net->mp_osi[i]);
            htonf(net->tit[i]);
            htonf(net->oil_temp[i]);
            htonf(net->oil_px[i]);
        }
        net->num_engines = htonl(net->num_engines);

        for (i=0; i<net->num_tanks; ++i ) {
            htonf(net->fuel_quantity[i]);
        }
        net->num_tanks = htonl(net->num_tanks);

        for (i=0; i<net->num_wheels; ++i ) {
            net->wow[i] = htonl(net->wow[i]);
            htonf(net->gear_pos[i]);
            htonf(net->gear_steer[i]);
            htonf(net->gear_compression[i]);
        }
        net->num_wheels = htonl(net->num_wheels);

        net->cur_time = htonl( net->cur_time );
        net->warp = htonl( net->warp );
        htonf(net->visibility);

        htonf(net->elevator);
        htonf(net->elevator_trim_tab);
        htonf(net->left_flap);
        htonf(net->right_flap);
        htonf(net->left_aileron);
        htonf(net->right_aileron);
        htonf(net->rudder);
        htonf(net->nose_wheel);
        htonf(net->speedbrake);
        htonf(net->spoilers);
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::FlightGearSocketOutput(void)
{
  int length = sizeof(fgSockBuf);


  if (flightGearSocket == NULL) return;
  if (!flightGearSocket->GetConnectStatus()) return;

  SocketDataFill(&fgSockBuf);
  flightGearSocket->Send((char *)&fgSockBuf, length);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SocketOutput(void)
{
  string asciiData, scratch;

  if (socket == NULL) return;
  if (!socket->GetConnectStatus()) return;

  socket->Clear();
  if (sFirstPass) {
    socket->Clear("<LABELS>");
    socket->Append("Time");

    if (SubSystems & ssAerosurfaces) {
      socket->Append("Aileron Command");
      socket->Append("Elevator Command");
      socket->Append("Rudder Command");
      socket->Append("Flap Command");
      socket->Append("Left Aileron Position");
      socket->Append("Right Aileron Position");
      socket->Append("Elevator Position");
      socket->Append("Rudder Position");
      socket->Append("Flap Position");
    }

    if (SubSystems & ssRates) {
      socket->Append("P");
      socket->Append("Q");
      socket->Append("R");
      socket->Append("PDot");
      socket->Append("QDot");
      socket->Append("RDot");
    }

    if (SubSystems & ssVelocities) {
      socket->Append("QBar");
      socket->Append("Vtotal");
      socket->Append("UBody");
      socket->Append("VBody");
      socket->Append("WBody");
      socket->Append("UAero");
      socket->Append("VAero");
      socket->Append("WAero");
      socket->Append("Vn");
      socket->Append("Ve");
      socket->Append("Vd");
    }
    if (SubSystems & ssForces) {
      socket->Append("F_Drag");
      socket->Append("F_Side");
      socket->Append("F_Lift");
      socket->Append("LoD");
      socket->Append("Fx");
      socket->Append("Fy");
      socket->Append("Fz");
    }
    if (SubSystems & ssMoments) {
      socket->Append("L");
      socket->Append("M");
      socket->Append("N");
    }
    if (SubSystems & ssAtmosphere) {
      socket->Append("Rho");
      socket->Append("SL pressure");
      socket->Append("Ambient pressure");
      socket->Append("Turbulence Magnitude");
      socket->Append("Turbulence Direction X");
      socket->Append("Turbulence Direction Y");
      socket->Append("Turbulence Direction Z");
      socket->Append("NWind");
      socket->Append("EWind");
      socket->Append("DWind");
    }
    if (SubSystems & ssMassProps) {
      socket->Append("Ixx");
      socket->Append("Ixy");
      socket->Append("Ixz");
      socket->Append("Iyx");
      socket->Append("Iyy");
      socket->Append("Iyz");
      socket->Append("Izx");
      socket->Append("Izy");
      socket->Append("Izz");
      socket->Append("Mass");
      socket->Append("Xcg");
      socket->Append("Ycg");
      socket->Append("Zcg");
    }
    if (SubSystems & ssPropagate) {
        socket->Append("Altitude");
        socket->Append("Phi (deg)");
        socket->Append("Tht (deg)");
        socket->Append("Psi (deg)");
        socket->Append("Alpha (deg)");
        socket->Append("Beta (deg)");
        socket->Append("Latitude (deg)");
        socket->Append("Longitude (deg)");
    }
    if (SubSystems & ssCoefficients) {
      scratch = Aerodynamics->GetCoefficientStrings(",");
      if (scratch.length() != 0) socket->Append(scratch);
    }
    if (SubSystems & ssFCS) {
      scratch = FCS->GetComponentStrings(",");
      if (scratch.length() != 0) socket->Append(scratch);
    }
    if (SubSystems & ssGroundReactions) {
      socket->Append(GroundReactions->GetGroundReactionStrings(","));
    }
    if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
      socket->Append(Propulsion->GetPropulsionStrings(","));
    }
    if (OutputProperties.size() > 0) {
      for (unsigned int i=0;i<OutputProperties.size();i++) {
        socket->Append(OutputProperties[i]->GetPrintableName());
      }
    }

    sFirstPass = false;
    socket->Send();
  }

  socket->Clear();
  socket->Append(State->Getsim_time());

  if (SubSystems & ssAerosurfaces) {
    socket->Append(FCS->GetDaCmd());
    socket->Append(FCS->GetDeCmd());
    socket->Append(FCS->GetDrCmd());
    socket->Append(FCS->GetDfCmd());
    socket->Append(FCS->GetDaLPos());
    socket->Append(FCS->GetDaRPos());
    socket->Append(FCS->GetDePos());
    socket->Append(FCS->GetDrPos());
    socket->Append(FCS->GetDfPos());
  }
  if (SubSystems & ssRates) {
    socket->Append(radtodeg*Propagate->GetPQR(eP));
    socket->Append(radtodeg*Propagate->GetPQR(eQ));
    socket->Append(radtodeg*Propagate->GetPQR(eR));
    socket->Append(radtodeg*Propagate->GetPQRdot(eP));
    socket->Append(radtodeg*Propagate->GetPQRdot(eQ));
    socket->Append(radtodeg*Propagate->GetPQRdot(eR));
  }
  if (SubSystems & ssVelocities) {
    socket->Append(Auxiliary->Getqbar());
    socket->Append(Auxiliary->GetVt());
    socket->Append(Propagate->GetUVW(eU));
    socket->Append(Propagate->GetUVW(eV));
    socket->Append(Propagate->GetUVW(eW));
    socket->Append(Auxiliary->GetAeroUVW(eU));
    socket->Append(Auxiliary->GetAeroUVW(eV));
    socket->Append(Auxiliary->GetAeroUVW(eW));
    socket->Append(Propagate->GetVel(eNorth));
    socket->Append(Propagate->GetVel(eEast));
    socket->Append(Propagate->GetVel(eDown));
  }
  if (SubSystems & ssForces) {
    socket->Append(Aerodynamics->GetvFw()(eDrag));
    socket->Append(Aerodynamics->GetvFw()(eSide));
    socket->Append(Aerodynamics->GetvFw()(eLift));
    socket->Append(Aerodynamics->GetLoD());
    socket->Append(Aircraft->GetForces(eX));
    socket->Append(Aircraft->GetForces(eY));
    socket->Append(Aircraft->GetForces(eZ));
  }
  if (SubSystems & ssMoments) {
    socket->Append(Aircraft->GetMoments(eL));
    socket->Append(Aircraft->GetMoments(eM));
    socket->Append(Aircraft->GetMoments(eN));
  }
  if (SubSystems & ssAtmosphere) {
    socket->Append(Atmosphere->GetDensity());
    socket->Append(Atmosphere->GetPressureSL());
    socket->Append(Atmosphere->GetPressure());
    socket->Append(Atmosphere->GetTurbMagnitude());
    socket->Append(Atmosphere->GetTurbDirection().Dump(","));
    socket->Append(Atmosphere->GetTotalWindNED().Dump(","));
  }
  if (SubSystems & ssMassProps) {
    socket->Append(MassBalance->GetJ()(1,1));
    socket->Append(MassBalance->GetJ()(1,2));
    socket->Append(MassBalance->GetJ()(1,3));
    socket->Append(MassBalance->GetJ()(2,1));
    socket->Append(MassBalance->GetJ()(2,2));
    socket->Append(MassBalance->GetJ()(2,3));
    socket->Append(MassBalance->GetJ()(3,1));
    socket->Append(MassBalance->GetJ()(3,2));
    socket->Append(MassBalance->GetJ()(3,3));
    socket->Append(MassBalance->GetMass());
    socket->Append(MassBalance->GetXYZcg()(eX));
    socket->Append(MassBalance->GetXYZcg()(eY));
    socket->Append(MassBalance->GetXYZcg()(eZ));
  }
  if (SubSystems & ssPropagate) {
    socket->Append(Propagate->GetAltitudeASL());
    socket->Append(radtodeg*Propagate->GetEuler(ePhi));
    socket->Append(radtodeg*Propagate->GetEuler(eTht));
    socket->Append(radtodeg*Propagate->GetEuler(ePsi));
    socket->Append(Auxiliary->Getalpha(inDegrees));
    socket->Append(Auxiliary->Getbeta(inDegrees));
    socket->Append(Propagate->GetLocation().GetLatitudeDeg());
    socket->Append(Propagate->GetLocation().GetLongitudeDeg());
  }
  if (SubSystems & ssCoefficients) {
    scratch = Aerodynamics->GetCoefficientValues(",");
    if (scratch.length() != 0) socket->Append(scratch);
  }
  if (SubSystems & ssFCS) {
    scratch = FCS->GetComponentValues(",");
    if (scratch.length() != 0) socket->Append(scratch);
  }
  if (SubSystems & ssGroundReactions) {
    socket->Append(GroundReactions->GetGroundReactionValues(","));
  }
  if (SubSystems & ssPropulsion && Propulsion->GetNumEngines() > 0) {
    socket->Append(Propulsion->GetPropulsionValues(","));
  }

  for (unsigned int i=0;i<OutputProperties.size();i++) {
    socket->Append(OutputProperties[i]->getDoubleValue());
  }

  socket->Send();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutput::SocketStatusOutput(string out_str)
{
  string asciiData;

  if (socket == NULL) return;

  socket->Clear();
  asciiData = string("<STATUS>") + out_str;
  socket->Append(asciiData.c_str());
  socket->Send();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutput::Load(Element* element)
{
  string type="", parameter="";
  string name="";
  string protocol="tcp";
  int OutRate = 0;
  string property;
  unsigned int port;
  Element *property_element;

  string separator = "/";

  if (!DirectivesFile.empty()) { // A directives filename from the command line overrides
    output_file_name = DirectivesFile;      // one found in the config file.
    document = LoadXMLDocument(output_file_name);
  } else if (!element->GetAttributeValue("file").empty()) {
    output_file_name = element->GetAttributeValue("file");
    document = LoadXMLDocument(output_file_name);
  } else {
    document = element;
  }

  name = document->GetAttributeValue("name");
  type = document->GetAttributeValue("type");
  SetType(type);
  if (!document->GetAttributeValue("port").empty() && type == string("SOCKET")) {
    port = atoi(document->GetAttributeValue("port").c_str());
    socket = new FGfdmSocket(name, port);
  } else if (!document->GetAttributeValue("port").empty() && type == string("FLIGHTGEAR")) {
    port = atoi(document->GetAttributeValue("port").c_str());
    if (!document->GetAttributeValue("protocol").empty())
       protocol = document->GetAttributeValue("protocol");
    if (protocol == "udp")
       flightGearSocket = new FGfdmSocket(name, port, FGfdmSocket::ptUDP);  // create udp socket
    else
       flightGearSocket = new FGfdmSocket(name, port, FGfdmSocket::ptTCP);  // create tcp socket (default)
  } else {
    BaseFilename = Filename = name;
  }
  if (!document->GetAttributeValue("rate").empty()) {
    OutRate = (int)document->GetAttributeValueAsNumber("rate");
  } else {
    OutRate = 1;
  }

  if (document->FindElementValue("simulation") == string("ON"))
    SubSystems += ssSimulation;
  if (document->FindElementValue("aerosurfaces") == string("ON"))
    SubSystems += ssAerosurfaces;
  if (document->FindElementValue("rates") == string("ON"))
    SubSystems += ssRates;
  if (document->FindElementValue("velocities") == string("ON"))
    SubSystems += ssVelocities;
  if (document->FindElementValue("forces") == string("ON"))
    SubSystems += ssForces;
  if (document->FindElementValue("moments") == string("ON"))
    SubSystems += ssMoments;
  if (document->FindElementValue("atmosphere") == string("ON"))
    SubSystems += ssAtmosphere;
  if (document->FindElementValue("massprops") == string("ON"))
    SubSystems += ssMassProps;
  if (document->FindElementValue("position") == string("ON"))
    SubSystems += ssPropagate;
  if (document->FindElementValue("coefficients") == string("ON"))
    SubSystems += ssCoefficients;
  if (document->FindElementValue("ground_reactions") == string("ON"))
    SubSystems += ssGroundReactions;
  if (document->FindElementValue("fcs") == string("ON"))
    SubSystems += ssFCS;
  if (document->FindElementValue("propulsion") == string("ON"))
    SubSystems += ssPropulsion;
  property_element = document->FindElement("property");
  while (property_element) {
    string property_str = property_element->GetDataLine();
    FGPropertyManager* node = PropertyManager->GetNode(property_str);
    if (!node) {
      cerr << fgred << highint << endl << "  No property by the name "
           << property_str << " has been defined. This property will " << endl
           << "  not be logged. You should check your configuration file."
           << reset << endl;
    } else {
      OutputProperties.push_back(node);
    }
    property_element = document->FindNextElement("property");
  }

  OutRate = OutRate>1000?1000:(OutRate<0?0:OutRate);
  rate = (int)(0.5 + 1.0/(State->Getdt()*OutRate));

  Debug(2);

  return true;
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

void FGOutput::Debug(int from)
{
  string scratch="";

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
    if (from == 2) {
      if (output_file_name.empty())
        cout << "  " << "Output parameters read inline" << endl;
      else
        cout << "    Output parameters read from file: " << output_file_name << endl;

      if (Filename == "cout" || Filename == "COUT") {
        scratch = "    Log output goes to screen console";
      } else if (!Filename.empty()) {
        scratch = "    Log output goes to file: " + Filename;
      }
      switch (Type) {
      case otCSV:
        cout << scratch << " in CSV format output at rate " << 1/(State->Getdt()*rate) << " Hz" << endl;
        break;
      case otNone:
        cout << "  No log output" << endl;
        break;
      }

      if (SubSystems & ssSimulation)      cout << "    Simulation parameters logged" << endl;
      if (SubSystems & ssAerosurfaces)    cout << "    Aerosurface parameters logged" << endl;
      if (SubSystems & ssRates)           cout << "    Rate parameters logged" << endl;
      if (SubSystems & ssVelocities)      cout << "    Velocity parameters logged" << endl;
      if (SubSystems & ssForces)          cout << "    Force parameters logged" << endl;
      if (SubSystems & ssMoments)         cout << "    Moments parameters logged" << endl;
      if (SubSystems & ssAtmosphere)      cout << "    Atmosphere parameters logged" << endl;
      if (SubSystems & ssMassProps)       cout << "    Mass parameters logged" << endl;
      if (SubSystems & ssCoefficients)    cout << "    Coefficient parameters logged" << endl;
      if (SubSystems & ssPropagate)       cout << "    Propagate parameters logged" << endl;
      if (SubSystems & ssGroundReactions) cout << "    Ground parameters logged" << endl;
      if (SubSystems & ssFCS)             cout << "    FCS parameters logged" << endl;
      if (SubSystems & ssPropulsion)      cout << "    Propulsion parameters logged" << endl;
      if (OutputProperties.size() > 0)    cout << "    Properties logged:" << endl;
      for (unsigned int i=0;i<OutputProperties.size();i++) {
        cout << "      - " << OutputProperties[i]->GetName() << endl;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGOutput" << endl;
    if (from == 1) cout << "Destroyed:    FGOutput" << endl;
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
