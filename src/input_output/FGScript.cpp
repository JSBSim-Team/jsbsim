/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGScript.cpp
 Author:       Jon S. Berndt
 Date started: 12/21/01
 Purpose:      Loads and runs JSBSim scripts.

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

#include <iomanip>

#include "FGScript.h"
#include "FGFDMExec.h"
#include "input_output/FGXMLFileRead.h"
#include "initialization/FGInitialCondition.h"
#include "models/FGInput.h"
#include "math/FGCondition.h"
#include "math/FGFunctionValue.h"
#include "input_output/string_utilities.h"

using namespace std;

namespace JSBSim {

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

  for (i=0; i<Events.size(); i++) {
    delete Events[i].Condition;
    for (j=0; j<Events[i].Functions.size(); j++)
      delete Events[i].Functions[j];
    for (j=0; j<Events[i].NotifyProperties.size(); j++)
      delete Events[i].NotifyProperties[j];
  }
  Events.clear();

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGScript::LoadScript(const SGPath& script, double default_dT,
                          const SGPath& initfile)
{
  SGPath initialize;
  string aircraft="", prop_name="";
  string notifyPropertyName="";
  Element *element=0, *run_element=0, *event_element=0;
  Element *set_element=0;
  Element *notify_element = 0L, *notify_property_element = 0L;
  double dt = 0.0, value = 0.0;
  FGCondition *newCondition;

  FGXMLFileRead XMLFileRead;
  Element* document = XMLFileRead.LoadXMLDocument(script);

  if (!document) {
    cerr << "File: " << script << " could not be loaded." << endl;
    return false;
  }

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

  if (run_element->HasAttribute("start"))
    StartTime = run_element->GetAttributeValueAsNumber("start");
  else
    StartTime = 0.0;
  FDMExec->Setsim_time(StartTime);
  if (run_element->HasAttribute("end")) {
    EndTime   = run_element->GetAttributeValueAsNumber("end");
  } else {
    cerr << "An end time (duration) for the script must be specified in the script <run> element." << endl;
    return false;
  }

  if (default_dT == 0.0)
    dt = run_element->GetAttributeValueAsNumber("dt");
  else {
    dt = default_dT;
    cout << endl << "Overriding simulation step size from the command line. New step size is: "
         << default_dT << " seconds (" << 1/default_dT << " Hz)" << endl << endl;
  }

  FDMExec->Setdt(dt);

  // Make sure that the desired time is reached and executed.
  EndTime += 0.99*FDMExec->GetDeltaT();

  // read aircraft and initialization files

  element = document->FindElement("use");
  if (element) {
    aircraft = element->GetAttributeValue("aircraft");
    if (!aircraft.empty()) {
      if (!FDMExec->LoadModel(aircraft))
        return false;
    } else {
      cerr << "Aircraft must be specified in use element." << endl;
      return false;
    }

    initialize = SGPath::fromLocal8Bit(element->GetAttributeValue("initialize").c_str());
    if (initfile.isNull()) {
      if (initialize.isNull()) {
        cerr << "Initialization file must be specified in use element." << endl;
        return false;
      }
    } else {
      cout << endl << "The initialization file specified in the script file ("
           << initialize << ") has been overridden with a specified file ("
           << initfile << ")." << endl;
      initialize = initfile;
    }

  } else {
    cerr << "No \"use\" directives in the script file." << endl;
    return false;
  }

  auto IC = FDMExec->GetIC();
  if ( ! IC->Load( initialize )) {
    cerr << "Initialization unsuccessful" << endl;
    return false;
  }

  // Now, read input spec if given.
  element = document->FindElement("input");
  while (element) {
    if (!FDMExec->GetInput()->Load(element))
      return false;

    element = document->FindNextElement("input");
  }

  // Now, read output spec if given.
  element = document->FindElement("output");
  SGPath scriptDir = SGPath(script.dir());
  if (scriptDir.isNull())
    scriptDir = SGPath(".");

  while (element) {
    if (!FDMExec->GetOutput()->Load(element, scriptDir))
      return false;

    element = document->FindNextElement("output");
  }

  // Read local property/value declarations
  int saved_debug_lvl = debug_lvl;
  debug_lvl = 0; // Disable messages
  LocalProperties.Load(run_element, PropertyManager.get(), true);
  debug_lvl = saved_debug_lvl;

  // Read "events" from script

  event_element = run_element->FindElement("event");
  while (event_element) { // event processing

    // Create the event structure
    struct event *newEvent = new struct event();

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
    Element* condition_element = event_element->FindElement("condition");
    if (condition_element) {
      try {
        newCondition = new FGCondition(condition_element, PropertyManager);
      } catch(BaseException& e) {
        cerr << condition_element->ReadFrom()
             << fgred << e.what() << reset << endl << endl;
        delete newEvent;
        return false;
      }
      newEvent->Condition = newCondition;
    } else {
      cerr << "No condition specified in script event " << newEvent->Name
           << endl;
      delete newEvent;
      return false;
    }

    // Is there a delay between the time this event is triggered, and when the
    // event actions are executed?

    Element* delay_element = event_element->FindElement("delay");
    if (delay_element)
      newEvent->Delay = event_element->FindElementValueAsNumber("delay");
    else
      newEvent->Delay = 0.0;

    // Notify about when this event is triggered?
    if ((notify_element = event_element->FindElement("notify")) != 0) {
      if (notify_element->HasAttribute("format")) {
        if (notify_element->GetAttributeValue("format") == "kml") newEvent->NotifyKML = true;
      }
      newEvent->Notify = true;
      // Check here for new <description> tag that gets echoed
      string notify_description = notify_element->FindElementValue("description");
      if (!notify_description.empty()) {
        newEvent->Description = notify_description;
      }
      notify_property_element = notify_element->FindElement("property");
      while (notify_property_element) {
        notifyPropertyName = notify_property_element->GetDataLine();

        if (notify_property_element->HasAttribute("apply")) {
          string function_str = notify_property_element->GetAttributeValue("apply");
          auto f = FDMExec->GetTemplateFunc(function_str);
          if (f)
            newEvent->NotifyProperties.push_back(new FGFunctionValue(notifyPropertyName, PropertyManager, f,
                                                                     notify_property_element));
          else {
            cerr << notify_property_element->ReadFrom()
              << fgred << highint << "  No function by the name "
              << function_str << " has been defined. This property will "
              << "not be logged. You should check your configuration file."
              << reset << endl;
          }
        }
        else
          newEvent->NotifyProperties.push_back(new FGPropertyValue(notifyPropertyName, PropertyManager,
                                                                   notify_property_element));

        string caption_attribute = notify_property_element->GetAttributeValue("caption");
        if (caption_attribute.empty()) {
          newEvent->DisplayString.push_back(notifyPropertyName);
        } else {
          newEvent->DisplayString.push_back(caption_attribute);
        }

        notify_property_element = notify_element->FindNextElement("property");
      }
    }

    // Read set definitions (these define the actions to be taken when the event
    // is triggered).
    set_element = event_element->FindElement("set");
    while (set_element) {
      prop_name = set_element->GetAttributeValue("name");
      if (PropertyManager->HasNode(prop_name)) {
        newEvent->SetParam.push_back( PropertyManager->GetNode(prop_name) );
      } else {
        newEvent->SetParam.push_back( 0L );
      }
      newEvent->SetParamName.push_back( prop_name );

      // Todo - should probably do some safety checking here to make sure one or
      // the other of value or function is specified.
      if (!set_element->GetAttributeValue("value").empty()) {
        value = set_element->GetAttributeValueAsNumber("value");
        newEvent->Functions.push_back(nullptr);
      } else if (set_element->FindElement("function")) {
        value = 0.0;
        newEvent->Functions.push_back(new FGFunction(FDMExec, set_element->FindElement("function")));
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

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGScript::ResetEvents(void)
{
  LocalProperties.ResetToIC();
  FDMExec->Setsim_time(StartTime);

  for (unsigned int i=0; i<Events.size(); i++)
    Events[i].reset();
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

    struct event &thisEvent = Events[ev_ctr];

    // Determine whether the set of conditional tests for this condition equate
    // to true and should cause the event to execute. If the conditions evaluate
    // to true, then the event is triggered. If the event is not persistent,
    // then this trigger will remain set true. If the event is persistent, the
    // trigger will reset to false when the condition evaluates to false.
    if (thisEvent.Condition->Evaluate()) {
      if (!thisEvent.Triggered) {

        // The conditions are true, do the setting of the desired Event
        // parameters
        for (i=0; i<thisEvent.SetValue.size(); i++) {
          if (thisEvent.SetParam[i] == 0L) { // Late bind property if necessary
            if (PropertyManager->HasNode(thisEvent.SetParamName[i])) {
              thisEvent.SetParam[i] = PropertyManager->GetNode(thisEvent.SetParamName[i]);
            } else {
              throw("No property, \""+thisEvent.SetParamName[i]+"\" is defined.");
            }
          }
          thisEvent.OriginalValue[i] = thisEvent.SetParam[i]->getDoubleValue();
          if (thisEvent.Functions[i] != 0) { // Parameter should be set to a function value
            try {
              thisEvent.SetValue[i] = thisEvent.Functions[i]->GetValue();
            } catch (string& msg) {
              std::cerr << std::endl << "A problem occurred in the execution of the script. " << msg << endl;
              throw;
            }
          }
          switch (thisEvent.Type[i]) {
          case FG_VALUE:
          case FG_BOOL:
            thisEvent.newValue[i] = thisEvent.SetValue[i];
            break;
          case FG_DELTA:
            thisEvent.newValue[i] = thisEvent.OriginalValue[i] + thisEvent.SetValue[i];
            break;
          default:
            cerr << "Invalid Type specified" << endl;
            break;
          }
          thisEvent.StartTime = currentTime + thisEvent.Delay;
          thisEvent.ValueSpan[i] = thisEvent.newValue[i] - thisEvent.OriginalValue[i];
          thisEvent.Transiting[i] = true;
        }
      }
      thisEvent.Triggered = true;

    } else if (thisEvent.Persistent) { // If the event is persistent, reset the trigger.
      thisEvent.Triggered = false; // Reset the trigger for persistent events
      thisEvent.Notified = false;  // Also reset the notification flag
    } else if (thisEvent.Continuous) { // If the event is continuous, reset the trigger.
      thisEvent.Triggered = false; // Reset the trigger for persistent events
      thisEvent.Notified = false;  // Also reset the notification flag
    }

    if ((currentTime >= thisEvent.StartTime) && thisEvent.Triggered) {

      for (i=0; i<thisEvent.SetValue.size(); i++) {
        if (thisEvent.Transiting[i]) {
          thisEvent.TimeSpan = currentTime - thisEvent.StartTime;
          switch (thisEvent.Action[i]) {
          case FG_RAMP:
            if (thisEvent.TimeSpan <= thisEvent.TC[i]) {
              newSetValue = thisEvent.TimeSpan/thisEvent.TC[i] * thisEvent.ValueSpan[i] + thisEvent.OriginalValue[i];
            } else {
              newSetValue = thisEvent.newValue[i];
              if (thisEvent.Continuous != true) thisEvent.Transiting[i] = false;
            }
            break;
          case FG_STEP:
            newSetValue = thisEvent.newValue[i];

            // If this is not a continuous event, reset the transiting flag.
            // Otherwise, it is known that the event is a continuous event.
            // Furthermore, if the event is to be determined by a function,
            // then the function will be continuously calculated.
            if (thisEvent.Continuous != true)
              thisEvent.Transiting[i] = false;
            else if (thisEvent.Functions[i] != 0)
              newSetValue = thisEvent.Functions[i]->GetValue();

            break;
          case FG_EXP:
            newSetValue = (1 - exp( -thisEvent.TimeSpan/thisEvent.TC[i] )) * thisEvent.ValueSpan[i] + thisEvent.OriginalValue[i];
            break;
          default:
            cerr << "Invalid Action specified" << endl;
            break;
          }
          thisEvent.SetParam[i]->setDoubleValue(newSetValue);
        }
      }

      // Print notification values after setting them
      if (thisEvent.Notify && !thisEvent.Notified) {
        if (thisEvent.NotifyKML) {
          cout << endl << "<Placemark>" << endl;
          cout << "  <name> " << currentTime << " seconds" << " </name>"
               << endl;
          cout << "  <description>" << endl;
          cout << "  <![CDATA[" << endl;
          cout << "  <b>" << thisEvent.Name << " (Event " << event_ctr << ")"
               << " executed at time: " << currentTime << "</b><br/>" << endl;
        } else  {
          cout << endl << underon
               << highint << thisEvent.Name << normint << underoff
               << " (Event " << event_ctr << ")"
               << " executed at time: " << highint << currentTime << normint
               << endl;
        }
        if (!thisEvent.Description.empty()) {
          cout << "    " << thisEvent.Description << endl;
        }
        for (j=0; j<thisEvent.NotifyProperties.size();j++) {
          cout << "    " << thisEvent.DisplayString[j] << " = "
               << thisEvent.NotifyProperties[j]->getDoubleValue();
          if (thisEvent.NotifyKML) cout << " <br/>";
          cout << endl;
        }
        if (thisEvent.NotifyKML) {
          cout << "  ]]>" << endl;
          cout << "  </description>" << endl;
          cout << "  <Point>" << endl;
          cout << "    <altitudeMode> absolute </altitudeMode>" << endl;
          cout << "    <extrude> 1 </extrude>" << endl;
          cout << "    <coordinates>"
               << FDMExec->GetPropagate()->GetLongitudeDeg() << ","
               << FDMExec->GetPropagate()->GetGeodLatitudeDeg() << ","
               << FDMExec->GetPropagate()->GetAltitudeASLmeters()
               << "</coordinates>" << endl;
          cout << "  </Point>" << endl;
          cout << "</Placemark>" << endl;
        }
        cout << endl;
        thisEvent.Notified = true;
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
           << " seconds with dt = " << setprecision(6) << FDMExec->GetDeltaT()
           << " (" << ceil(1.0/FDMExec->GetDeltaT()) << " Hz)" << endl;
      cout << endl;

      for (auto node: LocalProperties) {
        cout << "Local property: " << node->getNameString()
             << " = " << node->getDoubleValue()
             << endl;
      }

      if (LocalProperties.empty()) cout << endl;

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
              if (Events[i].SetParamName[j].empty()) {
                stringstream s;
                s << "  An attempt has been made to access a non-existent property" << endl
                  << "  in this event. Please check the property names used, spelling, etc.";
                cerr << fgred << highint << endl << s.str() << reset << endl;
                throw BaseException(s.str());
              } else {
                cout << endl << "      set " << Events[i].SetParamName[j]
                     << " to function value (Late Bound)";
              }
            } else {
              cout << endl << "      set "
                   << GetRelativeName(Events[i].SetParam[j], "/fdm/jsbsim/")
                   << " to function value";
            }
          } else {
            if (Events[i].SetParam[j] == 0) {
              if (Events[i].SetParamName[j].empty()) {
                stringstream s;
                s << "  An attempt has been made to access a non-existent property" << endl
                  << "  in this event. Please check the property names used, spelling, etc.";
                cerr << fgred << highint << endl << s.str() << reset << endl;
                throw BaseException(s.str());
              } else {
                cout << endl << "      set " << Events[i].SetParamName[j]
                     << " to function value (Late Bound)";
              }
            } else {
              cout << endl << "      set "
                   << GetRelativeName(Events[i].SetParam[j], "/fdm/jsbsim/")
                   << " to " << Events[i].SetValue[j];
            }
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
          if (!Events[i].NotifyProperties.empty()) {
            if (Events[i].NotifyKML) {
              cout << "  Notifications (KML Format):" << endl << "    {"
                   << endl;
            } else {
              cout << "  Notifications:" << endl << "    {" << endl;
            }
            for (unsigned j=0; j<Events[i].NotifyProperties.size();j++) {
              cout << "      "
                   << Events[i].NotifyProperties[j]->GetPrintableName()
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
    }
  }
}
}
