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
#include <FGFDMExec.h>
#include <input_output/FGPropertyManager.h>
#include <fstream>

#include <models/flight_control/FGFilter.h>
#include <models/flight_control/FGDeadBand.h>
#include <models/flight_control/FGGain.h>
#include <models/flight_control/FGGradient.h>
#include <models/flight_control/FGSwitch.h>
#include <models/flight_control/FGSummer.h>
#include <models/flight_control/FGKinemat.h>
#include <models/flight_control/FGFCSFunction.h>
#include <models/flight_control/FGSensor.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGFCS.cpp,v 1.7 2005/09/10 12:49:46 jberndt Exp $";
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
  PTrimCmd = YTrimCmd = RTrimCmd = 0.0;
  GearCmd = GearPos = 1; // default to gear down
  LeftBrake = RightBrake = CenterBrake = 0.0;

  bind();
  for (i=0;i<=NForms;i++) {
    DePos[i] = DaLPos[i] = DaRPos[i] = DrPos[i] = 0.0;
    DfPos[i] = DsbPos[i] = DspPos[i] = 0.0;
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFCS::~FGFCS()
{
  if (PropertyManager->HasNode("fcs")) unbind( PropertyManager->GetNode("fcs") );
  if (PropertyManager->HasNode("ap")) unbind( PropertyManager->GetNode("ap") );
  PropertyManager->Untie( "gear/gear-cmd-norm" );
  PropertyManager->Untie( "gear/gear-pos-norm" );

  ThrottleCmd.clear();
  ThrottlePos.clear();
  MixtureCmd.clear();
  MixturePos.clear();
  PropAdvanceCmd.clear();
  PropAdvance.clear();
  SteerPosDeg.clear();

  unsigned int i;

  for (i=0;i<APComponents.size();i++) delete APComponents[i];
  for (i=0;i<FCSComponents.size();i++) delete FCSComponents[i];
  for (i=0;i<sensors.size();i++) delete sensors[i];

  APComponents.clear();
  FCSComponents.clear();
  sensors.clear();
  interface_properties.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Notes: In this logic the default engine commands are set. This is simply a
// sort of safe-mode method in case the user has not defined control laws for
// throttle, mixture, and prop-advance. The throttle, mixture, and prop advance
// positions are set equal to the respective commands. Any control logic that is
// actually present in the flight_control or autopilot section will override
// these simple assignments.

bool FGFCS::Run(void)
{
  unsigned int i;

  if (FGModel::Run()) return true; // fast exit if nothing to do
  if (FDMExec->Holding()) return false;

  for (i=0; i<ThrottlePos.size(); i++) ThrottlePos[i] = ThrottleCmd[i];
  for (i=0; i<MixturePos.size(); i++) MixturePos[i] = MixtureCmd[i];
  for (i=0; i<PropAdvance.size(); i++) PropAdvance[i] = PropAdvanceCmd[i];

  // Set the default steering angle
  for (i=0; i<SteerPosDeg.size(); i++) {
    FGLGear* gear = GroundReactions->GetGearUnit(i);
    SteerPosDeg[i] = gear->GetDefaultSteerAngle( GetDsCmd() );
  }

  // Cycle through the sensor, autopilot, and flight control components
  for (i=0; i<sensors.size(); i++) sensors[i]->Run();
  for (i=0; i<APComponents.size(); i++) APComponents[i]->Run();
  for (i=0; i<FCSComponents.size(); i++) FCSComponents[i]->Run();

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

bool FGFCS::Load(Element* el)
{
  string name, file, fname, comp_name, interface_property_string;
  unsigned i;
  vector <FGFCSComponent*> *Components;
  Element *FCS_cfg, *document, *component_element, *property_element, *sensor_element;
  Element *channel_element;
  ifstream* controls_file = new ifstream();
  FGXMLParse controls_file_parser;

  Components=0;
  // Determine if the FCS/Autopilot is defined inline in the aircraft configuration
  // file or in a separate file. Set up the Element pointer as appropriate.

  string separator = "/";
#ifdef macintosh
  separator = ";";
#endif

  name = el->GetAttributeValue("name");

  if (name.empty()) {
    fname = el->GetAttributeValue("file");
    file = FDMExec->GetAircraftPath() + separator + FDMExec->GetModelName() + separator + fname + ".xml";
    if (fname.empty()) {
      cerr << "FCS/Autopilot does not appear to be defined inline nor in a file" << endl;
      return false;
    } else {
      controls_file->open(file.c_str());
      readXML(*controls_file, controls_file_parser);
      delete controls_file;
      document = controls_file_parser.GetDocument();
    }
  } else {
    document = el;
  }

  if (document->GetName() == "autopilot") {
    Components = &APComponents;
    Name = "Autopilot: " + document->GetAttributeValue("name");
  } else if (document->GetName() == "flight_control") {
    Components = &FCSComponents;
    Name = "FCS: " + document->GetAttributeValue("name");
  }

  Debug(2);

  // ToDo: How do these get untied?
  // ToDo: Consider having INPUT and OUTPUT interface properties. Would then
  //       have to duplicate this block of code after channel read code.
  //       Input properties could be write only (nah), and output could be read
  //       only.

  property_element = document->FindElement("property");
  while (property_element) {
    interface_properties.push_back(new double(0));
    interface_property_string = property_element->GetDataLine();
    PropertyManager->Tie(interface_property_string, interface_properties.back());
    property_element = document->FindNextElement("property");
  }

  sensor_element = document->FindElement("sensor");
  while (sensor_element) {
    try {
      sensors.push_back(new FGSensor(this, sensor_element));
    } catch (...) {
      throw("Unable to process sensor in config file");
    }
    sensor_element = document->FindNextElement("sensor");
  }

  channel_element = document->FindElement("channel");
  while (channel_element) {
    component_element = channel_element->FindElement("component");
    while (component_element) {
      comp_name = component_element->GetAttributeValue("type");
      try {
        if ((comp_name == "LAG_FILTER") ||
            (comp_name == "LEAD_LAG_FILTER") ||
            (comp_name == "SECOND_ORDER_FILTER") ||
            (comp_name == "WASHOUT_FILTER") ||
            (comp_name == "INTEGRATOR") )
        {
          Components->push_back(new FGFilter(this, component_element));
        } else if ((comp_name == "PURE_GAIN") ||
                   (comp_name == "SCHEDULED_GAIN") ||
                   (comp_name == "AEROSURFACE_SCALE") )
        {
          Components->push_back(new FGGain(this, component_element));
        } else if (comp_name == "SUMMER") {
          Components->push_back(new FGSummer(this, component_element));
        } else if (comp_name == "DEADBAND") {
          Components->push_back(new FGDeadBand(this, component_element));
        } else if (comp_name == "GRADIENT") {
          Components->push_back(new FGGradient(this, component_element));
        } else if (comp_name == "SWITCH") {
          Components->push_back(new FGSwitch(this, component_element));
        } else if (comp_name == "KINEMAT") {
          Components->push_back(new FGKinemat(this, component_element));
        } else if (comp_name == "FUNCTION") {
          Components->push_back(new FGFCSFunction(this, component_element));
        } else {
          cerr << "Unknown FCS component: " << comp_name << endl;
        }
      } catch(string s) {
        cerr << highint << fgred << endl << "  " << s << endl;
        cerr << reset << endl;
        return false;
      } catch (...) {
        return false;
      }
      component_element = channel_element->FindNextElement("component");
    }
    channel_element = document->FindNextElement("channel");
  }

  if (document->GetName() == "flight_control") bindModel();

  return true;
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

string FGFCS::GetComponentStrings(string delimeter)
{
  unsigned int comp;
  string CompStrings = "";
  bool firstime = true;

  for (comp = 0; comp < FCSComponents.size(); comp++) {
    if (firstime) firstime = false;
    else          CompStrings += delimeter;

    CompStrings += FCSComponents[comp]->GetName();
  }

  for (comp = 0; comp < APComponents.size(); comp++)
  {
    CompStrings += delimeter;
    CompStrings += APComponents[comp]->GetName();
  }

  return CompStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentValues(string delimeter)
{
  unsigned int comp;
  string CompValues = "";
  char buffer[12];
  bool firstime = true;

  for (comp = 0; comp < FCSComponents.size(); comp++) {
    if (firstime) firstime = false;
    else          CompValues += delimeter;

    sprintf(buffer, "%9.6f", FCSComponents[comp]->GetOutput());
    CompValues += string(buffer);
  }

  for (comp = 0; comp < APComponents.size(); comp++) {
    sprintf(buffer, "%s%9.6f", delimeter.c_str(), APComponents[comp]->GetOutput());
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

  unsigned int num = ThrottleCmd.size()-1;
  bindThrottle(num);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::AddGear(void)
{
  SteerPosDeg.push_back(0.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::bind(void)
{
  PropertyManager->Tie("fcs/aileron-cmd-norm", this, &FGFCS::GetDaCmd, &FGFCS::SetDaCmd);
  PropertyManager->Tie("fcs/elevator-cmd-norm", this, &FGFCS::GetDeCmd, &FGFCS::SetDeCmd);
  PropertyManager->Tie("fcs/rudder-cmd-norm", this, &FGFCS::GetDrCmd, &FGFCS::SetDrCmd);
  PropertyManager->Tie("fcs/steer-cmd-norm", this, &FGFCS::GetDsCmd, &FGFCS::SetDsCmd);
  PropertyManager->Tie("fcs/flap-cmd-norm", this, &FGFCS::GetDfCmd, &FGFCS::SetDfCmd);
  PropertyManager->Tie("fcs/speedbrake-cmd-norm", this, &FGFCS::GetDsbCmd, &FGFCS::SetDsbCmd);
  PropertyManager->Tie("fcs/spoiler-cmd-norm", this, &FGFCS::GetDspCmd, &FGFCS::SetDspCmd);
  PropertyManager->Tie("fcs/pitch-trim-cmd-norm", this, &FGFCS::GetPitchTrimCmd, &FGFCS::SetPitchTrimCmd);
  PropertyManager->Tie("fcs/roll-trim-cmd-norm", this, &FGFCS::GetRollTrimCmd, &FGFCS::SetRollTrimCmd);
  PropertyManager->Tie("fcs/yaw-trim-cmd-norm", this, &FGFCS::GetYawTrimCmd, &FGFCS::SetYawTrimCmd);
  PropertyManager->Tie("gear/gear-cmd-norm", this, &FGFCS::GetGearCmd, &FGFCS::SetGearCmd);

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
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Technically, this function should probably bind propulsion type specific controls
// rather than mixture and prop-advance.
//

void FGFCS::bindThrottle(unsigned int num)
{
  char tmp[80];

  snprintf(tmp, 80, "fcs/throttle-cmd-norm[%u]",num);
  PropertyManager->Tie( tmp, this, num, &FGFCS::GetThrottleCmd,
                                        &FGFCS::SetThrottleCmd);
  snprintf(tmp, 80, "fcs/throttle-pos-norm[%u]",num);
  PropertyManager->Tie( tmp, this, num, &FGFCS::GetThrottlePos,
                                        &FGFCS::SetThrottlePos);
  snprintf(tmp, 80, "fcs/mixture-cmd-norm[%u]",num);
  PropertyManager->Tie( tmp, this, num, &FGFCS::GetMixtureCmd,
                                        &FGFCS::SetMixtureCmd);
  snprintf(tmp, 80, "fcs/mixture-pos-norm[%u]",num);
  PropertyManager->Tie( tmp, this, num, &FGFCS::GetMixturePos,
                                        &FGFCS::SetMixturePos);
  snprintf(tmp, 80, "fcs/advance-cmd-norm[%u]",num);
  PropertyManager->Tie( tmp, this, num, &FGFCS::GetPropAdvanceCmd,
                                        &FGFCS::SetPropAdvanceCmd);
  snprintf(tmp, 80, "fcs/advance-pos-norm[%u]", num);
  PropertyManager->Tie( tmp, this, num, &FGFCS::GetPropAdvance,
                                        &FGFCS::SetPropAdvance);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::bindModel(void)
{
  unsigned int i;
  char tmp[80];

  for (i=0; i<SteerPosDeg.size(); i++) {
    if (GroundReactions->GetGearUnit(i)->GetSteerable()) {
      snprintf(tmp,80,"fcs/steer-pos-deg[%u]",i);
      PropertyManager->Tie( tmp, this, i, &FGFCS::GetSteerPosDeg, &FGFCS::SetSteerPosDeg);
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::unbind(FGPropertyManager *node)
{
  int N = node->nChildren();
  for (int i=0; i<N; i++) {
    if (node->getChild(i)->nChildren() ) {
      unbind( (FGPropertyManager*)node->getChild(i) );
    } else if ( node->getChild(i)->isTied() ) {
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
    if (from == 2) { // Loader
      cout << endl << "  Flight Control (" << Name << ")" << endl;
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

}
