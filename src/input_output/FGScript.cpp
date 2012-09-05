/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGScript.cpp
 Author:       Jon S. Berndt
 Date started: 12/21/01
 Purpose:      Loads and runs JSBSim scripts.

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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

This class wraps up the simulation scripting routines.

HISTORY
--------------------------------------------------------------------------------
12/21/01   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <cstdlib>
#include <iomanip>

#include "FGScript.h"
#include "input_output/FGXMLElement.h"
#include "input_output/FGXMLParse.h"
#include "initialization/FGTrim.h"
#include "models/FGInput.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGScript.cpp,v 1.50 2012/09/05 04:49:13 jberndt Exp $";
static const char *IdHdr = ID_FGSCRIPT;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
GLOBAL DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// Constructor

FGScript::FGScript(FGFDMExec* fgex) : FDMExec(fgex)
{
  PropertyManager=FDMExec->GetPropertyManager();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGScript::~FGScript()
{
  unsigned int i, j;

  for (i=0; i<local_properties.size(); i++) {
    delete local_properties[i]->value;
    delete local_properties[i];
  }
  local_properties.clear();

  for (i=0; i<Events.size(); i++) {
    delete Events[i].Condition;
    for (j=0; j<Events[i].Functions.size(); j++)
      delete Events[i].Functions[j];
  }
  Events.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGScript::LoadScript(string script, double deltaT, const string initfile)
{
  string aircraft="", initialize="", comparison = "", prop_name="";
  string notifyPropertyName="";
  Element *element=0, *run_element=0, *event_element=0;
  Element *condition_element=0, *set_element=0, *delay_element=0;
  Element *notify_element = 0L, *notify_property_element = 0L;
  Element *property_element = 0L;
  Element *output_element = 0L;
  Element *input_element = 0L;
  bool result = false;
  double dt = 0.0, value = 0.0;
  struct event *newEvent;
  FGCondition *newCondition;

  document = LoadXMLDocument(script);

  if (!document) {
    cerr << "File: " << script << " could not be loaded." << endl;
    return false;
  }

  // Set up input and output files if specified
  
  output_element = document->FindElement("output");
  input_element = document->FindElement("input");

  if (document->GetName() != string("runscript")) {
    cerr << "File: " << script << " is not a script file" << endl;
    return false;
  }

  ScriptName = document->GetAttributeValue("name");

 // First, find "run" element and set delta T

  run_element = document->FindElement("run");

  if (!run_element) {
    cerr << "No \"run\" element found in script." << endl;
    return false;
  }

  // Set sim timing

  StartTime = run_element->GetAttributeValueAsNumber("start");
  FDMExec->Setsim_time(StartTime);
  EndTime   = run_element->GetAttributeValueAsNumber("end");
  // Make sure that the desired time is reached and executed.
  EndTime += 0.99*FDMExec->GetDeltaT();

  if (deltaT == 0.0)
    dt = run_element->GetAttributeValueAsNumber("dt");
  else {
    dt = deltaT;
    cout << endl << "Overriding simulation step size from the command line. New step size is: "
         << deltaT << " seconds (" << 1/deltaT << " Hz)" << endl << endl;
  }

  FDMExec->Setdt(dt);
  
  // read aircraft and initialization files

  element = document->FindElement("use");
  if (element) {
    aircraft = element->GetAttributeValue("aircraft");
    if (!aircraft.empty()) {
      result = FDMExec->LoadModel(aircraft);
      if (!result) return false;
    } else {
      cerr << "Aircraft must be specified in use element." << endl;
      return false;
    }

    initialize = element->GetAttributeValue("initialize");
    if (initfile.empty()) {
    if (initialize.empty()) {
      cerr << "Initialization file must be specified in use element." << endl;
      return false;
      }
    } else {
      cout << endl << "The initialization file specified in the script file (" << initialize
                   << ") has been overridden with a specified file (" << initfile << ")." << endl;
      initialize = initfile;
    }

  } else {
    cerr << "No \"use\" directives in the script file." << endl;
    return false;
  }

  // Now, read input spec if given.
  if (input_element > 0) {
    FDMExec->GetInput()->Load(input_element);
  }

  // Now, read output spec if given.
  if (output_element > 0) {
    string output_file = output_element->GetAttributeValue("file");
    if (output_file.empty()) {
      cerr << "No logging directives file was specified." << endl;
    } else {
      if (!FDMExec->SetOutputDirectives(output_file)) return false;
    }
  }

  // Read local property/value declarations
  property_element = run_element->FindElement("property");
  while (property_element) {

    double value=0.0;
    string title="";

    title = property_element->GetDataLine();
    if ( ! property_element->GetAttributeValue("value").empty())
      value = property_element->GetAttributeValueAsNumber("value");

    LocalProps *localProp = new LocalProps(value);
    localProp->title = title;
    local_properties.push_back(localProp);
    if (PropertyManager->HasNode(title)) {
      PropertyManager->GetNode(title)->setDoubleValue(value);
    } else {
      PropertyManager->Tie(localProp->title, localProp->value);
    }
    property_element = run_element->FindNextElement("property");
  }

  // Read "events" from script

  event_element = run_element->FindElement("event");
  while (event_element) { // event processing

    // Create the event structure
    newEvent = new struct event();

    // Retrieve the event name if given
    newEvent->Name = event_element->GetAttributeValue("name");

    // Is this event persistent? That is, does it execute every time the
    // condition triggers to true, or does it execute as a one-shot event, only?
    if (event_element->GetAttributeValue("persistent") == string("true")) {
      newEvent->Persistent = true;
    }

    // Does this event execute continuously when triggered to true?
    if (event_element->GetAttributeValue("continuous") == string("true")) {
      newEvent->Continuous = true;
    }

    // Process the conditions
    condition_element = event_element->FindElement("condition");
    if (condition_element != 0) {
      try {
        newCondition = new FGCondition(condition_element, PropertyManager);
      } catch(string str) {
        cout << endl << fgred << str << reset << endl << endl;
        delete newEvent;
        return false;
      }
      newEvent->Condition = newCondition;
    } else {
      cerr << "No condition specified in script event " << newEvent->Name << endl;
      delete newEvent;
      return false;
    }

    // Is there a delay between the time this event is triggered, and when the event
    // actions are executed?

    delay_element = event_element->FindElement("delay");
    if (delay_element) newEvent->Delay = event_element->FindElementValueAsNumber("delay");
    else newEvent->Delay = 0.0;

    // Notify about when this event is triggered?
    if ((notify_element = event_element->FindElement("notify")) != 0) {
      newEvent->Notify = true;
      // Check here for new <description> tag that gets echoed
      string notify_description = notify_element->FindElementValue("description");
      if (!notify_description.empty()) {
        newEvent->Description = notify_description;
      }
      notify_property_element = notify_element->FindElement("property");
      while (notify_property_element) {
        notifyPropertyName = notify_property_element->GetDataLine();
        if (PropertyManager->GetNode(notifyPropertyName)) {
          newEvent->NotifyProperties.push_back( PropertyManager->GetNode(notifyPropertyName) );
          string caption_attribute = notify_property_element->GetAttributeValue("caption");
          if (caption_attribute.empty()) {
            newEvent->DisplayString.push_back(notifyPropertyName);
          } else {
            newEvent->DisplayString.push_back(caption_attribute);
          }
        } else {
          cout << endl << fgred << "  Could not find the property named "
               << notifyPropertyName << " in script" << endl << "  \""
               << ScriptName << "\". Execution is aborted. Please recheck "
               << "your input files and scripts." << reset << endl;
          delete newEvent->Condition;
          delete newEvent;
          return false;
        }
        notify_property_element = notify_element->FindNextElement("property");
      }
    }

    // Read set definitions (these define the actions to be taken when the event is triggered).
    set_element = event_element->FindElement("set");
    while (set_element) {
      prop_name = set_element->GetAttributeValue("name");
      newEvent->SetParam.push_back( PropertyManager->GetNode(prop_name) );
      //Todo - should probably do some safety checking here to make sure one or the other
      //of value or function is specified.
      if (!set_element->GetAttributeValue("value").empty()) {
        value = set_element->GetAttributeValueAsNumber("value");
        newEvent->Functions.push_back((FGFunction*)0L);
      } else if (set_element->FindElement("function")) {
        value = 0.0;
        newEvent->Functions.push_back(new FGFunction(PropertyManager, set_element->FindElement("function")));
      }
      newEvent->SetValue.push_back(value);
      newEvent->OriginalValue.push_back(0.0);
      newEvent->newValue.push_back(0.0);
      newEvent->ValueSpan.push_back(0.0);
      string tempCompare = set_element->GetAttributeValue("type");
      if      (to_lower(tempCompare).find("delta") != string::npos) newEvent->Type.push_back(FG_DELTA);
      else if (to_lower(tempCompare).find("bool") != string::npos)  newEvent->Type.push_back(FG_BOOL);
      else if (to_lower(tempCompare).find("value") != string::npos) newEvent->Type.push_back(FG_VALUE);
      else                                newEvent->Type.push_back(FG_VALUE); // DEFAULT
      tempCompare = set_element->GetAttributeValue("action");
      if      (to_lower(tempCompare).find("ramp") != string::npos) newEvent->Action.push_back(FG_RAMP);
      else if (to_lower(tempCompare).find("step") != string::npos) newEvent->Action.push_back(FG_STEP);
      else if (to_lower(tempCompare).find("exp") != string::npos) newEvent->Action.push_back(FG_EXP);
      else                               newEvent->Action.push_back(FG_STEP); // DEFAULT

      if (!set_element->GetAttributeValue("tc").empty())
        newEvent->TC.push_back(set_element->GetAttributeValueAsNumber("tc"));
      else
        newEvent->TC.push_back(1.0); // DEFAULT

      newEvent->Transiting.push_back(false);

      set_element = event_element->FindNextElement("set");
    }
    Events.push_back(*newEvent);
    delete newEvent;

    event_element = run_element->FindNextElement("event");
  }

  Debug(4);

  FGInitialCondition *IC=FDMExec->GetIC();
  if ( ! IC->Load( initialize )) {
    cerr << "Initialization unsuccessful" << endl;
    exit(-1);
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGScript::RunScript(void)
{
  unsigned i, j;
  unsigned event_ctr = 0;

  double currentTime = FDMExec->GetSimTime();
  double newSetValue = 0;

  if (currentTime > EndTime) return false;

  // Iterate over all events.
  for (unsigned int ev_ctr=0; ev_ctr < Events.size(); ev_ctr++) {
    // Determine whether the set of conditional tests for this condition equate
    // to true and should cause the event to execute. If the conditions evaluate 
    // to true, then the event is triggered. If the event is not persistent,
    // then this trigger will remain set true. If the event is persistent,
    // the trigger will reset to false when the condition evaluates to false.
    if (Events[ev_ctr].Condition->Evaluate()) {
      if (!Events[ev_ctr].Triggered) {

        // The conditions are true, do the setting of the desired Event parameters
        for (i=0; i<Events[ev_ctr].SetValue.size(); i++) {
          Events[ev_ctr].OriginalValue[i] = Events[ev_ctr].SetParam[i]->getDoubleValue();
          if (Events[ev_ctr].Functions[i] != 0) { // Parameter should be set to a function value
            try {
              Events[ev_ctr].SetValue[i] = Events[ev_ctr].Functions[i]->GetValue();
            } catch (string msg) {
              std::cerr << std::endl << "A problem occurred in the execution of the script. " << msg << endl;
              throw;
            }
          }
          switch (Events[ev_ctr].Type[i]) {
          case FG_VALUE:
          case FG_BOOL:
            Events[ev_ctr].newValue[i] = Events[ev_ctr].SetValue[i];
            break;
          case FG_DELTA:
            Events[ev_ctr].newValue[i] = Events[ev_ctr].OriginalValue[i] + Events[ev_ctr].SetValue[i];
            break;
          default:
            cerr << "Invalid Type specified" << endl;
            break;
          }
          Events[ev_ctr].StartTime = currentTime + Events[ev_ctr].Delay;
          Events[ev_ctr].ValueSpan[i] = Events[ev_ctr].newValue[i] - Events[ev_ctr].OriginalValue[i];
          Events[ev_ctr].Transiting[i] = true;
        }
      }
      Events[ev_ctr].Triggered = true;

    } else if (Events[ev_ctr].Persistent) { // If the event is persistent, reset the trigger.
      Events[ev_ctr].Triggered = false; // Reset the trigger for persistent events
      Events[ev_ctr].Notified = false;  // Also reset the notification flag
    } else if (Events[ev_ctr].Continuous) { // If the event is continuous, reset the trigger.
      Events[ev_ctr].Triggered = false; // Reset the trigger for persistent events
      Events[ev_ctr].Notified = false;  // Also reset the notification flag
    }

    if ((currentTime >= Events[ev_ctr].StartTime) && Events[ev_ctr].Triggered) {

      for (i=0; i<Events[ev_ctr].SetValue.size(); i++) {
        if (Events[ev_ctr].Transiting[i]) {
          Events[ev_ctr].TimeSpan = currentTime - Events[ev_ctr].StartTime;
          switch (Events[ev_ctr].Action[i]) {
          case FG_RAMP:
            if (Events[ev_ctr].TimeSpan <= Events[ev_ctr].TC[i]) {
              newSetValue = Events[ev_ctr].TimeSpan/Events[ev_ctr].TC[i] * Events[ev_ctr].ValueSpan[i] + Events[ev_ctr].OriginalValue[i];
            } else {
              newSetValue = Events[ev_ctr].newValue[i];
              if (Events[ev_ctr].Continuous != true) Events[ev_ctr].Transiting[i] = false;
            }
            break;
          case FG_STEP:
            newSetValue = Events[ev_ctr].newValue[i];

            // If this is not a continuous event, reset the transiting flag.
            // Otherwise, it is known that the event is a continuous event.
            // Furthermore, if the event is to be determined by a function,
            // then the function will be continuously calculated.
            if (Events[ev_ctr].Continuous != true)
              Events[ev_ctr].Transiting[i] = false;
            else if (Events[ev_ctr].Functions[i] != 0)
              newSetValue = Events[ev_ctr].Functions[i]->GetValue();

            break;
          case FG_EXP:
            newSetValue = (1 - exp( -Events[ev_ctr].TimeSpan/Events[ev_ctr].TC[i] )) * Events[ev_ctr].ValueSpan[i] + Events[ev_ctr].OriginalValue[i];
            break;
          default:
            cerr << "Invalid Action specified" << endl;
            break;
          }
          Events[ev_ctr].SetParam[i]->setDoubleValue(newSetValue);
        }
      }

      // Print notification values after setting them
      if (Events[ev_ctr].Notify && !Events[ev_ctr].Notified) {
        cout << endl << "  Event " << event_ctr << " (" << Events[ev_ctr].Name << ")"
             << " executed at time: " << currentTime << endl;
        if (!Events[ev_ctr].Description.empty()) {
          cout << "    " << Events[ev_ctr].Description << endl;
        }
        for (j=0; j<Events[ev_ctr].NotifyProperties.size();j++) {
//          cout << "    " << Events[ev_ctr].NotifyProperties[j]->GetRelativeName()
          cout << "    " << Events[ev_ctr].DisplayString[j]
               << " = " << Events[ev_ctr].NotifyProperties[j]->getDoubleValue() << endl;
        }
        cout << endl;
        Events[ev_ctr].Notified = true;
      }

    }

    event_ctr++;
  }
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

void FGScript::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    } else if (from == 3) {
    } else if (from == 4)  { // print out script data
      cout << endl;
      cout << "Script: \"" << ScriptName << "\"" << endl;
      cout << "  begins at " << StartTime << " seconds and runs to " << EndTime
        << " seconds with dt = " << setprecision(6) << FDMExec->GetDeltaT() << " (" <<
        ceil(1.0/FDMExec->GetDeltaT()) << " Hz)" << endl;
      cout << endl;

      for (unsigned int i=0; i<local_properties.size(); i++) {
        cout << "Local property: " << local_properties[i]->title 
             << " = " << PropertyManager->GetNode(local_properties[i]->title)->getDoubleValue()
             << endl;
      }
      
      if (local_properties.size() > 0) cout << endl;

      for (unsigned i=0; i<Events.size(); i++) {
        cout << "Event " << i;
        if (!Events[i].Name.empty()) cout << " (" << Events[i].Name << ")";
        cout << ":" << endl;

        if (Events[i].Persistent)
          cout << "  " << "Whenever triggered, executes once";
        else if (Events[i].Continuous)
          cout << "  " << "While true, always executes";
        else
          cout << "  " << "When first triggered, executes once";

        Events[i].Condition->PrintCondition();

        cout << endl << "  Actions taken";
        if (Events[i].Delay > 0.0)
          cout << " (after a delay of " << Events[i].Delay << " secs)";
        cout << ":" << endl << "    {";
        for (unsigned j=0; j<Events[i].SetValue.size(); j++) {
          if (Events[i].SetValue[j] == 0.0 && Events[i].Functions[j] != 0L) {
            if (Events[i].SetParam[j] == 0) {
              cerr << fgred << highint << endl
                   << "  An attempt has been made to access a non-existent property" << endl
                   << "  in this event. Please check the property names used, spelling, etc."
                   << reset << endl;
              exit(-1);
            }
            cout << endl << "      set " << Events[i].SetParam[j]->GetRelativeName("/fdm/jsbsim/")
                 << " to function value";
          } else {
            if (Events[i].SetParam[j] == 0) {
              cerr << fgred << highint << endl
                   << "  An attempt has been made to access a non-existent property" << endl
                   << "  in this event. Please check the property names used, spelling, etc."
                   << reset << endl;
              exit(-1);
            }
            cout << endl << "      set " << Events[i].SetParam[j]->GetRelativeName("/fdm/jsbsim/")
                 << " to " << Events[i].SetValue[j];
          }

          switch (Events[i].Type[j]) {
          case FG_VALUE:
          case FG_BOOL:
            cout << " (constant";
            break;
          case FG_DELTA:
            cout << " (delta";
            break;
          default:
            cout << " (unspecified type";
          }

          switch (Events[i].Action[j]) {
          case FG_RAMP:
            cout << " via ramp";
            break;
          case FG_STEP:
            cout << " via step)";
            break;
          case FG_EXP:
            cout << " via exponential approach";
            break;
          default:
            cout << " via unspecified action)";
          }

          if (Events[i].Action[j] == FG_RAMP || Events[i].Action[j] == FG_EXP)
            cout << " with time constant " << Events[i].TC[j] << ")";
        }
        cout << endl << "    }" << endl;

        // Print notifications
        if (Events[i].Notify) {
          if (Events[i].NotifyProperties.size() > 0) {
            cout << "  Notifications" << ":" << endl << "    {" << endl;
            for (unsigned j=0; j<Events[i].NotifyProperties.size();j++) {
              cout << "      "
                   << Events[i].NotifyProperties[j]->GetRelativeName("/fdm/jsbsim/")
                   << endl;
            }
            cout << "    }" << endl;
          }
        }
        cout << endl;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGScript" << endl;
    if (from == 1) cout << "Destroyed:    FGScript" << endl;
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
