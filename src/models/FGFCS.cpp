/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFCS.cpp 
 Author:       Jon Berndt
 Date started: 12/12/98
 Purpose:      Model the flight controls
 Called by:    FDMExec

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
#include <sstream>
#include <iomanip>

#include <models/flight_control/FGFilter.h>
#include <models/flight_control/FGDeadBand.h>
#include <models/flight_control/FGGain.h>
#include <models/flight_control/FGPID.h>
#include <models/flight_control/FGSwitch.h>
#include <models/flight_control/FGSummer.h>
#include <models/flight_control/FGKinemat.h>
#include <models/flight_control/FGFCSFunction.h>
#include <models/flight_control/FGSensor.h>
#include <models/flight_control/FGActuator.h>
#include <models/flight_control/FGAccelerometer.h>

namespace JSBSim {

static const char *IdSrc = "$Id: FGFCS.cpp,v 1.58 2009/06/09 03:23:55 jberndt Exp $";
static const char *IdHdr = ID_FCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGFCS::FGFCS(FGFDMExec* fdmex) : FGModel(fdmex)
{
  int i;
  Name = "FGFCS";

  DaCmd = DeCmd = DrCmd = DsCmd = DfCmd = DsbCmd = DspCmd = 0;
  PTrimCmd = YTrimCmd = RTrimCmd = 0.0;
  GearCmd = GearPos = 1; // default to gear down
  LeftBrake = RightBrake = CenterBrake = 0.0;
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
  SteerPosDeg.clear();
  PropFeatherCmd.clear();
  PropFeather.clear();

  unsigned int i;

  for (i=0;i<APComponents.size();i++) delete APComponents[i];
  APComponents.clear();
  for (i=0;i<FCSComponents.size();i++) delete FCSComponents[i];
  FCSComponents.clear();
  for (i=0;i<Systems.size();i++) delete Systems[i];
  Systems.clear();


  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::InitModel(void)
{
  unsigned int i;

  if (!FGModel::InitModel()) return false;

  for (i=0; i<ThrottlePos.size(); i++) ThrottlePos[i] = 0.0;
  for (i=0; i<MixturePos.size(); i++) MixturePos[i] = 0.0;
  for (i=0; i<ThrottleCmd.size(); i++) ThrottleCmd[i] = 0.0;
  for (i=0; i<MixtureCmd.size(); i++) MixtureCmd[i] = 0.0;
  for (i=0; i<PropAdvance.size(); i++) PropAdvance[i] = 0.0;
  for (i=0; i<PropFeather.size(); i++) PropFeather[i] = 0.0;

  DaCmd = DeCmd = DrCmd = DsCmd = DfCmd = DsbCmd = DspCmd = 0;
  PTrimCmd = YTrimCmd = RTrimCmd = 0.0;
  TailhookPos = WingFoldPos = 0.0;

  for (i=0;i<NForms;i++) {
    DePos[i] = DaLPos[i] = DaRPos[i] = DrPos[i] = 0.0;
    DfPos[i] = DsbPos[i] = DspPos[i] = 0.0;
  }

  for (unsigned int i=0; i<Systems.size(); i++) {
    if (Systems[i]->GetType() == "LAG" ||
        Systems[i]->GetType() == "LEAD_LAG" ||
        Systems[i]->GetType() == "WASHOUT" ||
        Systems[i]->GetType() == "SECOND_ORDER_FILTER" ||
        Systems[i]->GetType() == "INTEGRATOR")
    {
      ((FGFilter*)Systems[i])->ResetPastStates();
    } else if (Systems[i]->GetType() == "PID" ) {
      ((FGPID*)Systems[i])->ResetPastStates();
    }
  }

  for (unsigned int i=0; i<FCSComponents.size(); i++) {
    if (FCSComponents[i]->GetType() == "LAG" ||
        FCSComponents[i]->GetType() == "LEAD_LAG" ||
        FCSComponents[i]->GetType() == "WASHOUT" ||
        FCSComponents[i]->GetType() == "SECOND_ORDER_FILTER" ||
        FCSComponents[i]->GetType() == "INTEGRATOR")
    {
      ((FGFilter*)FCSComponents[i])->ResetPastStates();
    } else if (FCSComponents[i]->GetType() == "PID" ) {
      ((FGPID*)FCSComponents[i])->ResetPastStates();
    }
  }

  for (unsigned int i=0; i<APComponents.size(); i++) {
    if (APComponents[i]->GetType() == "LAG" ||
        APComponents[i]->GetType() == "LEAD_LAG" ||
        APComponents[i]->GetType() == "WASHOUT" ||
        APComponents[i]->GetType() == "SECOND_ORDER_FILTER" ||
        APComponents[i]->GetType() == "INTEGRATOR")
    {
      ((FGFilter*)APComponents[i])->ResetPastStates();
    } else if (APComponents[i]->GetType() == "PID" ) {
      ((FGPID*)APComponents[i])->ResetPastStates();
    }
  }

  return true;
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
  for (i=0; i<PropFeather.size(); i++) PropFeather[i] = PropFeatherCmd[i];

  // Set the default steering angle
  for (i=0; i<SteerPosDeg.size(); i++) {
    FGLGear* gear = GroundReactions->GetGearUnit(i);
    SteerPosDeg[i] = gear->GetDefaultSteerAngle( GetDsCmd() );
  }

  // Execute Systems in order
  for (i=0; i<Systems.size(); i++) Systems[i]->Run();

  // Execute Autopilot
  for (i=0; i<APComponents.size(); i++) APComponents[i]->Run();

  // Execute Flight Control System
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
      for (ctr=0;ctr<MixtureCmd.size();ctr++) MixturePos[ctr] = MixtureCmd[ctr];
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
      for (ctr=0;ctr<PropAdvanceCmd.size();ctr++) PropAdvance[ctr] = PropAdvanceCmd[ctr];
    } else {
      PropAdvance[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetFeatherCmd(int engineNum, bool setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<PropFeatherCmd.size();ctr++) PropFeatherCmd[ctr] = setting;
    } else {
      PropFeatherCmd[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCS::SetPropFeather(int engineNum, bool setting)
{
  unsigned int ctr;

  if (engineNum < (int)ThrottlePos.size()) {
    if (engineNum < 0) {
      for (ctr=0;ctr<PropFeatherCmd.size();ctr++) PropFeather[ctr] = PropFeatherCmd[ctr];
    } else {
      PropFeather[engineNum] = setting;
    }
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCS::Load(Element* el, SystemType systype)
{
  string name, file, fname="", interface_property_string, parent_name;
  vector <FGFCSComponent*> *Components;
  Element *component_element, *sensor_element;
  Element *channel_element;

  Components=0;

  string separator = "/";

// ToDo: The handling of name and file attributes could be improved, here,
//       considering that a name can be in the external file, as well.

  name = el->GetAttributeValue("name");

  if (name.empty()) {
    fname = el->GetAttributeValue("file");
    if (systype == stSystem) {
      file = FindSystemFullPathname(fname);
    } else { 
      file = FDMExec->GetFullAircraftPath() + separator + fname + ".xml";
    }
    if (fname.empty()) {
      cerr << "FCS, Autopilot, or system does not appear to be defined inline nor in a file" << endl;
      return false;
    } else {
      document = LoadXMLDocument(file);
      if (!document) {
        cerr << "Error loading file " << file << endl;
        return false;
      }
      name = document->GetAttributeValue("name");
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
  } else if (document->GetName() == "system") {
    Components = &Systems;
    Name = "System: " + document->GetAttributeValue("name");
  }
  Debug(2);

  if (document->GetName() == "flight_control") bindModel();

  FGModel::Load(document); // Load interface properties from document

  // After reading interface properties in a file, read properties in the local
  // flight_control, autopilot, or system element. This allows general-purpose
  // systems to be defined in a file, with overrides or initial loaded constants
  // supplied in the relevant element of the aircraft configuration file.

  Element* property_element = 0;

  if (!fname.empty()) {
    property_element = el->FindElement("property");
    if (property_element && debug_lvl > 0) cout << endl << "    Overriding properties" << endl << endl;
    while (property_element) {
      double value=0.0;
      if ( ! property_element->GetAttributeValue("value").empty())
        value = property_element->GetAttributeValueAsNumber("value");

      interface_property_string = property_element->GetDataLine();
      if (PropertyManager->HasNode(interface_property_string)) {
        FGPropertyManager* node = PropertyManager->GetNode(interface_property_string);
        if (debug_lvl > 0)
          cout << "      " << "Overriding value for property " << interface_property_string
               << " (old value: " << node->getDoubleValue() << "  new value: " << value << ")" << endl;
        node->setDoubleValue(value);
      } else {
        interface_properties.push_back(new double(value));
        PropertyManager->Tie(interface_property_string, interface_properties.back());
        if (debug_lvl > 0)
          cout << "      " << interface_property_string << " (initial value: " << value << ")" << endl;
      }
      
      property_element = el->FindNextElement("property");
    }
  }

  channel_element = document->FindElement("channel");
  while (channel_element) {
  
    if (debug_lvl > 0)
      cout << endl << highint << fgblue << "    Channel " 
         << normint << channel_element->GetAttributeValue("name") << reset << endl;
  
    component_element = channel_element->GetElement();
    while (component_element) {
      try {
        if ((component_element->GetName() == string("lag_filter")) ||
            (component_element->GetName() == string("lead_lag_filter")) ||
            (component_element->GetName() == string("washout_filter")) ||
            (component_element->GetName() == string("second_order_filter")) ||
            (component_element->GetName() == string("integrator")) )
        {
          Components->push_back(new FGFilter(this, component_element));
        } else if ((component_element->GetName() == string("pure_gain")) ||
                   (component_element->GetName() == string("scheduled_gain")) ||
                   (component_element->GetName() == string("aerosurface_scale")))
        {
          Components->push_back(new FGGain(this, component_element));
        } else if (component_element->GetName() == string("summer")) {
          Components->push_back(new FGSummer(this, component_element));
        } else if (component_element->GetName() == string("deadband")) {
          Components->push_back(new FGDeadBand(this, component_element));
        } else if (component_element->GetName() == string("switch")) {
          Components->push_back(new FGSwitch(this, component_element));
        } else if (component_element->GetName() == string("kinematic")) {
          Components->push_back(new FGKinemat(this, component_element));
        } else if (component_element->GetName() == string("fcs_function")) {
          Components->push_back(new FGFCSFunction(this, component_element));
        } else if (component_element->GetName() == string("pid")) {
          Components->push_back(new FGPID(this, component_element));
        } else if (component_element->GetName() == string("actuator")) {
          Components->push_back(new FGActuator(this, component_element));
        } else if (component_element->GetName() == string("sensor")) {
          Components->push_back(new FGSensor(this, component_element));
        } else if (component_element->GetName() == string("accelerometer")) {
          Components->push_back(new FGAccelerometer(this, component_element));
        } else {
          cerr << "Unknown FCS component: " << component_element->GetName() << endl;
        }
      } catch(string s) {
        cerr << highint << fgred << endl << "  " << s << endl;
        cerr << reset << endl;
        return false;
      }
      component_element = channel_element->GetNextElement();
    }
    channel_element = document->FindNextElement("channel");
  }

  ResetParser();

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

string FGFCS::FindSystemFullPathname(string system_filename)
{
  string fullpath, localpath;
  string systemPath = FDMExec->GetSystemsPath();
  string aircraftPath = FDMExec->GetFullAircraftPath();
  ifstream system_file;

  string separator = "/";

  fullpath = systemPath + separator;
  localpath = aircraftPath + separator + "Systems" + separator;

  system_file.open(string(fullpath + system_filename + ".xml").c_str());
  if ( !system_file.is_open()) {
    system_file.open(string(localpath + system_filename + ".xml").c_str());
      if ( !system_file.is_open()) {
        cerr << " Could not open system file: " << system_filename << " in path "
             << fullpath << " or " << localpath << endl;
        return string("");
      } else {
        return string(localpath + system_filename + ".xml");
      }
  }
  return string(fullpath + system_filename + ".xml");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

ifstream* FGFCS::FindSystemFile(string system_filename)
{
  string fullpath, localpath;
  string systemPath = FDMExec->GetSystemsPath();
  string aircraftPath = FDMExec->GetFullAircraftPath();
  ifstream* system_file = new ifstream();

  string separator = "/";

  fullpath = systemPath + separator;
  localpath = aircraftPath + separator + "Systems" + separator;

  system_file->open(string(fullpath + system_filename + ".xml").c_str());
  if ( !system_file->is_open()) {
    system_file->open(string(localpath + system_filename + ".xml").c_str());
      if ( !system_file->is_open()) {
        cerr << " Could not open system file: " << system_filename << " in path "
             << fullpath << " or " << localpath << endl;
      }
  }
  return system_file;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentStrings(string delimeter)
{
  unsigned int comp;
  string CompStrings = "";
  bool firstime = true;
  int total_count=0;

  for (unsigned int i=0; i<Systems.size(); i++) {
    if (firstime) firstime = false;
    else          CompStrings += delimeter;

    CompStrings += Systems[i]->GetName();
    total_count++;
  }

  for (comp = 0; comp < APComponents.size(); comp++)
  {
    if (firstime) firstime = false;
    else          CompStrings += delimeter;

    CompStrings += APComponents[comp]->GetName();
    total_count++;
  }

  for (comp = 0; comp < FCSComponents.size(); comp++) {
    if (firstime) firstime = false;
    else          CompStrings += delimeter;

    CompStrings += FCSComponents[comp]->GetName();
    total_count++;
  }

  return CompStrings;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGFCS::GetComponentValues(string delimeter)
{
  std::ostringstream buf;

  unsigned int comp;
  bool firstime = true;
  int total_count=0;

  for (unsigned int i=0; i<Systems.size(); i++) {
    if (firstime) firstime = false;
    else          buf << delimeter;

    buf << setprecision(9) << Systems[i]->GetOutput();
    total_count++;
  }

  for (comp = 0; comp < APComponents.size(); comp++) {
    if (firstime) firstime = false;
    else          buf << delimeter;

    buf << setprecision(9) << APComponents[comp]->GetOutput();
    total_count++;
  }

  for (comp = 0; comp < FCSComponents.size(); comp++) {
    if (firstime) firstime = false;
    else          buf << delimeter;

    buf << setprecision(9) << FCSComponents[comp]->GetOutput();
    total_count++;
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

void FGFCS::AddGear(void)
{
  SteerPosDeg.push_back(0.0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCS::GetDt(void)
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
  PropertyManager->Tie("fcs/steer-cmd-norm", this, &FGFCS::GetDsCmd, &FGFCS::SetDsCmd);

  PropertyManager->Tie("gear/tailhook-pos-norm", this, &FGFCS::GetTailhookPos, &FGFCS::SetTailhookPos);
  PropertyManager->Tie("fcs/wing-fold-pos-norm", this, &FGFCS::GetWingFoldPos, &FGFCS::SetWingFoldPos);
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

void FGFCS::bindModel(void)
{
  unsigned int i;
  string tmp;

  for (i=0; i<SteerPosDeg.size(); i++) {
    if (GroundReactions->GetGearUnit(i)->GetSteerable()) {
      tmp = CreateIndexedPropertyName("fcs/steer-pos-deg", i);
      PropertyManager->Tie( tmp.c_str(), this, i, &FGFCS::GetSteerPosDeg, &FGFCS::SetSteerPosDeg);
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
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}

}
