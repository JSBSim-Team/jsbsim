/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Module:       FGFCS.cpp
 Author:       Jon Berndt
 Date started: 12/12/98
 Purpose:      Model the flight controls
 Called by:    FDMExec
 
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
This class models the flight controls for a specific airplane
 
HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCS.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGPropertyManager.h"

#include "filtersjb/FGFilter.h"
#include "filtersjb/FGDeadBand.h"
#include "filtersjb/FGGain.h"
#include "filtersjb/FGGradient.h"
#include "filtersjb/FGSwitch.h"
#include "filtersjb/FGSummer.h"
#include "filtersjb/FGKinemat.h"

static const char *IdSrc = "$Id: FGFCS.cpp,v 1.89 2002/09/29 13:17:10 apeden Exp $";
static const char *IdHdr = ID_FCS;

#if defined(WIN32) && !defined(__CYGWIN__)
#define snprintf _snprintf
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFCS::FGFCS(FGFDMExec* fdmex) : FGModel(fdmex)
{
  int i;
  Name = "FGFCS";

  DaCmd = DeCmd = DrCmd = DfCmd = DsbCmd = DspCmd = 0.0;
  AP_DaCmd = AP_DeCmd = AP_DrCmd = AP_ThrottleCmd = 0.0;
  PTrimCmd = YTrimCmd = RTrimCmd = 0.0;
  GearCmd = GearPos = 1; // default to gear down
  LeftBrake = RightBrake = CenterBrake = 0.0;
  DoNormalize=true;
  
  eMode = mNone;

  bind();
  for (i=0;i<=NForms;i++) {
    DePos[i] = DaLPos[i] = DaRPos[i] = DrPos[i] = 0.0;
    DfPos[i] = DsbPos[i] = DspPos[i] = 0.0;
  }
    
  for (i=0;i<NNorm;i++) { ToNormalize[i]=-1;}
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFCS::~FGFCS()
{
  unbind( PropertyManager->GetNode("fcs") );
  unbind( PropertyManager->GetNode("ap") );
  PropertyManager->Untie( "gear/gear-cmd-norm" );
  PropertyManager->Untie( "gear/gear-pos-norm" );

  ThrottleCmd.clear();
  ThrottlePos.clear();
  MixtureCmd.clear();
  MixturePos.clear();
  PropAdvanceCmd.clear();
  PropAdvance.clear();


  unsigned int i;

  for (i=0;i<APComponents.size();i++) delete APComponents[i];
  for (i=0;i<FCSComponents.size();i++) delete FCSComponents[i];
  
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::Run(void)
{
  unsigned int i;

  if (!FGModel::Run()) {
    for (i=0; i<ThrottlePos.size(); i++) ThrottlePos[i] = ThrottleCmd[i];
    for (i=0; i<MixturePos.size(); i++) MixturePos[i] = MixtureCmd[i];
    for (i=0; i<PropAdvance.size(); i++) PropAdvance[i] = PropAdvanceCmd[i];
    for (i=0; i<APComponents.size(); i++)  {
      eMode = mAP;
      APComponents[i]->Run();
      eMode = mNone;
    }
    for (i=0; i<FCSComponents.size(); i++)  {
      eMode = mFCS;
      FCSComponents[i]->Run();
      eMode = mNone;
    }
    if (DoNormalize) Normalize();

    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetThrottleCmd(int engineNum, double setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<ThrottleCmd.size();ctr++) ThrottleCmd[ctr] = setting;
    } else {
      ThrottleCmd[engineNum] = setting;
    }
  } else {
    cerr << "Throttle " << engineNum << " does not exist! " << ThrottleCmd.size()
         << " engines exist, but attempted throttle command is for engine "
         << engineNum << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetThrottlePos(int engineNum, double setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<ThrottlePos.size();ctr++) ThrottlePos[ctr] = setting;
    } else {
      ThrottlePos[engineNum] = setting;
    }
  } else {
    cerr << "Throttle " << engineNum << " does not exist! " << ThrottlePos.size()
         << " engines exist, but attempted throttle position setting is for engine "
         << engineNum << endl;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetThrottleCmd(int engineNum) const
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
       cerr << "Cannot get throttle value for ALL engines" << endl;
    } else {
      return ThrottleCmd[engineNum];
    }
  } else {
    cerr << "Throttle " << engineNum << " does not exist! " << ThrottleCmd.size()
         << " engines exist, but throttle setting for engine " << engineNum
         << " is selected" << endl;
  }
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetThrottlePos(int engineNum) const
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
       cerr << "Cannot get throttle value for ALL engines" << endl;
    } else {
      return ThrottlePos[engineNum];
    }
  } else {
    cerr << "Throttle " << engineNum << " does not exist! " << ThrottlePos.size()
         << " engines exist, but attempted throttle position setting is for engine "
         << engineNum << endl;
  }
  return 0.0; 
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetMixtureCmd(int engineNum, double setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<MixtureCmd.size();ctr++) MixtureCmd[ctr] = setting;
    } else {
      MixtureCmd[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetMixturePos(int engineNum, double setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<=MixtureCmd.size();ctr++) MixturePos[ctr] = MixtureCmd[ctr];
    } else {
      MixturePos[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetPropAdvanceCmd(int engineNum, double setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<PropAdvanceCmd.size();ctr++) PropAdvanceCmd[ctr] = setting;
    } else {
      PropAdvanceCmd[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetPropAdvance(int engineNum, double setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<=PropAdvanceCmd.size();ctr++) PropAdvance[ctr] = PropAdvanceCmd[ctr];
    } else {
      PropAdvance[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::Load(FGConfigFile* AC_cfg)
{
  string token, delimiter;
  string name, file, fname;
  unsigned i;
  vector <FGFCSComponent*> *Components;
  FGConfigFile *FCS_cfg;

  Components=0;
  // Determine if the FCS/Autopilot is defined inline in the aircraft configuration
  // file or in a separate file. Set up the config file class as appropriate.

  delimiter = AC_cfg->GetValue();
  name  = AC_cfg->GetValue("NAME");
  fname = AC_cfg->GetValue("FILE");

  if ( AC_cfg->GetValue("NORMALIZE") == "FALSE") {
    DoNormalize = false;
    cout << "    Automatic Control Surface Normalization Disabled" << endl;
  }

# ifndef macintosh
  file = "control/" + fname + ".xml";
# else
  file = "control;" + fname + ".xml";
# endif

  if (name.empty()) {
    name = fname;
    if (file.empty()) {
      cerr << "FCS/Autopilot does not appear to be defined inline nor in a file" << endl;
    } else {
      FCS_cfg = new FGConfigFile(file);
      if (!FCS_cfg->IsOpen()) {
        cerr << "Could not open " << delimiter << " file: " << file << endl;
        return false;
      } else {
        AC_cfg = FCS_cfg; // set local config file object pointer to FCS config
	                        // file object pointer
      }
    }
  } else {
    AC_cfg->GetNextConfigLine();
  }

  if (delimiter == "AUTOPILOT") {
    Components = &APComponents;
    eMode = mAP;
    Name = "Autopilot: " + name;
  } else if (delimiter == "FLIGHT_CONTROL") {
    Components = &FCSComponents;
    eMode = mFCS;
    Name = "FCS: " + name;
  } else {
    cerr << endl << "Unknown FCS delimiter" << endl << endl;
  }
  
  if (debug_lvl > 0) cout << "    Control System Name: " << Name << endl;

  while ((token = AC_cfg->GetValue()) != string("/" + delimiter)) {
    if (token == "COMPONENT") {
      token = AC_cfg->GetValue("TYPE");
      if (debug_lvl > 0) cout << endl << "    Loading Component \""
                              << AC_cfg->GetValue("NAME")
                              << "\" of type: " << token << endl;
      if ((token == "LAG_FILTER") ||
          (token == "LEAD_LAG_FILTER") ||
          (token == "SECOND_ORDER_FILTER") ||
          (token == "WASHOUT_FILTER") ||
          (token == "INTEGRATOR") ) {
        Components->push_back(new FGFilter(this, AC_cfg));
      } else if ((token == "PURE_GAIN") ||
                 (token == "SCHEDULED_GAIN") ||
                 (token == "AEROSURFACE_SCALE") ) {

        Components->push_back(new FGGain(this, AC_cfg));

      } else if (token == "SUMMER") {
        Components->push_back(new FGSummer(this, AC_cfg));
      } else if (token == "DEADBAND") {
        Components->push_back(new FGDeadBand(this, AC_cfg));
      } else if (token == "GRADIENT") {
        Components->push_back(new FGGradient(this, AC_cfg));
      } else if (token == "SWITCH") {
        Components->push_back(new FGSwitch(this, AC_cfg));
      } else if (token == "KINEMAT") {
        Components->push_back(new FGKinemat(this, AC_cfg));
      } else {
        cerr << "Unknown token [" << token << "] in FCS portion of config file" << endl;
        return false;
      }
      if (AC_cfg->GetNextConfigLine() == "EOF") break;
    }
  }

  //collect information for normalizing control surfaces

  string nodeName;
  for (i=0; i<Components->size(); i++) {
    
    if ( (((*Components)[i])->GetType() == "AEROSURFACE_SCALE" 
          || ((*Components)[i])->GetType() == "KINEMAT")  
                    && ((*Components)[i])->GetOutputNode() ) { 
      nodeName = ((*Components)[i])->GetOutputNode()->GetName();
      if ( nodeName == "elevator-pos-rad" ) {
        ToNormalize[iDe]=i;
      } else if ( nodeName  == "left-aileron-pos-rad" 
                   || nodeName == "aileron-pos-rad" ) {
        ToNormalize[iDaL]=i;
      } else if ( nodeName == "right-aileron-pos-rad" ) {
        ToNormalize[iDaR]=i;
      } else if ( nodeName == "rudder-pos-rad" ) {
        ToNormalize[iDr]=i;
      } else if ( nodeName == "speedbrake-pos-rad" ) {
        ToNormalize[iDsb]=i;
      } else if ( nodeName == "spoiler-pos-rad" ) {
        ToNormalize[iDsp]=i;
      } else if ( nodeName == "flap-pos-deg" ) {
        ToNormalize[iDf]=i;
      }
    }
  }     
  
  if (delimiter == "FLIGHT_CONTROL") bindModel();

  eMode = mNone;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetComponentOutput(int idx)
{
  switch (eMode) {
  case mFCS:
    return FCSComponents[idx]->GetOutput();
  case mAP:
    return APComponents[idx]->GetOutput();
  case mNone:
    cerr << "Unknown FCS mode" << endl;
    break;
  }
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentName(int idx)
{
  switch (eMode) {
  case mFCS:
    return FCSComponents[idx]->GetName();
  case mAP:
    return APComponents[idx]->GetName();
  case mNone:
    cerr << "Unknown FCS mode" << endl;
    break;
  }
  return string("");
} 

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetBrake(FGLGear::BrakeGroup bg)
{
  switch (bg) {
  case FGLGear::bgLeft:
    return LeftBrake;
  case FGLGear::bgRight:
    return RightBrake;
  case FGLGear::bgCenter:
    return CenterBrake;
  default:
    cerr << "GetBrake asked to return a bogus brake value" << endl;
  }
  return 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentStrings(void)
{
  unsigned int comp;
  string CompStrings = "";
  bool firstime = true;

  for (comp = 0; comp < FCSComponents.size(); comp++) {
    if (firstime) firstime = false;
    else          CompStrings += ", ";

    CompStrings += FCSComponents[comp]->GetName();
  }

  for (comp = 0; comp < APComponents.size(); comp++)
  {
    CompStrings += ", ";
    CompStrings += APComponents[comp]->GetName();
  }

  return CompStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentValues(void)
{
  unsigned int comp;
  string CompValues = "";
  char buffer[10];
  bool firstime = true;

  for (comp = 0; comp < FCSComponents.size(); comp++) {
    if (firstime) firstime = false;
    else          CompValues += ", ";

    sprintf(buffer, "%9.6f", FCSComponents[comp]->GetOutput());
    CompValues += string(buffer);
  }

  for (comp = 0; comp < APComponents.size(); comp++) {
    sprintf(buffer, ", %9.6f", APComponents[comp]->GetOutput());
    CompValues += string(buffer);
  }

  return CompValues;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::AddThrottle(void)
{
  ThrottleCmd.push_back(0.0);
  ThrottlePos.push_back(0.0);
  MixtureCmd.push_back(0.0);     // assume throttle and mixture are coupled
  MixturePos.push_back(0.0);
  PropAdvanceCmd.push_back(0.0); // assume throttle and prop pitch are coupled
  PropAdvance.push_back(0.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::Normalize(void) {
  
  //not all of these are guaranteed to be defined for every model
  //those that are have an index >=0 in the ToNormalize array
  //ToNormalize is filled in Load()
  
  if ( ToNormalize[iDe] > -1 ) {
    DePos[ofNorm] = FCSComponents[ToNormalize[iDe]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDaL] > -1 ) {
    DaLPos[ofNorm] = FCSComponents[ToNormalize[iDaL]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDaR] > -1 ) {
    DaRPos[ofNorm] = FCSComponents[ToNormalize[iDaR]]->GetOutputPct();
  }

  if ( ToNormalize[iDr] > -1 ) {
    DrPos[ofNorm] = FCSComponents[ToNormalize[iDr]]->GetOutputPct();
  }
       
  if ( ToNormalize[iDsb] > -1 ) { 
    DsbPos[ofNorm] = FCSComponents[ToNormalize[iDsb]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDsp] > -1 ) {
    DspPos[ofNorm] = FCSComponents[ToNormalize[iDsp]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDf] > -1 ) {
    DfPos[ofNorm] = FCSComponents[ToNormalize[iDf]]->GetOutputPct();
  }
  
  DePos[ofMag]  = fabs(DePos[ofRad]);
  DaLPos[ofMag] = fabs(DaLPos[ofRad]);
  DaRPos[ofMag] = fabs(DaRPos[ofRad]);
  DrPos[ofMag]  = fabs(DrPos[ofRad]);
  DsbPos[ofMag] = fabs(DsbPos[ofRad]);
  DspPos[ofMag] = fabs(DspPos[ofRad]);
  DfPos[ofMag]  = fabs(DfPos[ofRad]);
   
}  
    
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::bind(void)
{
  PropertyManager->Tie("fcs/aileron-cmd-norm", this,
                       &FGFCS::GetDaCmd,
                       &FGFCS::SetDaCmd,
                       true);
  PropertyManager->Tie("fcs/elevator-cmd-norm", this,
                       &FGFCS::GetDeCmd,
                       &FGFCS::SetDeCmd,
                       true);
  PropertyManager->Tie("fcs/rudder-cmd-norm", this,
                       &FGFCS::GetDrCmd,
                       &FGFCS::SetDrCmd,
                       true);
  PropertyManager->Tie("fcs/flap-cmd-norm", this,
                       &FGFCS::GetDfCmd,
                       &FGFCS::SetDfCmd,
                       true);
  PropertyManager->Tie("fcs/speedbrake-cmd-norm", this,
                       &FGFCS::GetDsbCmd,
                       &FGFCS::SetDsbCmd,
                       true);
  PropertyManager->Tie("fcs/spoiler-cmd-norm", this,
                       &FGFCS::GetDspCmd,
                       &FGFCS::SetDspCmd,
                       true);
  PropertyManager->Tie("fcs/pitch-trim-cmd-norm", this,
                       &FGFCS::GetPitchTrimCmd,
                       &FGFCS::SetPitchTrimCmd,
                       true);
  PropertyManager->Tie("fcs/roll-trim-cmd-norm", this,
                       &FGFCS::GetYawTrimCmd,
                       &FGFCS::SetYawTrimCmd,
                       true);
  PropertyManager->Tie("fcs/yaw-trim-cmd-norm", this,
                       &FGFCS::GetRollTrimCmd,
                       &FGFCS::SetRollTrimCmd,
                       true);
  PropertyManager->Tie("gear/gear-cmd-norm", this,
                       &FGFCS::GetGearCmd,
                       &FGFCS::SetGearCmd,
                       true);
  
  PropertyManager->Tie("fcs/left-aileron-pos-rad", this,ofRad,
                       &FGFCS::GetDaLPos,
                       &FGFCS::SetDaLPos,
                       true);
  PropertyManager->Tie("fcs/left-aileron-pos-norm", this,ofNorm,
                       &FGFCS::GetDaLPos,
                       &FGFCS::SetDaLPos,
                       true);
  PropertyManager->Tie("fcs/mag-left-aileron-pos-rad", this,ofMag,
                       &FGFCS::GetDaLPos,
                       &FGFCS::SetDaLPos,
                       true);
 
  PropertyManager->Tie("fcs/right-aileron-pos-rad", this,ofRad,
                       &FGFCS::GetDaRPos,
                       &FGFCS::SetDaRPos,
                       true);
  PropertyManager->Tie("fcs/right-aileron-pos-norm", this,ofNorm,
                       &FGFCS::GetDaRPos,
                       &FGFCS::SetDaRPos,
                       true);
  PropertyManager->Tie("fcs/mag-right-aileron-pos-rad", this,ofMag,
                       &FGFCS::GetDaRPos,
                       &FGFCS::SetDaRPos,
                       true);
  
  PropertyManager->Tie("fcs/elevator-pos-rad", this, ofRad,
                       &FGFCS::GetDePos,
                       &FGFCS::SetDePos,
                       true );
  PropertyManager->Tie("fcs/elevator-pos-norm", this,ofNorm,
                       &FGFCS::GetDePos,                       
                       &FGFCS::SetDePos,
                       true );
  PropertyManager->Tie("fcs/mag-elevator-pos-rad", this,ofMag,
                       &FGFCS::GetDePos,
                       &FGFCS::SetDePos,
                       true );
  
  PropertyManager->Tie("fcs/rudder-pos-rad", this,ofRad,
                       &FGFCS::GetDrPos,
                       &FGFCS::SetDrPos,
                       true);
  PropertyManager->Tie("fcs/rudder-pos-norm", this,ofNorm,
                       &FGFCS::GetDrPos,
                       &FGFCS::SetDrPos,
                       true);
  PropertyManager->Tie("fcs/mag-rudder-pos-rad", this,ofMag,
                       &FGFCS::GetDrPos,
                       &FGFCS::SetDrPos,
                       true);
                       
  PropertyManager->Tie("fcs/flap-pos-deg", this,ofRad,
                       &FGFCS::GetDfPos,
                       &FGFCS::SetDfPos,
                       true);
  PropertyManager->Tie("fcs/flap-pos-norm", this,ofNorm,
                       &FGFCS::GetDfPos,
                       &FGFCS::SetDfPos,
                       true);
  
  PropertyManager->Tie("fcs/speedbrake-pos-rad", this,ofRad,
                       &FGFCS::GetDsbPos,
                       &FGFCS::SetDsbPos,
                       true);
  PropertyManager->Tie("fcs/speedbrake-pos-norm", this,ofNorm,
                       &FGFCS::GetDsbPos,
                       &FGFCS::SetDsbPos,
                       true);
  PropertyManager->Tie("fcs/mag-speedbrake-pos-rad", this,ofMag,
                       &FGFCS::GetDsbPos,
                       &FGFCS::SetDsbPos,
                       true);
                       
  PropertyManager->Tie("fcs/spoiler-pos-rad", this,ofRad,
                       &FGFCS::GetDspPos,
                       &FGFCS::SetDspPos,
                       true);
  PropertyManager->Tie("fcs/spoiler-pos-norm", this,ofNorm,
                       &FGFCS::GetDspPos,
                       &FGFCS::SetDspPos,
                       true);
  PropertyManager->Tie("fcs/mag-spoiler-pos-rad", this,ofMag,
                       &FGFCS::GetDspPos,
                       &FGFCS::SetDspPos,
                       true);
                       
  PropertyManager->Tie("gear/gear-pos-norm", this,
                       &FGFCS::GetGearPos,
                       &FGFCS::SetGearPos,
                       true);

  PropertyManager->Tie("ap/elevator_cmd", this,
                       &FGFCS::GetAPDeCmd,
                       &FGFCS::SetAPDeCmd,
                       true);

  PropertyManager->Tie("ap/aileron_cmd", this,
                       &FGFCS::GetAPDaCmd,
                       &FGFCS::SetAPDaCmd,
                       true);

  PropertyManager->Tie("ap/rudder_cmd", this,
                       &FGFCS::GetAPDrCmd,
                       &FGFCS::SetAPDrCmd,
                       true);

  PropertyManager->Tie("ap/throttle_cmd", this,
                       &FGFCS::GetAPThrottleCmd,
                       &FGFCS::SetAPThrottleCmd,
                       true);

  PropertyManager->Tie("ap/attitude_setpoint", this,
                       &FGFCS::GetAPAttitudeSetPt,
                       &FGFCS::SetAPAttitudeSetPt,
                       true);

  PropertyManager->Tie("ap/altitude_setpoint", this,
                       &FGFCS::GetAPAltitudeSetPt,
                       &FGFCS::SetAPAltitudeSetPt,
                       true);

  PropertyManager->Tie("ap/heading_setpoint", this,
                       &FGFCS::GetAPHeadingSetPt,
                       &FGFCS::SetAPHeadingSetPt,
                       true);

  PropertyManager->Tie("ap/airspeed_setpoint", this,
                       &FGFCS::GetAPAirspeedSetPt,
                       &FGFCS::SetAPAirspeedSetPt,
                       true);

  PropertyManager->Tie("ap/acquire_attitude", this,
                       &FGFCS::GetAPAcquireAttitude,
                       &FGFCS::SetAPAcquireAttitude,
                       true);

  PropertyManager->Tie("ap/acquire_altitude", this,
                       &FGFCS::GetAPAcquireAltitude,
                       &FGFCS::SetAPAcquireAltitude,
                       true);

  PropertyManager->Tie("ap/acquire_heading", this,
                       &FGFCS::GetAPAcquireHeading,
                       &FGFCS::SetAPAcquireHeading,
                       true);

  PropertyManager->Tie("ap/acquire_airspeed", this,
                       &FGFCS::GetAPAcquireAirspeed,
                       &FGFCS::SetAPAcquireAirspeed,
                       true);

  PropertyManager->Tie("ap/attitude_hold", this,
                       &FGFCS::GetAPAttitudeHold,
                       &FGFCS::SetAPAttitudeHold,
                       true);

  PropertyManager->Tie("ap/altitude_hold", this,
                       &FGFCS::GetAPAltitudeHold,
                       &FGFCS::SetAPAltitudeHold,
                       true);

  PropertyManager->Tie("ap/heading_hold", this,
                       &FGFCS::GetAPHeadingHold,
                       &FGFCS::SetAPHeadingHold,
                       true);

  PropertyManager->Tie("ap/airspeed_hold", this,
                       &FGFCS::GetAPAirspeedHold,
                       &FGFCS::SetAPAirspeedHold,
                       true);

  PropertyManager->Tie("ap/wingslevel_hold", this,
                       &FGFCS::GetAPWingsLevelHold,
                       &FGFCS::SetAPWingsLevelHold,
                       true);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::bindModel(void)
{
  unsigned i;
  char tmp[80];
  
  
  for (i=0; i<ThrottleCmd.size(); i++) {
    snprintf(tmp,80,"fcs/throttle-cmd-norm[%u]",i);
    PropertyManager->Tie( tmp,this,i,
                          &FGFCS::GetThrottleCmd,
                          &FGFCS::SetThrottleCmd,
                          true );
    snprintf(tmp,80,"fcs/throttle-pos-norm[%u]",i);                      
    PropertyManager->Tie( tmp,this,i,
                          &FGFCS::GetThrottlePos,
                          &FGFCS::SetThrottlePos,
                          true );
    if ( MixtureCmd.size() > i ) {
      snprintf(tmp,80,"fcs/mixture-cmd-norm[%u]",i); 
      PropertyManager->Tie( tmp,this,i,
                            &FGFCS::GetMixtureCmd,
                            &FGFCS::SetMixtureCmd,
                            true );
      snprintf(tmp,80,"fcs/mixture-pos-norm[%u]",i);                    
      PropertyManager->Tie( tmp,this,i,
                            &FGFCS::GetMixturePos,
                            &FGFCS::SetMixturePos,
                            true );
    }
    if ( PropAdvanceCmd.size() > i ) {
      snprintf(tmp,80,"fcs/advance-cmd-norm[%u]",i); 
      PropertyManager->Tie( tmp,this,i,
                            &FGFCS::GetPropAdvanceCmd,
                            &FGFCS::SetPropAdvanceCmd,
                            true );
      snprintf(tmp,80,"fcs/advance-pos-norm[%u]",i);                       
      PropertyManager->Tie( tmp,this,i,
                            &FGFCS::GetPropAdvance,
                            &FGFCS::SetPropAdvance,
                            true );
    }
  }
}                            
                          
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::unbind(FGPropertyManager *node)
{
  int N = node->nChildren();
  for(int i=0;i<N;i++) {
    if(node->getChild(i)->nChildren() ) {
      unbind( (FGPropertyManager*)node->getChild(i) );
    } else if( node->getChild(i)->isTied() ) {
      node->getChild(i)->untie();
    } 
  }        
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

void FGFCS::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFCS" << endl;
    if (from == 1) cout << "Destroyed:    FGFCS" << endl;
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

