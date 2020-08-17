/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFCS.cpp 
 Author:       Jon Berndt
 Date started: 12/12/98
 Purpose:      Model the flight controls
 Called by:    FDMExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
This class models the flight controls for a specific airplane

HISTORY
--------------------------------------------------------------------------------
12/12/98   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iomanip>

#include "FGFCS.h"
#include "input_output/FGModelLoader.h"

#include "models/flight_control/FGFilter.h"
#include "models/flight_control/FGDeadBand.h"
#include "models/flight_control/FGGain.h"
#include "models/flight_control/FGPID.h"
#include "models/flight_control/FGSwitch.h"
#include "models/flight_control/FGSummer.h"
#include "models/flight_control/FGKinemat.h"
#include "models/flight_control/FGFCSFunction.h"
#include "models/flight_control/FGActuator.h"
#include "models/flight_control/FGAccelerometer.h"
#include "models/flight_control/FGMagnetometer.h"
#include "models/flight_control/FGGyro.h"
#include "models/flight_control/FGWaypoint.h"
#include "models/flight_control/FGAngles.h"
#include "models/flight_control/FGDistributor.h"
#include "models/flight_control/FGLinearActuator.h"

#include "FGFCSChannel.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFCS::FGFCS(FGFDMExec* fdm) : FGModel(fdm), ChannelRate(1)
{
  int i;
  Name = "FGFCS";
  systype = stFCS;

  fdmex = fdm;
  DaCmd = DeCmd = DrCmd = DfCmd = DsbCmd = DspCmd = 0;
  PTrimCmd = YTrimCmd = RTrimCmd = 0.0;
  GearCmd = GearPos = 1; // default to gear down
  BrakePos.resize(FGLGear::bgNumBrakeGroups);
  TailhookPos = WingFoldPos = 0.0; 

  bind();
  for (i=0;i<NForms;i++) {
    DePos[i] = DaLPos[i] = DaRPos[i] = DrPos[i] = 0.0;
    DfPos[i] = DsbPos[i] = DspPos[i] = 0.0;
  }

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
  PropFeatherCmd.clear();
  PropFeather.clear();

  unsigned int i;

  for (i=0;i<SystemChannels.size();i++) delete SystemChannels[i];
  SystemChannels.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::InitModel(void)
{
  if (!FGModel::InitModel()) return false;

  unsigned int i;

  for (i=0; i<ThrottlePos.size(); i++) ThrottlePos[i] = 0.0;
  for (i=0; i<MixturePos.size(); i++) MixturePos[i] = 0.0;
  for (i=0; i<ThrottleCmd.size(); i++) ThrottleCmd[i] = 0.0;
  for (i=0; i<MixtureCmd.size(); i++) MixtureCmd[i] = 0.0;
  for (i=0; i<PropAdvance.size(); i++) PropAdvance[i] = 0.0;
  for (i=0; i<PropFeather.size(); i++) PropFeather[i] = 0.0;

  DaCmd = DeCmd = DrCmd = DfCmd = DsbCmd = DspCmd = 0;
  PTrimCmd = YTrimCmd = RTrimCmd = 0.0;
  TailhookPos = WingFoldPos = 0.0;

  for (i=0;i<NForms;i++) {
    DePos[i] = DaLPos[i] = DaRPos[i] = DrPos[i] = 0.0;
    DfPos[i] = DsbPos[i] = DspPos[i] = 0.0;
  }

  // Reset the channels components.
  for (unsigned int i=0; i<SystemChannels.size(); i++) SystemChannels[i]->Reset();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Notes: In this logic the default engine commands are set. This is simply a
// sort of safe-mode method in case the user has not defined control laws for
// throttle, mixture, and prop-advance. The throttle, mixture, and prop advance
// positions are set equal to the respective commands. Any control logic that is
// actually present in the flight_control or autopilot section will override
// these simple assignments.

bool FGFCS::Run(bool Holding)
{
  unsigned int i;

  if (FGModel::Run(Holding)) return true; // fast exit if nothing to do
  if (Holding) return false;

  RunPreFunctions();

  for (i=0; i<ThrottlePos.size(); i++) ThrottlePos[i] = ThrottleCmd[i];
  for (i=0; i<MixturePos.size(); i++) MixturePos[i] = MixtureCmd[i];
  for (i=0; i<PropAdvance.size(); i++) PropAdvance[i] = PropAdvanceCmd[i];
  for (i=0; i<PropFeather.size(); i++) PropFeather[i] = PropFeatherCmd[i];

  // Execute system channels in order
  for (i=0; i<SystemChannels.size(); i++) {
    if (debug_lvl & 4) cout << "    Executing System Channel: " << SystemChannels[i]->GetName() << endl;
    ChannelRate = SystemChannels[i]->GetRate();
    SystemChannels[i]->Execute();
  }
  ChannelRate = 1;

  RunPostFunctions();

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetDaLPos( int form , double pos )
{
  switch(form) {
  case ofRad:
    DaLPos[ofRad] = pos;
    DaLPos[ofDeg] = pos*radtodeg;
    break;
  case ofDeg:
    DaLPos[ofRad] = pos*degtorad;
    DaLPos[ofDeg] = pos;
    break;
  case ofNorm:
    DaLPos[ofNorm] = pos;
  }
  DaLPos[ofMag] = fabs(DaLPos[ofRad]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetDaRPos( int form , double pos )
{
  switch(form) {
  case ofRad:
    DaRPos[ofRad] = pos;
    DaRPos[ofDeg] = pos*radtodeg;
    break;
  case ofDeg:
    DaRPos[ofRad] = pos*degtorad;
    DaRPos[ofDeg] = pos;
    break;
  case ofNorm:
    DaRPos[ofNorm] = pos;
  }
  DaRPos[ofMag] = fabs(DaRPos[ofRad]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetDePos( int form , double pos )
{
  switch(form) {
  case ofRad:
    DePos[ofRad] = pos;
    DePos[ofDeg] = pos*radtodeg;
    break;
  case ofDeg:
    DePos[ofRad] = pos*degtorad;
    DePos[ofDeg] = pos;
    break;
  case ofNorm:
    DePos[ofNorm] = pos;
  }
  DePos[ofMag] = fabs(DePos[ofRad]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetDrPos( int form , double pos )
{
  switch(form) {
  case ofRad:
    DrPos[ofRad] = pos;
    DrPos[ofDeg] = pos*radtodeg;
    break;
  case ofDeg:
    DrPos[ofRad] = pos*degtorad;
    DrPos[ofDeg] = pos;
    break;
  case ofNorm:
    DrPos[ofNorm] = pos;
  }
  DrPos[ofMag] = fabs(DrPos[ofRad]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetDfPos( int form , double pos )
{
  switch(form) {
  case ofRad:
    DfPos[ofRad] = pos;
    DfPos[ofDeg] = pos*radtodeg;
    break;
  case ofDeg:
    DfPos[ofRad] = pos*degtorad;
    DfPos[ofDeg] = pos;
    break;
  case ofNorm:
    DfPos[ofNorm] = pos;
  }
  DfPos[ofMag] = fabs(DfPos[ofRad]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetDsbPos( int form , double pos )
{
  switch(form) {
  case ofRad:
    DsbPos[ofRad] = pos;
    DsbPos[ofDeg] = pos*radtodeg;
    break;
  case ofDeg:
    DsbPos[ofRad] = pos*degtorad;
    DsbPos[ofDeg] = pos;
    break;
  case ofNorm:
    DsbPos[ofNorm] = pos;
  }
  DsbPos[ofMag] = fabs(DsbPos[ofRad]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetDspPos( int form , double pos )
{
  switch(form) {
  case ofRad:
    DspPos[ofRad] = pos;
    DspPos[ofDeg] = pos*radtodeg;
    break;
  case ofDeg:
    DspPos[ofRad] = pos*degtorad;
    DspPos[ofDeg] = pos;
    break;
  case ofNorm:
    DspPos[ofNorm] = pos;
  }
  DspPos[ofMag] = fabs(DspPos[ofRad]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetThrottleCmd(int engineNum, double setting)
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<ThrottleCmd.size(); ctr++)
        ThrottleCmd[ctr] = setting;
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
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<ThrottlePos.size(); ctr++)
        ThrottlePos[ctr] = setting;
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
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<MixtureCmd.size(); ctr++)
        MixtureCmd[ctr] = setting;
    } else {
      MixtureCmd[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetMixturePos(int engineNum, double setting)
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<MixtureCmd.size(); ctr++)
        MixturePos[ctr] = MixtureCmd[ctr];
    } else {
      MixturePos[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetPropAdvanceCmd(int engineNum, double setting)
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<PropAdvanceCmd.size(); ctr++)
        PropAdvanceCmd[ctr] = setting;
    } else {
      PropAdvanceCmd[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetPropAdvance(int engineNum, double setting)
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<PropAdvanceCmd.size(); ctr++)
        PropAdvance[ctr] = PropAdvanceCmd[ctr];
    } else {
      PropAdvance[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetFeatherCmd(int engineNum, bool setting)
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<PropFeatherCmd.size(); ctr++)
        PropFeatherCmd[ctr] = setting;
    } else {
      PropFeatherCmd[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetPropFeather(int engineNum, bool setting)
{
  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (unsigned int ctr=0; ctr<PropFeatherCmd.size(); ctr++)
        PropFeather[ctr] = PropFeatherCmd[ctr];
    } else {
      PropFeather[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::Load(Element* document)
{
  if (document->GetName() == "autopilot") {
    Name = "Autopilot: ";
    systype = stAutoPilot;
  } else if (document->GetName() == "flight_control") {
    Name = "FCS: ";
    systype = stFCS;
  } else if (document->GetName() == "system") {
    Name = "System: ";
    systype = stSystem;
  }

  // Load interface properties from document
  if (!FGModel::Upload(document, true))
    return false;

  Name += document->GetAttributeValue("name");

  Debug(2);

  Element* channel_element = document->FindElement("channel");
  
  while (channel_element) {
  
    FGFCSChannel* newChannel = 0;

    string sOnOffProperty = channel_element->GetAttributeValue("execute");
    string sChannelName = channel_element->GetAttributeValue("name");
    
    if (!channel_element->GetAttributeValue("execrate").empty())
      ChannelRate = channel_element->GetAttributeValueAsNumber("execrate");
    else
      ChannelRate = 1;

    if (sOnOffProperty.length() > 0) {
      FGPropertyNode* OnOffPropertyNode = PropertyManager->GetNode(sOnOffProperty);
      if (OnOffPropertyNode == 0) {
        cerr << channel_element->ReadFrom() << highint << fgred
             << "The On/Off property, " << sOnOffProperty << " specified for channel "
             << channel_element->GetAttributeValue("name") << " is undefined or not "
             << "understood. The simulation will abort" << reset << endl;
        throw("Bad system definition");
      } else
        newChannel = new FGFCSChannel(this, sChannelName, ChannelRate,
                                      OnOffPropertyNode);
    } else
      newChannel = new FGFCSChannel(this, sChannelName, ChannelRate);

    SystemChannels.push_back(newChannel);

    if (debug_lvl > 0)
      cout << endl << highint << fgblue << "    Channel " 
         << normint << channel_element->GetAttributeValue("name") << reset << endl;
  
    Element* component_element = channel_element->GetElement();
    while (component_element) {
      try {
        if ((component_element->GetName() == string("lag_filter")) ||
            (component_element->GetName() == string("lead_lag_filter")) ||
            (component_element->GetName() == string("washout_filter")) ||
            (component_element->GetName() == string("second_order_filter")) )
        {
          newChannel->Add(new FGFilter(this, component_element));
        } else if ((component_element->GetName() == string("pure_gain")) ||
                   (component_element->GetName() == string("scheduled_gain")) ||
                   (component_element->GetName() == string("aerosurface_scale")))
        {
          newChannel->Add(new FGGain(this, component_element));
        } else if (component_element->GetName() == string("summer")) {
          newChannel->Add(new FGSummer(this, component_element));
        } else if (component_element->GetName() == string("deadband")) {
          newChannel->Add(new FGDeadBand(this, component_element));
        } else if (component_element->GetName() == string("switch")) {
          newChannel->Add(new FGSwitch(this, component_element));
        } else if (component_element->GetName() == string("kinematic")) {
          newChannel->Add(new FGKinemat(this, component_element));
        } else if (component_element->GetName() == string("fcs_function")) {
          newChannel->Add(new FGFCSFunction(this, component_element));
        } else if (component_element->GetName() == string("pid")) {
          newChannel->Add(new FGPID(this, component_element));
        } else if (component_element->GetName() == string("integrator")) {
          // <integrator> is equivalent to <pid type="trap">
          Element* c1_el = component_element->FindElement("c1");
          if (!c1_el) {
            cerr << component_element->ReadFrom();
            throw("INTEGRATOR component " + component_element->GetAttributeValue("name")
                  + " does not provide the parameter <c1>");
          }
          c1_el->ChangeName("ki");
          if (!c1_el->HasAttribute("type"))
            c1_el->AddAttribute("type", "trap");
          newChannel->Add(new FGPID(this, component_element));
        } else if (component_element->GetName() == string("actuator")) {
          newChannel->Add(new FGActuator(this, component_element));
        } else if (component_element->GetName() == string("sensor")) {
          newChannel->Add(new FGSensor(this, component_element));
        } else if (component_element->GetName() == string("accelerometer")) {
          newChannel->Add(new FGAccelerometer(this, component_element));
        } else if (component_element->GetName() == string("magnetometer")) {
          newChannel->Add(new FGMagnetometer(this, component_element));
        } else if (component_element->GetName() == string("gyro")) {
          newChannel->Add(new FGGyro(this, component_element));
        } else if ((component_element->GetName() == string("waypoint_heading")) ||
                   (component_element->GetName() == string("waypoint_distance")))
        {
          newChannel->Add(new FGWaypoint(this, component_element));
        } else if (component_element->GetName() == string("angle")) {
          newChannel->Add(new FGAngles(this, component_element));
        } else if (component_element->GetName() == string("distributor")) {
          newChannel->Add(new FGDistributor(this, component_element));
        } else if (component_element->GetName() == string("linear_actuator")) {
          newChannel->Add(new FGLinearActuator(this, component_element));
        } else {
          cerr << "Unknown FCS component: " << component_element->GetName() << endl;
        }
      } catch(string& s) {
        cerr << highint << fgred << endl << "  " << s << endl;
        cerr << reset << endl;
        return false;
      }
      component_element = channel_element->GetNextElement();
    }
    channel_element = document->FindNextElement("channel");
  }

  PostLoad(document, FDMExec);

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetBrake(FGLGear::BrakeGroup bg)
{
  return BrakePos[bg];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

SGPath FGFCS::FindFullPathName(const SGPath& path) const
{
  SGPath name = FGModel::FindFullPathName(path);
  if (systype != stSystem || !name.isNull()) return name;

  name = CheckPathName(FDMExec->GetFullAircraftPath()/string("Systems"), path);
  if (!name.isNull()) return name;

  return CheckPathName(FDMExec->GetSystemsPath(), path);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentStrings(const string& delimiter) const
{
  string CompStrings = "";
  bool firstime = true;
  int total_count=0;

  for (unsigned int i=0; i<SystemChannels.size(); i++)
  {
    for (unsigned int c=0; c<SystemChannels[i]->GetNumComponents(); c++)
    {
      if (firstime) firstime = false;
      else          CompStrings += delimiter;

      CompStrings += SystemChannels[i]->GetComponent(c)->GetName();
      total_count++;
    }
  }

  return CompStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentValues(const string& delimiter) const
{
  std::ostringstream buf;

  bool firstime = true;
  int total_count=0;

  for (unsigned int i=0; i<SystemChannels.size(); i++)
  {
    for (unsigned int c=0; c<SystemChannels[i]->GetNumComponents(); c++)
    {
      if (firstime) firstime = false;
      else          buf << delimiter;

      buf << setprecision(9) << SystemChannels[i]->GetComponent(c)->GetOutput();
      total_count++;
    }
  }

  return buf.str();
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
  PropFeatherCmd.push_back(false);
  PropFeather.push_back(false);

  unsigned int num = (unsigned int)ThrottleCmd.size()-1;
  bindThrottle(num);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetDt(void) const
{
  return FDMExec->GetDeltaT()*rate;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::bind(void)
{
  PropertyManager->Tie("fcs/aileron-cmd-norm", this, &FGFCS::GetDaCmd, &FGFCS::SetDaCmd);
  PropertyManager->Tie("fcs/elevator-cmd-norm", this, &FGFCS::GetDeCmd, &FGFCS::SetDeCmd);
  PropertyManager->Tie("fcs/rudder-cmd-norm", this, &FGFCS::GetDrCmd, &FGFCS::SetDrCmd);
  PropertyManager->Tie("fcs/flap-cmd-norm", this, &FGFCS::GetDfCmd, &FGFCS::SetDfCmd);
  PropertyManager->Tie("fcs/speedbrake-cmd-norm", this, &FGFCS::GetDsbCmd, &FGFCS::SetDsbCmd);
  PropertyManager->Tie("fcs/spoiler-cmd-norm", this, &FGFCS::GetDspCmd, &FGFCS::SetDspCmd);
  PropertyManager->Tie("fcs/pitch-trim-cmd-norm", this, &FGFCS::GetPitchTrimCmd, &FGFCS::SetPitchTrimCmd);
  PropertyManager->Tie("fcs/roll-trim-cmd-norm", this, &FGFCS::GetRollTrimCmd, &FGFCS::SetRollTrimCmd);
  PropertyManager->Tie("fcs/yaw-trim-cmd-norm", this, &FGFCS::GetYawTrimCmd, &FGFCS::SetYawTrimCmd);

  PropertyManager->Tie("fcs/left-aileron-pos-rad", this, ofRad, &FGFCS::GetDaLPos, &FGFCS::SetDaLPos);
  PropertyManager->Tie("fcs/left-aileron-pos-deg", this, ofDeg, &FGFCS::GetDaLPos, &FGFCS::SetDaLPos);
  PropertyManager->Tie("fcs/left-aileron-pos-norm", this, ofNorm, &FGFCS::GetDaLPos, &FGFCS::SetDaLPos);
  PropertyManager->Tie("fcs/mag-left-aileron-pos-rad", this, ofMag, &FGFCS::GetDaLPos);

  PropertyManager->Tie("fcs/right-aileron-pos-rad", this, ofRad, &FGFCS::GetDaRPos, &FGFCS::SetDaRPos);
  PropertyManager->Tie("fcs/right-aileron-pos-deg", this, ofDeg, &FGFCS::GetDaRPos, &FGFCS::SetDaRPos);
  PropertyManager->Tie("fcs/right-aileron-pos-norm", this, ofNorm, &FGFCS::GetDaRPos, &FGFCS::SetDaRPos);
  PropertyManager->Tie("fcs/mag-right-aileron-pos-rad", this, ofMag, &FGFCS::GetDaRPos);

  PropertyManager->Tie("fcs/elevator-pos-rad", this, ofRad, &FGFCS::GetDePos, &FGFCS::SetDePos);
  PropertyManager->Tie("fcs/elevator-pos-deg", this, ofDeg, &FGFCS::GetDePos, &FGFCS::SetDePos);
  PropertyManager->Tie("fcs/elevator-pos-norm", this, ofNorm, &FGFCS::GetDePos, &FGFCS::SetDePos);
  PropertyManager->Tie("fcs/mag-elevator-pos-rad", this, ofMag, &FGFCS::GetDePos);

  PropertyManager->Tie("fcs/rudder-pos-rad", this,ofRad, &FGFCS::GetDrPos, &FGFCS::SetDrPos);
  PropertyManager->Tie("fcs/rudder-pos-deg", this,ofDeg, &FGFCS::GetDrPos, &FGFCS::SetDrPos);
  PropertyManager->Tie("fcs/rudder-pos-norm", this,ofNorm, &FGFCS::GetDrPos, &FGFCS::SetDrPos);
  PropertyManager->Tie("fcs/mag-rudder-pos-rad", this,ofMag, &FGFCS::GetDrPos);

  PropertyManager->Tie("fcs/flap-pos-rad", this,ofRad, &FGFCS::GetDfPos, &FGFCS::SetDfPos);
  PropertyManager->Tie("fcs/flap-pos-deg", this,ofDeg, &FGFCS::GetDfPos, &FGFCS::SetDfPos);
  PropertyManager->Tie("fcs/flap-pos-norm", this,ofNorm, &FGFCS::GetDfPos, &FGFCS::SetDfPos);

  PropertyManager->Tie("fcs/speedbrake-pos-rad", this,ofRad, &FGFCS::GetDsbPos, &FGFCS::SetDsbPos);
  PropertyManager->Tie("fcs/speedbrake-pos-deg", this,ofDeg, &FGFCS::GetDsbPos, &FGFCS::SetDsbPos);
  PropertyManager->Tie("fcs/speedbrake-pos-norm", this,ofNorm, &FGFCS::GetDsbPos, &FGFCS::SetDsbPos);
  PropertyManager->Tie("fcs/mag-speedbrake-pos-rad", this,ofMag, &FGFCS::GetDsbPos);

  PropertyManager->Tie("fcs/spoiler-pos-rad", this, ofRad, &FGFCS::GetDspPos, &FGFCS::SetDspPos);
  PropertyManager->Tie("fcs/spoiler-pos-deg", this, ofDeg, &FGFCS::GetDspPos, &FGFCS::SetDspPos);
  PropertyManager->Tie("fcs/spoiler-pos-norm", this, ofNorm, &FGFCS::GetDspPos, &FGFCS::SetDspPos);
  PropertyManager->Tie("fcs/mag-spoiler-pos-rad", this, ofMag, &FGFCS::GetDspPos);

  PropertyManager->Tie("gear/gear-pos-norm", this, &FGFCS::GetGearPos, &FGFCS::SetGearPos);
  PropertyManager->Tie("gear/gear-cmd-norm", this, &FGFCS::GetGearCmd, &FGFCS::SetGearCmd);
  PropertyManager->Tie("fcs/left-brake-cmd-norm", this, &FGFCS::GetLBrake, &FGFCS::SetLBrake);
  PropertyManager->Tie("fcs/right-brake-cmd-norm", this, &FGFCS::GetRBrake, &FGFCS::SetRBrake);
  PropertyManager->Tie("fcs/center-brake-cmd-norm", this, &FGFCS::GetCBrake, &FGFCS::SetCBrake);

  PropertyManager->Tie("gear/tailhook-pos-norm", this, &FGFCS::GetTailhookPos, &FGFCS::SetTailhookPos);
  PropertyManager->Tie("fcs/wing-fold-pos-norm", this, &FGFCS::GetWingFoldPos, &FGFCS::SetWingFoldPos);
  PropertyManager->Tie("simulation/channel-dt", this, &FGFCS::GetChannelDeltaT);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Technically, this function should probably bind propulsion type specific controls
// rather than mixture and prop-advance.

void FGFCS::bindThrottle(unsigned int num)
{
  string tmp;

  tmp = CreateIndexedPropertyName("fcs/throttle-cmd-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetThrottleCmd,
                                        &FGFCS::SetThrottleCmd);
  tmp = CreateIndexedPropertyName("fcs/throttle-pos-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetThrottlePos,
                                        &FGFCS::SetThrottlePos);
  tmp = CreateIndexedPropertyName("fcs/mixture-cmd-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetMixtureCmd,
                                        &FGFCS::SetMixtureCmd);
  tmp = CreateIndexedPropertyName("fcs/mixture-pos-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetMixturePos,
                                        &FGFCS::SetMixturePos);
  tmp = CreateIndexedPropertyName("fcs/advance-cmd-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetPropAdvanceCmd,
                                        &FGFCS::SetPropAdvanceCmd);
  tmp = CreateIndexedPropertyName("fcs/advance-pos-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetPropAdvance,
                                        &FGFCS::SetPropAdvance);
  tmp = CreateIndexedPropertyName("fcs/feather-cmd-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetFeatherCmd,
                                        &FGFCS::SetFeatherCmd);
  tmp = CreateIndexedPropertyName("fcs/feather-pos-norm", num);
  PropertyManager->Tie( tmp.c_str(), this, num, &FGFCS::GetPropFeather,
                                        &FGFCS::SetPropFeather);
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
    if (from == 2) { // Loader
      cout << endl << "  " << Name << endl;
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
    }
  }
}

}
