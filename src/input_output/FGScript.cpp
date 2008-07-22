/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGScript.cpp
 Author:       Jon S. Berndt
 Date started: 12/21/01
 Purpose:      Loads and runs JSBSim scripts.

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

#include "FGScript.h"
#include <input_output/FGXMLParse.h>
#include <initialization/FGTrim.h>

#include <iostream>
#include <iterator>

namespace JSBSim {

static const char *IdSrc = "$Id: FGScript.cpp,v 1.29 2008/07/22 02:42:17 jberndt Exp $";
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
  State = FDMExec->GetState();
  PropertyManager=FDMExec->GetPropertyManager();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGScript::~FGScript()
{
  unsigned int i;

  for (i=0; i<local_properties.size(); i++) delete local_properties[i];
  local_properties.clear();

  for (i=0; i<Events.size(); i++) delete Events[i].Condition;
  Events.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGScript::LoadScript( string script )
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
  State->Setsim_time(StartTime);
  EndTime   = run_element->GetAttributeValueAsNumber("end");
  dt        = run_element->GetAttributeValueAsNumber("dt");
  State->Setdt(dt);
  
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
    if (initialize.empty()) {
      cerr << "Initialization file must be specified in use element." << endl;
      return false;
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
      FDMExec->SetOutputDirectives(output_file);
    }
  }

  // Read local property/value declarations
  property_element = run_element->FindElement("property");
  while (property_element) {

    double value=0.0;
    if ( ! property_element->GetAttributeValue("value").empty())
      value = property_element->GetAttributeValueAsNumber("value");

    LocalProps *localProp = new LocalProps(value);
    localProp->title = property_element->GetDataLine();
    local_properties.push_back(localProp);

    PropertyManager->Tie(localProp->title, (local_properties.back())->value);
    property_element = run_element->FindNextElement("property");
  }

  // Read "events" from script

