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

static const char *IdSrc = "$Id: FGFCS.cpp,v 1.77 2002/04/14 15:49:13 jberndt Exp $";
static const char *IdHdr = ID_FCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFCS::FGFCS(FGFDMExec* fdmex) : FGModel(fdmex)
{
  int i;
  Name = "FGFCS";

  DaCmd = DeCmd = DrCmd = DfCmd = DsbCmd = DspCmd = 0.0;
  PTrimCmd = YTrimCmd = RTrimCmd = 0.0;
  GearCmd = GearPos = 1; // default to gear down
  LeftBrake = RightBrake = CenterBrake = 0.0;
  DoNormalize=true;
  
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
  ThrottleCmd.clear();
  ThrottlePos.clear();
  MixtureCmd.clear();
  MixturePos.clear();
  PropAdvanceCmd.clear();
  PropAdvance.clear();

  unsigned int i;
  
  unbind();

  for (i=0;i<Components.size();i++) delete Components[i];
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
    for (i=0; i<Components.size(); i++)  Components[i]->Run();
    if (DoNormalize) Normalize();
  } else {
  }

  return false;
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
  string token;
  unsigned i;
  
  Name = Name + ":" + AC_cfg->GetValue("NAME");
  if (debug_lvl > 0) cout << "    Control System Name: " << Name << endl;
  if ( AC_cfg->GetValue("NORMALIZE") == "FALSE") {
      DoNormalize=false;
      cout << "    Automatic Control Surface Normalization Disabled" << endl;
  }    
  AC_cfg->GetNextConfigLine();
  while ((token = AC_cfg->GetValue()) != string("/FLIGHT_CONTROL")) {
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
        Components.push_back(new FGFilter(this, AC_cfg));
      } else if ((token == "PURE_GAIN") ||
                 (token == "SCHEDULED_GAIN") ||
                 (token == "AEROSURFACE_SCALE") ) {

        Components.push_back(new FGGain(this, AC_cfg));

      } else if (token == "SUMMER") {
        Components.push_back(new FGSummer(this, AC_cfg));
      } else if (token == "DEADBAND") {
        Components.push_back(new FGDeadBand(this, AC_cfg));
      } else if (token == "GRADIENT") {
        Components.push_back(new FGGradient(this, AC_cfg));
      } else if (token == "SWITCH") {
        Components.push_back(new FGSwitch(this, AC_cfg));
      } else if (token == "KINEMAT") {
        Components.push_back(new FGKinemat(this, AC_cfg));
      } else {
        cerr << "Unknown token [" << token << "] in FCS portion of config file" << endl;
        return false;
      }
      AC_cfg->GetNextConfigLine();
    }
  }
  //collect information for normalizing control surfaces
  string nodeName;
  for (i=0;i<Components.size();i++) {
    
    if ( (Components[i]->GetType() == "AEROSURFACE_SCALE" 
          || Components[i]->GetType() == "KINEMAT")  
                    && Components[i]->GetOutputNode() ) { 
      nodeName= Components[i]->GetOutputNode()->GetName();  
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
      } else if ( nodeName == "flaps-pos-deg" ) {
        ToNormalize[iDf]=i;
      }
    }
  }     
  
  bindModel();
  
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetComponentOutput(int idx) {
  return Components[idx]->GetOutput();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentName(int idx) {
  return Components[idx]->GetName();
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

  for (comp = 0; comp < Components.size(); comp++) {
    if (firstime) firstime = false;
    else          CompStrings += ", ";

    CompStrings += Components[comp]->GetName();
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

  for (comp = 0; comp < Components.size(); comp++) {
    if (firstime) firstime = false;
    else          CompValues += ", ";

    sprintf(buffer, "%9.6f", Components[comp]->GetOutput());
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
    DePos[ofNorm] = Components[ToNormalize[iDe]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDaL] > -1 ) {
    DaLPos[ofNorm] = Components[ToNormalize[iDaL]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDaR] > -1 ) {
    DaRPos[ofNorm] = Components[ToNormalize[iDaR]]->GetOutputPct();
  }

  if ( ToNormalize[iDr] > -1 ) {
    DrPos[ofNorm] = Components[ToNormalize[iDr]]->GetOutputPct();
  }
       
  if ( ToNormalize[iDsb] > -1 ) { 
    DsbPos[ofNorm] = Components[ToNormalize[iDsb]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDsp] > -1 ) {
    DspPos[ofNorm] = Components[ToNormalize[iDsp]]->GetOutputPct();
  }
  
  if ( ToNormalize[iDf] > -1 ) {
    DfPos[ofNorm] = Components[ToNormalize[iDf]]->GetOutputPct();
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
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::bindModel(void)
{
  unsigned i;

  for (i=0; i<ThrottleCmd.size(); i++) {
    PropertyManager->Tie("fcs/throttle-cmd-norm",this,i,
                          &FGFCS::GetThrottleCmd,
                          &FGFCS::SetThrottleCmd,
                          true );
    PropertyManager->Tie("fcs/throttle-pos-norm",this,i,
                          &FGFCS::GetThrottlePos,
                          &FGFCS::SetThrottlePos,
                          true );
    if ( MixtureCmd.size() > i ) {
      PropertyManager->Tie("fcs/mixture-cmd-norm",this,i,
                            &FGFCS::GetMixtureCmd,
                            &FGFCS::SetMixtureCmd,
                            true );
      PropertyManager->Tie("fcs/mixture-pos-norm",this,i,
                            &FGFCS::GetMixturePos,
                            &FGFCS::SetMixturePos,
                            true );
    }
    if ( PropAdvanceCmd.size() > i ) {
      PropertyManager->Tie("fcs/advance-cmd-norm",this,i,
                            &FGFCS::GetPropAdvanceCmd,
                            &FGFCS::SetPropAdvanceCmd,
                            true );
      PropertyManager->Tie("fcs/advance-pos-norm",this,i,
                            &FGFCS::GetPropAdvance,
                            &FGFCS::SetPropAdvance,
                            true );
    }
  }
}                            
                          
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::unbind(void)
{
  PropertyManager->Untie("fcs/aileron-cmd-norm");
  PropertyManager->Untie("fcs/elevator-cmd-norm");
  PropertyManager->Untie("fcs/rudder-cmd-norm");
  PropertyManager->Untie("fcs/flap-cmd-norm");
  PropertyManager->Untie("fcs/speedbrake-cmd-norm");
  PropertyManager->Untie("fcs/spoiler-cmd-norm");
  PropertyManager->Untie("fcs/pitch-trim-cmd-norm");
  PropertyManager->Untie("fcs/roll-trim-cmd-norm");
  PropertyManager->Untie("fcs/yaw-trim-cmd-norm");
  PropertyManager->Untie("gear/gear-cmd-norm");
  PropertyManager->Untie("fcs/left-aileron-pos-rad");
  PropertyManager->Untie("fcs/mag-left-aileron-pos-rad");
  PropertyManager->Untie("fcs/left-aileron-pos-norm");
  PropertyManager->Untie("fcs/right-aileron-pos-rad");
  PropertyManager->Untie("fcs/mag-right-aileron-pos-rad");
  PropertyManager->Untie("fcs/right-aileron-pos-norm");
  PropertyManager->Untie("fcs/elevator-pos-rad");
  PropertyManager->Untie("fcs/mag-elevator-pos-rad");
  PropertyManager->Untie("fcs/elevator-pos-norm");
  PropertyManager->Untie("fcs/rudder-pos-rad");
  PropertyManager->Untie("fcs/mag-rudder-pos-rad");
  PropertyManager->Untie("fcs/rudder-pos-norm");
  PropertyManager->Untie("fcs/flap-pos-deg");
  PropertyManager->Untie("fcs/flap-pos-norm");
  PropertyManager->Untie("fcs/speedbrake-pos-rad");
  PropertyManager->Untie("fcs/mag-speedbrake-pos-rad");
  PropertyManager->Untie("fcs/speedbrake-pos-norm");
  PropertyManager->Untie("fcs/spoiler-pos-rad");
  PropertyManager->Untie("fcs/mag-spoiler-pos-rad");
  PropertyManager->Untie("fcs/spoiler-pos-norm");
  PropertyManager->Untie("gear/gear-pos-norm");
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

