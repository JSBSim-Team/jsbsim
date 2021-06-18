/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutputFG.cpp
 Author:       Bertrand Coconnier
 Date started: 09/10/11
 Purpose:      Manage output of sim parameters to FlightGear
 Called by:    FGOutput

 ------------- Copyright (C) 2011 Bertrand Coconnier -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This is the place where you create output routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
11/09/07   HDW   Added FlightGear Socket Interface
09/10/11   BC    Moved the FlightGear socket in a separate class

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstring>

#include "FGOutputFG.h"
#include "FGXMLElement.h"
#include "models/FGAuxiliary.h"
#include "models/FGPropulsion.h"
#include "models/FGFCS.h"
#include "models/propulsion/FGPiston.h"
#include "models/propulsion/FGElectric.h"
#include "models/propulsion/FGTank.h"

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <windows.h>
#else
#  include <netinet/in.h>       // htonl() ntohl()
#endif

#if !defined (min)
#  define min(X,Y) X<Y?X:Y
#endif

static const int endianTest = 1;
#define isLittleEndian (*((char *) &endianTest ) != 0)

using namespace std;

namespace JSBSim {

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

FGOutputFG::FGOutputFG(FGFDMExec *fdmex)
    : FGOutputSocket(fdmex), outputOptions{false, 1e6}, net3(nullptr),
      dataLength(0)
{
  memset(data, 0x0, s);

  if (fdmex->GetDebugLevel() > 0) {
    // Engine status
    if (Propulsion->GetNumEngines() > FG_MAX_ENGINES)
      cerr << "This vehicle has " << Propulsion->GetNumEngines() << " engines, but the current " << endl
           << "version of FlightGear's FGNetFDM only supports " << FG_MAX_ENGINES << " engines." << endl
           << "Only the first " << FG_MAX_ENGINES << " engines will be used." << endl;

    // Consumables
    if (Propulsion->GetNumTanks() > FG_MAX_TANKS)
      cerr << "This vehicle has " << Propulsion->GetNumTanks() << " tanks, but the current " << endl
           << "version of FlightGear's FGNetFDM only supports " << FG_MAX_TANKS << " tanks." << endl
           << "Only the first " << FG_MAX_TANKS << " tanks will be used." << endl;

    // Gear status
    if (GroundReactions->GetNumGearUnits() > FG_MAX_WHEELS)
      cerr << "This vehicle has " << GroundReactions->GetNumGearUnits() << " bogeys, but the current " << endl
           << "version of FlightGear's FGNetFDM only supports " << FG_MAX_WHEELS << " bogeys." << endl
           << "Only the first " << FG_MAX_WHEELS << " bogeys will be used." << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGOutputFG::Load(Element* el)
{
  if (!FGOutputSocket::Load(el)) {
    return false;
  }

  // Check if there is a <time> element
  Element* time_el = el->FindElement("time");
  if (time_el) {
    // Check if the attribute "type" is specified and is set to "simulation"
    if (time_el->HasAttribute("type") && time_el->GetAttributeValue("type") == "simulation") {
      outputOptions.useSimTime = true;
    }

    // Check if the attribute "resolution" is specified and set to a valid value
    if (time_el->HasAttribute("resolution")) {
      if (time_el->GetAttributeValueAsNumber("resolution") <= 1 &&
          time_el->GetAttributeValueAsNumber("resolution") >= 1e-9) {
        outputOptions.timeFactor = 1./time_el->GetAttributeValueAsNumber("resolution");
      } else {
        return false;
      }
    }
  }

  // Set the version of the FDM network protocol
  net1->version = htonl(24);
  net3 = (FGNetFDM3 *)(net1 + 1);
  dataLength = sizeof(FGNetFDM1) + sizeof(FGNetFDM3);

  if (el->HasAttribute("version"))
  {
    unsigned int version = static_cast<unsigned int>(el->GetAttributeValueAsNumber("version"));
    switch (version)
    {
    case 24: // FlightGear 2020.2 and earlier
      break;
    case 25: // FlightGear 2020.3 and later
      net1->version = htonl(25);
      net3 = (FGNetFDM3 *)((char *)net3 + sizeof(FGNetFDM2));
      dataLength += sizeof(FGNetFDM2);
      break;
    default:
      cerr << "Invalid FDM protocol version: " << version << endl;
      return false;
    }
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputFG::SocketDataFill(void)
{
  unsigned int i;

  // Positions
  net1->longitude = Propagate->GetLongitude();               // longitude (radians)
  net1->latitude = Propagate->GetGeodLatitudeRad();          // geodetic (radians)
  net1->altitude = Propagate->GetAltitudeASL() * 0.3048;     // altitude, above sea level (meters)
  net1->agl = (float)(Propagate->GetDistanceAGL() * 0.3048); // altitude, above ground level (meters)

  net1->phi = (float)(Propagate->GetEuler(ePhi));   // roll (radians)
  net1->theta = (float)(Propagate->GetEuler(eTht)); // pitch (radians)
  net1->psi = (float)(Propagate->GetEuler(ePsi));   // yaw or true heading (radians)

  net1->alpha = (float)(Auxiliary->Getalpha()); // angle of attack (radians)
  net1->beta = (float)(Auxiliary->Getbeta());   // side slip angle (radians)

  // Velocities
  net1->phidot = (float)(Auxiliary->GetEulerRates(ePhi));   // roll rate (radians/sec)
  net1->thetadot = (float)(Auxiliary->GetEulerRates(eTht)); // pitch rate (radians/sec)
  net1->psidot = (float)(Auxiliary->GetEulerRates(ePsi));   // yaw rate (radians/sec)
  net1->vcas = (float)(Auxiliary->GetVcalibratedKTS());     // VCAS, knots
  net1->climb_rate = (float)(Propagate->Gethdot());         // altitude rate, ft/sec
  net1->v_north = (float)(Propagate->GetVel(eNorth));       // north vel in NED frame, fps
  net1->v_east = (float)(Propagate->GetVel(eEast));         // east vel in NED frame, fps
  net1->v_down = (float)(Propagate->GetVel(eDown));         // down vel in NED frame, fps
                                                            //---ADD METHOD TO CALCULATE THESE TERMS---
  net1->v_body_u = (float)(Propagate->GetUVW(1));           // ECEF speed in body axis
  net1->v_body_v = (float)(Propagate->GetUVW(2));           // ECEF speed in body axis
  net1->v_body_w = (float)(Propagate->GetUVW(3));           // ECEF speed in body axis

  // Accelerations
  net1->A_X_pilot = (float)(Auxiliary->GetPilotAccel(1)); // X body accel, ft/s/s
  net1->A_Y_pilot = (float)(Auxiliary->GetPilotAccel(2)); // Y body accel, ft/s/s
  net1->A_Z_pilot = (float)(Auxiliary->GetPilotAccel(3)); // Z body accel, ft/s/s

  // Stall
  net1->stall_warning = 0.0;                               // 0.0 - 1.0 indicating the amount of stall
  net1->slip_deg = (float)(Auxiliary->Getbeta(inDegrees)); // slip ball deflection, deg

  net1->num_engines = min(FG_MAX_ENGINES, Propulsion->GetNumEngines()); // Number of valid engines

  for (i = 0; i < net1->num_engines; i++)
  {
    auto engine = Propulsion->GetEngine(i);
    if (engine->GetRunning())
      net1->eng_state[i] = 2; // Engine state running
    else if (engine->GetCranking())
      net1->eng_state[i] = 1; // Engine state cranking
    else
      net1->eng_state[i] = 0; // Engine state off

    switch (engine->GetType()) {
    case (FGEngine::etRocket):
      break;
    case (FGEngine::etPiston):
      {
        auto piston_engine = static_pointer_cast<FGPiston>(engine);
        net1->rpm[i] = (float)(piston_engine->getRPM());
        net1->fuel_flow[i] = (float)(piston_engine->getFuelFlow_gph());
        net1->fuel_px[i] = 0; // Fuel pressure, psi  (N/A in current model)
        net1->egt[i] = (float)(piston_engine->GetEGT());
        net1->cht[i] = (float)(piston_engine->getCylinderHeadTemp_degF());
        net1->mp_osi[i] = (float)(piston_engine->getManifoldPressure_inHg());
        net1->oil_temp[i] = (float)(piston_engine->getOilTemp_degF());
        net1->oil_px[i] = (float)(piston_engine->getOilPressure_psi());
        net1->tit[i] = 0; // Turbine Inlet Temperature  (N/A for piston)
      }
      break;
    case (FGEngine::etTurbine):
      break;
    case (FGEngine::etTurboprop):
      break;
    case (FGEngine::etElectric):
      net1->rpm[i] = static_cast<float>(static_pointer_cast<FGElectric>(engine)->getRPM());
      break;
    case (FGEngine::etUnknown):
      break;
    }
  }

  net1->num_tanks = min(FG_MAX_TANKS, Propulsion->GetNumTanks()); // Max number of fuel tanks

  for (i = 0; i < net1->num_tanks; i++)
    net1->fuel_quantity[i] = (float)(Propulsion->GetTank(i)->GetContents());

  net3->num_wheels = min(FG_MAX_WHEELS, GroundReactions->GetNumGearUnits());

  for (i = 0; i < net3->num_wheels; i++)
  {
    net3->wow[i] = GroundReactions->GetGearUnit(i)->GetWOW();
    if (GroundReactions->GetGearUnit(i)->GetGearUnitDown())
      net3->gear_pos[i] = 1; //gear down, using FCS convention
    else
      net3->gear_pos[i] = 0; //gear up, using FCS convention
    net3->gear_steer[i] = (float)(GroundReactions->GetGearUnit(i)->GetSteerNorm());
    net3->gear_compression[i] = (float)(GroundReactions->GetGearUnit(i)->GetCompLen());
  }

  // Environment
  if (outputOptions.useSimTime) {
    // Send simulation time with specified resolution
    net3->cur_time = static_cast<uint32_t>(FDMExec->GetSimTime() * outputOptions.timeFactor);
  } else {
    // Default to sending constant dummy value to ensure backwards-compatibility
    net3->cur_time = 1234567890u;
  }

  net3->warp = 0;             // offset in seconds to unix time
  net3->visibility = 25000.0; // visibility in meters (for env. effects)

  // Control surface positions (normalized values)
  net3->elevator = (float)(FCS->GetDePos(ofNorm));           // Norm Elevator Pos, --
  net3->elevator_trim_tab = (float)(FCS->GetPitchTrimCmd()); // Norm Elev Trim Tab Pos, --
  net3->left_flap = (float)(FCS->GetDfPos(ofNorm));          // Norm Flap Pos, --
  net3->right_flap = (float)(FCS->GetDfPos(ofNorm));         // Norm Flap Pos, --
  net3->left_aileron = (float)(FCS->GetDaLPos(ofNorm));      // Norm L Aileron Pos, --
  net3->right_aileron = (float)(FCS->GetDaRPos(ofNorm));     // Norm R Aileron Pos, --
  net3->rudder = (float)(FCS->GetDrPos(ofNorm));             // Norm Rudder Pos, --
  net3->nose_wheel = (float)(FCS->GetDrPos(ofNorm));         // *** FIX ***  Using Rudder Pos for NWS, --
  net3->speedbrake = (float)(FCS->GetDsbPos(ofNorm));        // Norm Speedbrake Pos, --
  net3->spoilers = (float)(FCS->GetDspPos(ofNorm));          // Norm Spoiler Pos, --

  // Convert the net buffer to network format
  if ( isLittleEndian ) {
    htond(net1->longitude);
    htond(net1->latitude);
    htond(net1->altitude);
    htonf(net1->agl);
    htonf(net1->phi);
    htonf(net1->theta);
    htonf(net1->psi);
    htonf(net1->alpha);
    htonf(net1->beta);

    htonf(net1->phidot);
    htonf(net1->thetadot);
    htonf(net1->psidot);
    htonf(net1->vcas);
    htonf(net1->climb_rate);
    htonf(net1->v_north);
    htonf(net1->v_east);
    htonf(net1->v_down);
    htonf(net1->v_body_u);
    htonf(net1->v_body_v);
    htonf(net1->v_body_w);

    htonf(net1->A_X_pilot);
    htonf(net1->A_Y_pilot);
    htonf(net1->A_Z_pilot);

    htonf(net1->stall_warning);
    htonf(net1->slip_deg);

    for (i = 0; i < net1->num_engines; ++i)
    {
      net1->eng_state[i] = htonl(net1->eng_state[i]);
      htonf(net1->rpm[i]);
      htonf(net1->fuel_flow[i]);
      htonf(net1->fuel_px[i]);
      htonf(net1->egt[i]);
      htonf(net1->cht[i]);
      htonf(net1->mp_osi[i]);
      htonf(net1->tit[i]);
      htonf(net1->oil_temp[i]);
      htonf(net1->oil_px[i]);
    }
    net1->num_engines = htonl(net1->num_engines);

    for (i = 0; i < net1->num_tanks; ++i)
    {
      htonf(net1->fuel_quantity[i]);
    }
    net1->num_tanks = htonl(net1->num_tanks);

    for (i = 0; i < net3->num_wheels; ++i)
    {
      net3->wow[i] = htonl(net3->wow[i]);
      htonf(net3->gear_pos[i]);
      htonf(net3->gear_steer[i]);
      htonf(net3->gear_compression[i]);
    }
    net3->num_wheels = htonl(net3->num_wheels);

    net3->cur_time = htonl(net3->cur_time);
    net3->warp = htonl(net3->warp);
    htonf(net3->visibility);

    htonf(net3->elevator);
    htonf(net3->elevator_trim_tab);
    htonf(net3->left_flap);
    htonf(net3->right_flap);
    htonf(net3->left_aileron);
    htonf(net3->right_aileron);
    htonf(net3->rudder);
    htonf(net3->nose_wheel);
    htonf(net3->speedbrake);
    htonf(net3->spoilers);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputFG::Print(void)
{
  if (socket == 0) return;
  if (!socket->GetConnectStatus()) return;

  SocketDataFill();
  socket->Send((char *)net1, dataLength);
}
}
