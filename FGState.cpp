/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                                                                       
 Module:       FGState.cpp
 Author:       Jon Berndt
 Date started: 11/17/98
 Called by:    FGFDMExec and accessed by all models.
 
 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------
 
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
See header file.
 
HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include <math.h>
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "FGState.h"

static const char *IdSrc = "$Id: FGState.cpp,v 1.97 2001/12/12 18:31:08 jberndt Exp $";
static const char *IdHdr = ID_STATE;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
MACROS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define RegisterVariable(ID,DEF) coeffdef[#ID] = ID; paramdef[ID] = DEF

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// For every term registered here there must be a corresponding handler in
// GetParameter() below that retrieves that parameter. Also, there must be an
// entry in the enum eParam definition in FGJSBBase.h. The ID is what must be used
// in any config file entry which references that item.

FGState::FGState(FGFDMExec* fdex)
{
  FDMExec = fdex;

  a = 1000.0;
  sim_time = 0.0;
  dt = 1.0/120.0;
  ActiveEngine = -1;

  Aircraft     = FDMExec->GetAircraft();
  Translation  = FDMExec->GetTranslation();
  Rotation     = FDMExec->GetRotation();
  Position     = FDMExec->GetPosition();
  FCS          = FDMExec->GetFCS();
  Output       = FDMExec->GetOutput();
  Atmosphere   = FDMExec->GetAtmosphere();
  Aerodynamics = FDMExec->GetAerodynamics();
  GroundReactions = FDMExec->GetGroundReactions();
  Propulsion      = FDMExec->GetPropulsion();

  RegisterVariable(FG_TIME,           " time "           );
  RegisterVariable(FG_QBAR,           " qbar "           );
  RegisterVariable(FG_WINGAREA,       " wing_area "      );
  RegisterVariable(FG_WINGSPAN,       " wingspan "       );
  RegisterVariable(FG_CBAR,           " cbar "           );
  RegisterVariable(FG_ALPHA,          " alpha "          );
  RegisterVariable(FG_ALPHADOT,       " alphadot "       );
  RegisterVariable(FG_BETA,           " beta "           );
  RegisterVariable(FG_ABETA,          " |beta| "         );
  RegisterVariable(FG_BETADOT,        " betadot "        );
  RegisterVariable(FG_PHI,            " roll_angle "     );
  RegisterVariable(FG_THT,            " pitch_angle "    );
  RegisterVariable(FG_PSI,            " heading_angle "  );
  RegisterVariable(FG_PITCHRATE,      " pitch_rate "     );
  RegisterVariable(FG_ROLLRATE,       " roll_rate "      );
  RegisterVariable(FG_YAWRATE,        " yaw_rate "       );
  RegisterVariable(FG_AEROQ,          " aero_pitch_rate ");
  RegisterVariable(FG_AEROP,          " aero_roll_rate " );
  RegisterVariable(FG_AEROR,          " aero_yaw_rate "  );
  RegisterVariable(FG_CL_SQRD,        " Clift_sqrd "     );
  RegisterVariable(FG_MACH,           " mach "           );
  RegisterVariable(FG_ALTITUDE,       " altitude "       );
  RegisterVariable(FG_BI2VEL,         " BI2Vel "         );
  RegisterVariable(FG_CI2VEL,         " CI2Vel "         );
  RegisterVariable(FG_ELEVATOR_POS,   " elevator_pos "   );
  RegisterVariable(FG_AILERON_POS,    " aileron_pos "    );
  RegisterVariable(FG_RUDDER_POS,     " rudder_pos "     );
  RegisterVariable(FG_SPDBRAKE_POS,   " speedbrake_pos " );
  RegisterVariable(FG_SPOILERS_POS,   " spoiler_pos "    );
  RegisterVariable(FG_FLAPS_POS,      " flaps_pos "      );
  RegisterVariable(FG_GEAR_POS,       " gear_pos "       );
  RegisterVariable(FG_ELEVATOR_CMD,   " elevator_cmd "   );
  RegisterVariable(FG_AILERON_CMD,    " aileron_cmd "    );
  RegisterVariable(FG_RUDDER_CMD,     " rudder_cmd "     );
  RegisterVariable(FG_SPDBRAKE_CMD,   " speedbrake_cmd " );
  RegisterVariable(FG_SPOILERS_CMD,   " spoiler_cmd "    );
  RegisterVariable(FG_FLAPS_CMD,      " flaps_cmd "      );
  RegisterVariable(FG_THROTTLE_CMD,   " throttle_cmd "   );
  RegisterVariable(FG_GEAR_CMD,       " gear_cmd "       );
  RegisterVariable(FG_THROTTLE_POS,   " throttle_pos "   );
  RegisterVariable(FG_MIXTURE_CMD,    " mixture_cmd "    );
  RegisterVariable(FG_MIXTURE_POS,    " mixture_pos "    );
  RegisterVariable(FG_MAGNETO_CMD,    " magneto_cmd "    );
  RegisterVariable(FG_STARTER_CMD,    " starter_cmd "    );
  RegisterVariable(FG_ACTIVE_ENGINE,  " active_engine "  );
  RegisterVariable(FG_HOVERB,         " height/span "    );
  RegisterVariable(FG_PITCH_TRIM_CMD, " pitch_trim_cmd " );
  RegisterVariable(FG_LEFT_BRAKE_CMD, " left_brake_cmd " );
  RegisterVariable(FG_RIGHT_BRAKE_CMD," right_brake_cmd ");
  RegisterVariable(FG_CENTER_BRAKE_CMD," center_brake_cmd ");
  RegisterVariable(FG_ALPHAH,          " h-tail alpha " );
  RegisterVariable(FG_ALPHAW,          " wing alpha " );
  RegisterVariable(FG_LBARH,           " h-tail arm " );
  RegisterVariable(FG_LBARV,           " v-tail arm " );
  RegisterVariable(FG_HTAILAREA,       " h-tail area " );
  RegisterVariable(FG_VTAILAREA,       " v-tail area " );
  RegisterVariable(FG_VBARH,           " h-tail volume " );
  RegisterVariable(FG_VBARV,           " v-tail volume " );
  RegisterVariable(FG_SET_LOGGING,    " data_logging "   );

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGState::~FGState()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGState::GetParameter(eParam val_idx) {
  double scratch;
  
  switch(val_idx) {
  case FG_TIME:
    return sim_time;
  case FG_QBAR:
    return Translation->Getqbar();
  case FG_WINGAREA:
    return Aircraft->GetWingArea();
  case FG_WINGSPAN:
    return Aircraft->GetWingSpan();
  case FG_CBAR:
    return Aircraft->Getcbar();
  case FG_LBARH:
    return Aircraft->Getlbarh();
  case FG_LBARV:
    return Aircraft->Getvbarh();
  case FG_HTAILAREA:
    return Aircraft->GetHTailArea();
  case FG_VTAILAREA:
    return Aircraft->GetVTailArea();
  case FG_VBARH:
    return Aircraft->Getvbarh();
  case FG_VBARV:
    return Aircraft->Getvbarv();
  case FG_ALPHA:
    return Translation->Getalpha();
  case FG_ALPHAW:
    return  Translation->Getalpha() + Aircraft->GetWingIncidence();
  case FG_ALPHADOT:
    return Translation->Getadot();
  case FG_BETA:
    return Translation->Getbeta();
  case FG_ABETA:
    return fabs(Translation->Getbeta());   
  case FG_BETADOT:
    return Translation->Getbdot();
  case FG_PHI:
    return Rotation->Getphi();
  case FG_THT:
    return Rotation->Gettht();
  case FG_PSI:
    return Rotation->Getpsi();
  case FG_PITCHRATE:
    return Rotation->GetPQR(eQ);
  case FG_ROLLRATE:
    return Rotation->GetPQR(eP);
  case FG_YAWRATE:
    return Rotation->GetPQR(eR);
  case FG_AEROP:
    return Rotation->GetAeroPQR(eP);
  case FG_AEROQ:
    return Rotation->GetAeroPQR(eQ);
  case FG_AEROR:
    return Rotation->GetAeroPQR(eR);
  case FG_CL_SQRD:
    if (Translation->Getqbar() > 0.00)
      scratch = Aerodynamics->GetvLastFs(eLift)/(Aircraft->GetWingArea()*Translation->Getqbar());
    else
      scratch = 0.0;
    return scratch*scratch;					   
  case FG_ELEVATOR_POS:
    return FCS->GetDePos();
  case FG_AILERON_POS:
    return FCS->GetDaPos();
  case FG_RUDDER_POS:
    return FCS->GetDrPos();
  case FG_SPDBRAKE_POS:
    return FCS->GetDsbPos();
  case FG_SPOILERS_POS:
    return FCS->GetDspPos();
  case FG_FLAPS_POS:
    return FCS->GetDfPos();
  case FG_ELEVATOR_CMD:
    return FCS->GetDeCmd();
  case FG_AILERON_CMD:
    return FCS->GetDaCmd();
  case FG_RUDDER_CMD:
    return FCS->GetDrCmd();
  case FG_SPDBRAKE_CMD:
    return FCS->GetDsbCmd();
  case FG_SPOILERS_CMD:
    return FCS->GetDspCmd();
  case FG_FLAPS_CMD:
    return FCS->GetDfCmd();
  case FG_MACH:
    return Translation->GetMach();
  case FG_ALTITUDE:
    return Position->Geth();
  case FG_BI2VEL:
    if(Translation->GetVt() > 0)
        return Aircraft->GetWingSpan()/(2.0 * Translation->GetVt());
    else
        return 0;
  case FG_CI2VEL:
    if(Translation->GetVt() > 0)
        return Aircraft->Getcbar()/(2.0 * Translation->GetVt());
    else
        return 0;
  case FG_THROTTLE_CMD:
    if (ActiveEngine < 0) return FCS->GetThrottleCmd(0);
    else return FCS->GetThrottleCmd(ActiveEngine);
  case FG_THROTTLE_POS:
    if (ActiveEngine < 0) return FCS->GetThrottlePos(0);
    else return FCS->GetThrottlePos(ActiveEngine);
  case FG_MAGNETO_CMD:
    if (ActiveEngine < 0) return Propulsion->GetEngine(0)->GetMagnetos();
    else return Propulsion->GetEngine(ActiveEngine)->GetMagnetos();
  case FG_STARTER_CMD:
    if (ActiveEngine < 0) {
      if (Propulsion->GetEngine(0)->GetStarter()) return 1.0;
      else return 0.0;
    } else {
      if (Propulsion->GetEngine(ActiveEngine)->GetStarter()) return 1.0;
      else return 0.0;
    }
  case FG_MIXTURE_CMD:
    if (ActiveEngine < 0) return FCS->GetMixtureCmd(0);
    else return FCS->GetMixtureCmd(ActiveEngine);
  case FG_MIXTURE_POS:
    if (ActiveEngine < 0) return FCS->GetMixturePos(0);
    else return FCS->GetMixturePos(ActiveEngine);
  case FG_HOVERB:
    return Position->GetHOverBMAC();
  case FG_PITCH_TRIM_CMD:
    return FCS->GetPitchTrimCmd();
  case FG_GEAR_CMD:
    return FCS->GetGearCmd();
  case FG_GEAR_POS:
    return FCS->GetGearPos();    
  default:
    cerr << "FGState::GetParameter() - No handler for parameter " << paramdef[val_idx] << endl;
    return 0.0;
  }
  return 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGState::GetParameter(string val_string) {
  return GetParameter(coeffdef[val_string]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

eParam FGState::GetParameterIndex(string val_string)
{
  return coeffdef[val_string];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::SetParameter(eParam val_idx, double val)
{
  int i;

  switch(val_idx) {
  case FG_ELEVATOR_POS:
    FCS->SetDePos(val);
    break;
  case FG_AILERON_POS:
    FCS->SetDaPos(val);
    break;
  case FG_RUDDER_POS:
    FCS->SetDrPos(val);
    break;
  case FG_SPDBRAKE_POS:
    FCS->SetDsbPos(val);
    break;
  case FG_SPOILERS_POS:
    FCS->SetDspPos(val);
    break;
  case FG_FLAPS_POS:
    FCS->SetDfPos(val);
    break;
  case FG_THROTTLE_POS:
    if (ActiveEngine == -1) {
      for (i=0; i<Propulsion->GetNumEngines(); i++) {
        FCS->SetThrottlePos(i,val);
	    }
	  } else {
      FCS->SetThrottlePos(ActiveEngine,val);
	  }
    break;
  case FG_MIXTURE_POS:
    if (ActiveEngine == -1) {
      for (i=0; i<Propulsion->GetNumEngines(); i++) {
        FCS->SetMixturePos(i,val);
	    }
	  } else {
      FCS->SetMixturePos(ActiveEngine,val);
	  }
    break;

  case FG_ELEVATOR_CMD:
    FCS->SetDeCmd(val);
    break;
  case FG_AILERON_CMD:
    FCS->SetDaCmd(val);
    break;
  case FG_RUDDER_CMD:
    FCS->SetDrCmd(val);
    break;
  case FG_SPDBRAKE_CMD:
    FCS->SetDsbCmd(val);
    break;
  case FG_SPOILERS_CMD:
    FCS->SetDspCmd(val);
    break;
  case FG_FLAPS_CMD:
    FCS->SetDfCmd(val);
    break;
  case FG_THROTTLE_CMD:
    if (ActiveEngine == -1) {
      for (i=0; i<Propulsion->GetNumEngines(); i++) {
        FCS->SetThrottleCmd(i,val);
	    }
	  } else {
      FCS->SetThrottleCmd(ActiveEngine,val);
	  }
    break;
  case FG_MIXTURE_CMD:
    if (ActiveEngine == -1) {
      for (i=0; i<Propulsion->GetNumEngines(); i++) {
        FCS->SetMixtureCmd(i,val);
	    }
	  } else {
      FCS->SetMixtureCmd(ActiveEngine,val);
	  }
    break;
  case FG_MAGNETO_CMD:
    if (ActiveEngine == -1) {
      for (i=0; i<Propulsion->GetNumEngines(); i++) {
        Propulsion->GetEngine(i)->SetMagnetos((int)val);
      }
    } else {
      Propulsion->GetEngine(ActiveEngine)->SetMagnetos((int)val);
    }
    break;
  case FG_STARTER_CMD:
    if (ActiveEngine == -1) {
      for (i=0; i<Propulsion->GetNumEngines(); i++) {
        if (val < 0.001) 
          Propulsion->GetEngine(i)->SetStarter(false);
        else if (val >=  0.001)
          Propulsion->GetEngine(i)->SetStarter(true);
      }
    } else {
      Propulsion->GetEngine(ActiveEngine)->SetStarter(true);
    }
    break;
  case FG_ACTIVE_ENGINE:
    ActiveEngine = (int)val;
    break;

  case FG_LEFT_BRAKE_CMD:
    FCS->SetLBrake(val);
    break;
  case FG_CENTER_BRAKE_CMD:
    FCS->SetCBrake(val);
    break;
  case FG_RIGHT_BRAKE_CMD:
    FCS->SetRBrake(val);
    break;
  case FG_GEAR_CMD:
    FCS->SetGearCmd(val);
    break;
  case  FG_GEAR_POS:
    FCS->SetGearPos(val);
    break; 
  case FG_SET_LOGGING:
    if      (val < -0.01) Output->Disable();
    else if (val >  0.01) Output->Enable();
    else                  Output->Toggle();
    break;

  default:
    cerr << "Parameter '" << val_idx << "' (" << paramdef[val_idx] << ") not handled" << endl;
  }
}

//***************************************************************************
//
// Reset: Assume all angles READ FROM FILE IN DEGREES !!
//

bool FGState::Reset(string path, string acname, string fname)
{
  string resetDef;
  string token="";

  double U, V, W;
  double phi, tht, psi;
  double latitude, longitude, h;
  double wdir, wmag, wnorth, weast;

# ifndef macintosh
  resetDef = path + "/" + acname + "/" + fname + ".xml";
# else
  resetDef = path + ";" + acname + ";" + fname + ".xml";
# endif

  FGConfigFile resetfile(resetDef);
  if (!resetfile.IsOpen()) return false;

  resetfile.GetNextConfigLine();
  token = resetfile.GetValue();
  if (token != string("initialize")) {
    cerr << "The reset file " << resetDef
         << " does not appear to be a reset file" << endl;
    return false;
  }
  
  resetfile.GetNextConfigLine();
  resetfile >> token;
  while (token != string("/initialize") && token != string("EOF")) {
    if (token == "UBODY") resetfile >> U;
    if (token == "VBODY") resetfile >> V;
    if (token == "WBODY") resetfile >> W;
    if (token == "LATITUDE") resetfile >> latitude;
    if (token == "LONGITUDE") resetfile >> longitude;
    if (token == "PHI") resetfile >> phi;
    if (token == "THETA") resetfile >> tht;
    if (token == "PSI") resetfile >> psi;
    if (token == "ALTITUDE") resetfile >> h;
    if (token == "WINDDIR") resetfile >> wdir;
    if (token == "VWIND") resetfile >> wmag;

    resetfile >> token;
  }
  
  
  Position->SetLatitude(latitude*degtorad);
  Position->SetLongitude(longitude*degtorad);
  Position->Seth(h);

  wnorth = wmag*ktstofps*cos(wdir*degtorad);
  weast = wmag*ktstofps*sin(wdir*degtorad);
  
  Initialize(U, V, W, phi*degtorad, tht*degtorad, psi*degtorad,
               latitude*degtorad, longitude*degtorad, h, wnorth, weast, 0.0);

  return true;
}

//***************************************************************************
//
// Initialize: Assume all angles GIVEN IN RADIANS !!
//

void FGState::Initialize(double U, double V, double W,
                         double phi, double tht, double psi,
                         double Latitude, double Longitude, double H,
                         double wnorth, double weast, double wdown)
{
  double alpha, beta;
  double qbar, Vt;
  FGColumnVector3 vAeroUVW;

  Position->SetLatitude(Latitude);
  Position->SetLongitude(Longitude);
  Position->Seth(H);

  Atmosphere->Run();
  
  vLocalEuler << phi << tht << psi;
  Rotation->SetEuler(vLocalEuler);

  InitMatrices(phi, tht, psi);
  
  vUVW << U << V << W;
  Translation->SetUVW(vUVW);
  
  Atmosphere->SetWindNED(wnorth, weast, wdown);
  
  vAeroUVW = vUVW + mTl2b*Atmosphere->GetWindNED();
  
  if (vAeroUVW(eW) != 0.0)
    alpha = vAeroUVW(eU)*vAeroUVW(eU) > 0.0 ? atan2(vAeroUVW(eW), vAeroUVW(eU)) : 0.0;
  else
    alpha = 0.0;
  if (vAeroUVW(eV) != 0.0)
    beta = vAeroUVW(eU)*vAeroUVW(eU)+vAeroUVW(eW)*vAeroUVW(eW) > 0.0 ? atan2(vAeroUVW(eV), (fabs(vAeroUVW(eU))/vAeroUVW(eU))*sqrt(vAeroUVW(eU)*vAeroUVW(eU) + vAeroUVW(eW)*vAeroUVW(eW))) : 0.0;
  else
    beta = 0.0;

  Translation->SetAB(alpha, beta);

  Vt = sqrt(U*U + V*V + W*W);
  Translation->SetVt(Vt);

  Translation->SetMach(Vt/Atmosphere->GetSoundSpeed());

  qbar = 0.5*(U*U + V*V + W*W)*Atmosphere->GetDensity();
  Translation->Setqbar(qbar);

  vLocalVelNED = mTb2l*vUVW;
  Position->SetvVel(vLocalVelNED);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::Initialize(FGInitialCondition *FGIC) {

  double tht,psi,phi;
  double U, V, W, h;
  double latitude, longitude;
  double wnorth,weast, wdown;
  
  latitude = FGIC->GetLatitudeRadIC();
  longitude = FGIC->GetLongitudeRadIC();
  h = FGIC->GetAltitudeFtIC();
  U = FGIC->GetUBodyFpsIC();
  V = FGIC->GetVBodyFpsIC();
  W = FGIC->GetWBodyFpsIC();
  tht = FGIC->GetThetaRadIC();
  phi = FGIC->GetPhiRadIC();
  psi = FGIC->GetPsiRadIC();
  wnorth = FGIC->GetWindNFpsIC();
  weast = FGIC->GetWindEFpsIC();
  wdown = FGIC->GetWindDFpsIC();
  
  Position->SetSeaLevelRadius( FGIC->GetSeaLevelRadiusFtIC() );
  Position->SetRunwayRadius( FGIC->GetSeaLevelRadiusFtIC() + 
                                             FGIC->GetTerrainAltitudeFtIC() );

  // need to fix the wind speed args, here.  
  Initialize(U, V, W, phi, tht, psi, latitude, longitude, h, wnorth, weast, wdown);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGState::StoreData(string fname) {
  ofstream datafile(fname.c_str());

  if (datafile) {
    datafile << Translation->GetUVW(eU);
    datafile << Translation->GetUVW(eV);
    datafile << Translation->GetUVW(eW);
    datafile << Position->GetLatitude();
    datafile << Position->GetLongitude();
    datafile << Rotation->GetEuler(ePhi);
    datafile << Rotation->GetEuler(eTht);
    datafile << Rotation->GetEuler(ePsi);
    datafile << Position->Geth();
    datafile.close();
    return true;
  } else {
    cerr << "Could not open dump file " << fname << endl;
    return false;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::InitMatrices(double phi, double tht, double psi) {
  double thtd2, psid2, phid2;
  double Sthtd2, Spsid2, Sphid2;
  double Cthtd2, Cpsid2, Cphid2;
  double Cphid2Cthtd2;
  double Cphid2Sthtd2;
  double Sphid2Sthtd2;
  double Sphid2Cthtd2;

  thtd2 = tht/2.0;
  psid2 = psi/2.0;
  phid2 = phi/2.0;

  Sthtd2 = sin(thtd2);
  Spsid2 = sin(psid2);
  Sphid2 = sin(phid2);

  Cthtd2 = cos(thtd2);
  Cpsid2 = cos(psid2);
  Cphid2 = cos(phid2);

  Cphid2Cthtd2 = Cphid2*Cthtd2;
  Cphid2Sthtd2 = Cphid2*Sthtd2;
  Sphid2Sthtd2 = Sphid2*Sthtd2;
  Sphid2Cthtd2 = Sphid2*Cthtd2;

  vQtrn(1) = Cphid2Cthtd2*Cpsid2 + Sphid2Sthtd2*Spsid2;
  vQtrn(2) = Sphid2Cthtd2*Cpsid2 - Cphid2Sthtd2*Spsid2;
  vQtrn(3) = Cphid2Sthtd2*Cpsid2 + Sphid2Cthtd2*Spsid2;
  vQtrn(4) = Cphid2Cthtd2*Spsid2 - Sphid2Sthtd2*Cpsid2;

  CalcMatrices();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::CalcMatrices(void) {
  double Q0Q0, Q1Q1, Q2Q2, Q3Q3;
  double Q0Q1, Q0Q2, Q0Q3, Q1Q2;
  double Q1Q3, Q2Q3;

  Q0Q0 = vQtrn(1)*vQtrn(1);
  Q1Q1 = vQtrn(2)*vQtrn(2);
  Q2Q2 = vQtrn(3)*vQtrn(3);
  Q3Q3 = vQtrn(4)*vQtrn(4);
  Q0Q1 = vQtrn(1)*vQtrn(2);
  Q0Q2 = vQtrn(1)*vQtrn(3);
  Q0Q3 = vQtrn(1)*vQtrn(4);
  Q1Q2 = vQtrn(2)*vQtrn(3);
  Q1Q3 = vQtrn(2)*vQtrn(4);
  Q2Q3 = vQtrn(3)*vQtrn(4);

  mTl2b(1,1) = Q0Q0 + Q1Q1 - Q2Q2 - Q3Q3;
  mTl2b(1,2) = 2*(Q1Q2 + Q0Q3);
  mTl2b(1,3) = 2*(Q1Q3 - Q0Q2);
  mTl2b(2,1) = 2*(Q1Q2 - Q0Q3);
  mTl2b(2,2) = Q0Q0 - Q1Q1 + Q2Q2 - Q3Q3;
  mTl2b(2,3) = 2*(Q2Q3 + Q0Q1);
  mTl2b(3,1) = 2*(Q1Q3 + Q0Q2);
  mTl2b(3,2) = 2*(Q2Q3 - Q0Q1);
  mTl2b(3,3) = Q0Q0 - Q1Q1 - Q2Q2 + Q3Q3;

  mTb2l = mTl2b;
  mTb2l.T();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::IntegrateQuat(FGColumnVector3 vPQR, int rate) {
  vQdot(1) = -0.5*(vQtrn(2)*vPQR(eP) + vQtrn(3)*vPQR(eQ) + vQtrn(4)*vPQR(eR));
  vQdot(2) =  0.5*(vQtrn(1)*vPQR(eP) + vQtrn(3)*vPQR(eR) - vQtrn(4)*vPQR(eQ));
  vQdot(3) =  0.5*(vQtrn(1)*vPQR(eQ) + vQtrn(4)*vPQR(eP) - vQtrn(2)*vPQR(eR));
  vQdot(4) =  0.5*(vQtrn(1)*vPQR(eR) + vQtrn(2)*vPQR(eQ) - vQtrn(3)*vPQR(eP));
  vQtrn += 0.5*dt*rate*(vlastQdot + vQdot);

  vQtrn.Normalize();

  vlastQdot = vQdot;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector3& FGState::CalcEuler(void) {
  if (mTl2b(3,3) == 0.0) mTl2b(3,3) = 0.0000001;
  if (mTl2b(1,1) == 0.0) mTl2b(1,1) = 0.0000001;

  vEuler(ePhi) = atan2(mTl2b(2,3), mTl2b(3,3));
  vEuler(eTht) = asin(-mTl2b(1,3));
  vEuler(ePsi) = atan2(mTl2b(1,2), mTl2b(1,1));

  if (vEuler(ePsi) < 0.0) vEuler(ePsi) += 2*M_PI;

  return vEuler;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGState::GetTs2b(void)
{
  double ca, cb, sa, sb;

  double alpha = Translation->Getalpha();
  double beta  = Translation->Getbeta();

  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTs2b(1,1) = ca*cb;
  mTs2b(1,2) = -ca*sb;
  mTs2b(1,3) = -sa;
  mTs2b(2,1) = sb;
  mTs2b(2,2) = cb;
  mTs2b(2,3) = 0.0;
  mTs2b(3,1) = sa*cb;
  mTs2b(3,2) = -sa*sb;
  mTs2b(3,3) = ca;

  return mTs2b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix33& FGState::GetTb2s(void)
{
  float alpha,beta;
  float ca, cb, sa, sb;
  
  alpha = Translation->Getalpha();
  beta  = Translation->Getbeta();
  
  ca = cos(alpha);
  sa = sin(alpha);
  cb = cos(beta);
  sb = sin(beta);

  mTb2s(1,1) = ca*cb;
  mTb2s(1,2) = sb;
  mTb2s(1,3) = sa*cb;
  mTb2s(2,1) = -ca*sb;
  mTb2s(2,2) = cb;
  mTb2s(2,3) = -sa*sb;
  mTb2s(3,1) = -sa;
  mTb2s(3,2) = 0.0;
  mTb2s(3,3) = ca;

  return mTb2s;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGState::ReportState(void) {
  char out[80], flap[10], gear[12];
  
  cout << endl << "  JSBSim State" << endl;
  snprintf(out,80,"    Weight: %7.0f lbs.  CG: %5.1f, %5.1f, %5.1f inches\n",
                   FDMExec->GetMassBalance()->GetWeight(),
                   FDMExec->GetMassBalance()->GetXYZcg(1),
                   FDMExec->GetMassBalance()->GetXYZcg(2),
                   FDMExec->GetMassBalance()->GetXYZcg(3));
  cout << out;             
  if( FCS->GetDfPos() <= 0.01)
    snprintf(flap,10,"Up");
  else
    snprintf(flap,10,"%2.0f",FCS->GetDfPos());
  if(FCS->GetGearPos() < 0.01)
    snprintf(gear,12,"Up");
  else if(FCS->GetGearPos() > 0.99)
    snprintf(gear,12,"Down");
  else
    snprintf(gear,12,"In Transit");   
  snprintf(out,80, "    Flaps: %3s  Gear: %12s\n",flap,gear);
  cout << out;
  snprintf(out,80, "    Speed: %4.0f KCAS  Mach: %5.2f\n",
                    FDMExec->GetAuxiliary()->GetVcalibratedKTS(),
                    GetParameter(FG_MACH) );
  cout << out;
  snprintf(out,80, "    Altitude: %7.0f ft.  AGL Altitude: %7.0f ft.\n",
                    Position->Geth(),
                    Position->GetDistanceAGL() );
  cout << out;
  snprintf(out,80, "    Angle of Attack: %6.2f deg  Pitch Angle: %6.2f deg\n",
                    GetParameter(FG_ALPHA)*radtodeg,
                    Rotation->Gettht()*radtodeg );
  cout << out;
  snprintf(out,80, "    Flight Path Angle: %6.2f deg  Climb Rate: %5.0f ft/min\n",
                    Position->GetGamma()*radtodeg,
                    Position->Gethdot()*60 );
  cout << out;                  
  snprintf(out,80, "    Normal Load Factor: %4.2f g's  Pitch Rate: %5.2f deg/s\n",
                    Aircraft->GetNlf(),
                    GetParameter(FG_PITCHRATE)*radtodeg );
  cout << out;
  snprintf(out,80, "    Heading: %3.0f deg true  Sideslip: %5.2f deg\n",
                    Rotation->Getpsi()*radtodeg,
                    GetParameter(FG_BETA)*radtodeg );                  
  cout << out;
  snprintf(out,80, "    Bank Angle: %5.2f deg\n",
                    Rotation->Getphi()*radtodeg );
  cout << out;
  snprintf(out,80, "    Elevator: %5.2f deg  Left Aileron: %5.2f deg  Rudder: %5.2f deg\n",
                    GetParameter(FG_ELEVATOR_POS)*radtodeg,
                    GetParameter(FG_AILERON_POS)*radtodeg,
                    GetParameter(FG_RUDDER_POS)*radtodeg );
  cout << out;                  
  snprintf(out,80, "    Throttle: %5.2f%c\n",
                    FCS->GetThrottlePos(0)*100,'%' );
  cout << out;
  
  snprintf(out,80, "    Wind Components: %5.2f kts head wind, %5.2f kts cross wind\n",
                    FDMExec->GetAuxiliary()->GetHeadWind()*fpstokts,
                    FDMExec->GetAuxiliary()->GetCrossWind()*fpstokts );
  cout << out; 
  
  snprintf(out,80, "    Ground Speed: %4.0f knots , Ground Track: %3.0f deg true\n",
                    Position->GetVground()*fpstokts,
                    Position->GetGroundTrack()*radtodeg );
  cout << out;                                   

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

void FGState::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGState" << endl;
    if (from == 1) cout << "Destroyed:    FGState" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
}

