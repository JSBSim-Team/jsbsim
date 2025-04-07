/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGGain.cpp
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 -------------

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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGGain.h"
#include "models/FGFCS.h"
#include "math/FGParameterValue.h"
#include "math/FGTable.h"
#include "input_output/FGLog.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGGain::FGGain(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  Gain = nullptr;
  Table = nullptr;
  InMin = -1.0;
  InMax =  1.0;
  OutMin = OutMax = 0.0;

  CheckInputNodes(1, 1, element);

  if (Type == "PURE_GAIN") {
    if ( !element->FindElement("gain") ) {
      FGXMLLogging log(fcs->GetExec()->GetLogger(), element, LogLevel::ERROR);
      log << LogFormat::BOLD << "      No GAIN specified (default: 1.0)\n";
    }
  }

  auto PropertyManager = fcs->GetPropertyManager();
  Element* gain_element = element->FindElement("gain");
  if (gain_element)
    Gain = new FGParameterValue(gain_element, PropertyManager);
  else
    Gain = new FGRealValue(1.0);

  if (Type == "AEROSURFACE_SCALE") {
    Element* scale_element = element->FindElement("domain");
    if (scale_element) {
      if (scale_element->FindElement("max") && scale_element->FindElement("min") )
      {
        InMax = scale_element->FindElementValueAsNumber("max");
        InMin = scale_element->FindElementValueAsNumber("min");
      }
    }
    scale_element = element->FindElement("range");
    if (!scale_element) {
      XMLLogException err(fcs->GetExec()->GetLogger(), scale_element);
      err << "No range supplied for aerosurface scale component\n";
      throw err;
    }
    if (scale_element->FindElement("max") && scale_element->FindElement("min") )
    {
      OutMax = scale_element->FindElementValueAsNumber("max");
      OutMin = scale_element->FindElementValueAsNumber("min");
    } else {
      XMLLogException err(fcs->GetExec()->GetLogger(), scale_element);
      err << "Maximum and minimum output values must be supplied for the "
             "aerosurface scale component\n";
      throw err;
    }
    ZeroCentered = true;
    Element* zero_centered = element->FindElement("zero_centered");
    //ToDo if zero centered, then mins must be <0 and max's must be >0
    if (zero_centered) {
      string sZeroCentered = element->FindElementValue("zero_centered");
      if (sZeroCentered == string("0") || sZeroCentered == string("false")) {
        ZeroCentered = false;
      }
    }
  }

  if (Type == "SCHEDULED_GAIN") {
    if (element->FindElement("table")) {
      Table = new FGTable(PropertyManager, element->FindElement("table"));
    } else {
      XMLLogException err(fcs->GetExec()->GetLogger(), element);
      err << "A table must be provided for the scheduled gain component\n";
      throw err;
    }
  }

  bind(element, PropertyManager.get());

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGain::~FGGain()
{
  delete Table;

  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGain::Run(void )
{
  Input = InputNodes[0]->getDoubleValue();

  if (Type == "PURE_GAIN") {                       // PURE_GAIN

    Output = Gain * Input;

  } else if (Type == "SCHEDULED_GAIN") {           // SCHEDULED_GAIN

    double SchedGain = Table->GetValue();
    Output = Gain * SchedGain * Input;

  } else if (Type == "AEROSURFACE_SCALE") {        // AEROSURFACE_SCALE

    if (ZeroCentered) {
      if (Input == 0.0) {
        Output = 0.0;
      } else if (Input > 0) {
        Output = (Input / InMax) * OutMax;
      } else {
        Output = (Input / InMin) * OutMin;
      }
    } else {
      Output = OutMin + ((Input - InMin) / (InMax - InMin)) * (OutMax - OutMin);
    }

    Output *= Gain->GetValue();
  }

  Clip();
  SetOutput();

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
//    1: This value explicitly requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGGain::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
      log << "      INPUT: " << InputNodes[0]->GetNameWithSign() << "\n";
      log << "      GAIN: " << Gain->GetName() << fixed << "\n";

      for (auto node: OutputNodes)
        log << "      OUTPUT: " << node->getNameString() << "\n";

      if (Type == "AEROSURFACE_SCALE") {
        log << "      In/Out Mapping:\n" << setprecision(4);
        log << "        Input MIN: " << InMin << "\n";
        log << "        Input MAX: " << InMax << "\n";
        log << "        Output MIN: " << OutMin << "\n";
        log << "        Output MAX: " << OutMax << "\n";
      }
      if (Table) {
        log << "      Scheduled by table:\n";
        Table->Print();
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGGain\n";
    if (from == 1) log << "Destroyed:    FGGain\n";
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