  event_element = run_element->FindElement("event");
  while (event_element) { // event processing

    // Create the event structure
    newEvent = new struct event();

    // Retrieve the event name if given
    newEvent->Name = event_element->GetAttributeValue("name");

    // Is this event persistent? That is, does it execute repeatedly as long as the
    // condition is true, or does it execute as a one-shot event, only?
    if (event_element->GetAttributeValue("persistent") == string("true")) {
      newEvent->Persistent = true;
    }

    // Process the conditions
    condition_element = event_element->FindElement("condition");
    if (condition_element != 0) {
      newCondition = new FGCondition(condition_element, PropertyManager);
      newEvent->Condition = newCondition;
    } else {
      cerr << "No condition specified in script event " << newEvent->Name << endl;
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
      notify_property_element = notify_element->FindElement("property");
      while (notify_property_element) {
        notifyPropertyName = notify_property_element->GetDataLine();
        if (PropertyManager->GetNode(notifyPropertyName)) {
          newEvent->NotifyProperties.push_back( PropertyManager->GetNode(notifyPropertyName) );
        } else {
          cout << endl << fgred << "  Could not find the property named "
               << notifyPropertyName << " in script" << endl << "  \""
               << ScriptName << "\". This unknown property will not be "
               << "echoed for notification." << reset << endl;
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
      if      (tempCompare == "FG_DELTA") newEvent->Type.push_back(FG_DELTA);
      else if (tempCompare == "FG_BOOL")  newEvent->Type.push_back(FG_BOOL);
      else if (tempCompare == "FG_VALUE") newEvent->Type.push_back(FG_VALUE);
      else                                newEvent->Type.push_back(FG_VALUE); // DEFAULT
      tempCompare = set_element->GetAttributeValue("action");
      if      (tempCompare == "FG_RAMP") newEvent->Action.push_back(FG_RAMP);
      else if (tempCompare == "FG_STEP") newEvent->Action.push_back(FG_STEP);
      else if (tempCompare == "FG_EXP")  newEvent->Action.push_back(FG_EXP);
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
  vector <struct event>::iterator iEvent = Events.begin();
  unsigned i, j;
  unsigned event_ctr = 0;

  double currentTime = State->Getsim_time();
  double newSetValue = 0;

  if (currentTime > EndTime) return false; //Script done!

  // Iterate over all events.
  while (iEvent < Events.end()) {
    iEvent->PrevTriggered = iEvent->Triggered;
    // Determine whether the set of conditional tests for this condition equate
    // to true and should cause the event to execute.
    if (iEvent->Condition->Evaluate()) {
      if (!iEvent->Triggered) {

        // The conditions are true, do the setting of the desired Event parameters
        for (i=0; i<iEvent->SetValue.size(); i++) {
          iEvent->OriginalValue[i] = iEvent->SetParam[i]->getDoubleValue();
          if (iEvent->Functions[i] != 0) { // Parameter should be set to a function value
            iEvent->SetValue[i] = iEvent->Functions[i]->GetValue();
          }
          switch (iEvent->Type[i]) {
          case FG_VALUE:
          case FG_BOOL:
            iEvent->newValue[i] = iEvent->SetValue[i];
            break;
          case FG_DELTA:
            iEvent->newValue[i] = iEvent->OriginalValue[i] + iEvent->SetValue[i];
            break;
          default:
            cerr << "Invalid Type specified" << endl;
            break;
          }
          iEvent->StartTime = currentTime + iEvent->Delay;
          iEvent->ValueSpan[i] = iEvent->newValue[i] - iEvent->OriginalValue[i];
          iEvent->Transiting[i] = true;
        }
      }
      iEvent->Triggered = true;
    } else if (iEvent->Persistent) {
      iEvent->Triggered = false; // Reset the trigger for persistent events
      iEvent->Notified = false;  // Also reset the notification flag
    }

    if ((currentTime >= iEvent->StartTime) && iEvent->Triggered) {

      for (i=0; i<iEvent->SetValue.size(); i++) {
        if (iEvent->Transiting[i]) {
          iEvent->TimeSpan = currentTime - iEvent->StartTime;
          if (iEvent->Functions[i] == 0) {
            switch (iEvent->Action[i]) {
            case FG_RAMP:
              if (iEvent->TimeSpan <= iEvent->TC[i]) {
                newSetValue = iEvent->TimeSpan/iEvent->TC[i] * iEvent->ValueSpan[i] + iEvent->OriginalValue[i];
              } else {
                newSetValue = iEvent->newValue[i];
                iEvent->Transiting[i] = false;
              }
              break;
            case FG_STEP:
              newSetValue = iEvent->newValue[i];
              iEvent->Transiting[i] = false;
              break;
            case FG_EXP:
              newSetValue = (1 - exp( -iEvent->TimeSpan/iEvent->TC[i] )) * iEvent->ValueSpan[i] + iEvent->OriginalValue[i];
              break;
            default:
              cerr << "Invalid Action specified" << endl;
              break;
            }
          } else { // Set the new value based on a function
            newSetValue = iEvent->Functions[i]->GetValue();
          }
          iEvent->SetParam[i]->setDoubleValue(newSetValue);
        }
      }

      // Print notification values after setting them
      if (iEvent->Notify && !iEvent->Notified) {
        cout << endl << "  Event " << event_ctr << " (" << iEvent->Name << ")"
             << " executed at time: " << currentTime << endl;
        for (j=0; j<iEvent->NotifyProperties.size();j++) {
          cout << "    " << iEvent->NotifyProperties[j]->GetName()
               << " = " << iEvent->NotifyProperties[j]->getDoubleValue() << endl;
        }
        cout << endl;
        iEvent->Notified = true;
      }

    }

    iEvent++;
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
           << " seconds with dt = " << State->Getdt() << endl;
      cout << endl;

      for (unsigned i=0; i<Events.size(); i++) {
        cout << "Event " << i;
        if (!Events[i].Name.empty()) cout << " (" << Events[i].Name << ")";
        cout << ":" << endl;

        if (Events[i].Persistent)
          cout << "  " << "Always executes";
        else
          cout << "  " << "Executes once";

        Events[i].Condition->PrintCondition();

        cout << endl << "  Actions taken:" << endl << "    {";
        for (unsigned j=0; j<Events[i].SetValue.size(); j++) {
          if (Events[i].SetValue[j] == 0.0 && Events[i].Functions[j] != 0L) {
            if (Events[i].SetParam[j] == 0) {
              cerr << fgred << highint << endl
                   << "  An attempt has been made to access a non-existent property" << endl
                   << "  in this event. Please check the property names used, spelling, etc."
                   << reset << endl;
              exit(-1);
            }
            cout << endl << "      set " << Events[i].SetParam[j]->GetName()
                 << " to function value";
          } else {
            if (Events[i].SetParam[j] == 0) {
              cerr << fgred << highint << endl
                   << "  An attempt has been made to access a non-existent property" << endl
                   << "  in this event. Please check the property names used, spelling, etc."
                   << reset << endl;
              exit(-1);
            }
            cout << endl << "      set " << Events[i].SetParam[j]->GetName()
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
        cout << endl << "    }" << endl << endl;

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
