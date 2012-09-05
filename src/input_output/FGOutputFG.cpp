/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGOutputFG.cpp
 Author:       Bertrand Coconnier
 Date started: 09/10/11
 Purpose:      Manage output of sim parameters to FlightGear
 Called by:    FGOutput

 ------------- Copyright (C) 2011 Bertrand Coconnier -------------

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
11/09/07   HDW   Added FlightGear Socket Interface
09/10/11   BC    Moved the FlightGear socket in a separate class

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstring>
#include <cstdlib>

#include "FGOutputFG.h"
#include "FGFDMExec.h"
#include "models/FGAerodynamics.h"
#include "models/FGAuxiliary.h"
#include "models/FGPropulsion.h"
#include "models/FGMassBalance.h"
#include "models/FGPropagate.h"
#include "models/FGGroundReactions.h"
#include "models/FGFCS.h"
#include "models/propulsion/FGPiston.h"
#include "models/propulsion/FGTank.h"

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <windows.h>
#else
#  include <netinet/in.h>       // htonl() ntohl()
#endif

static const int endianTest = 1;
#define isLittleEndian (*((char *) &endianTest ) != 0)

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGOutputFG.cpp,v 1.1 2012/09/05 21:49:19 bcoconni Exp $";
static const char *IdHdr = ID_OUTPUTFG;

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

FGOutputFG::FGOutputFG(FGFDMExec* fdmex, Element* element, int idx) :
  FGOutputSocket(fdmex, element, idx)
{
  memset(&fgSockBuf, 0x0, sizeof(fgSockBuf));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGOutputFG::FGOutputFG(FGFDMExec* fdmex, int idx, int subSystems,
                       std::string protocol, std::string Port, std::string name,
                       double outRate, std::vector<FGPropertyManager *> & outputProperties) :
  FGOutputSocket(fdmex, idx, subSystems, protocol, Port, name, outRate, outputProperties)
{
  memset(&fgSockBuf, 0x0, sizeof(fgSockBuf));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGOutputFG::SocketDataFill(FGNetFDM* net)
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
  net->cur_time    = (long int)1234567890;    // Friday, Feb 13, 2009, 23:31:30 UTC (not processed by FGFS anyway)
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

void FGOutputFG::Print(void)
{
  int length = sizeof(fgSockBuf);

  if (socket == 0) return;
  if (!socket->GetConnectStatus()) return;

  SocketDataFill(&fgSockBuf);
  socket->Send((char *)&fgSockBuf, length);
}
}
