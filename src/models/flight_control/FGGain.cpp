/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGGain.cpp
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 -------------

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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGGain.h"
#include "input_output/FGXMLElement.h"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGGain.cpp,v 1.23 2011/04/18 08:51:12 andgi Exp $";
static const char *IdHdr = ID_GAIN;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGGain::FGGain(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  Element *scale_element, *zero_centered;
  string strScheduledBy, gain_string, sZeroCentered;

  GainPropertyNode = 0;
  GainPropertySign = 1.0;
  Gain = 1.000;
  Rows = 0;
  Table = 0;
  InMin = -1.0;
  InMax =  1.0;
  OutMin = OutMax = 0.0;

  if (Type == "PURE_GAIN") {
    if ( !element->FindElement("gain") ) {
      cerr << highint << "      No GAIN specified (default: 1.0)" << normint << endl;
    }
  }

  if ( element->FindElement("gain") ) {
    gain_string = element->FindElementValue("gain");
    if (!is_number(gain_string)) { // property
      if (gain_string[0] == '-') {
       GainPropertySign = -1.0;
       gain_string.erase(0,1);
      }
      GainPropertyNode = PropertyManager->GetNode(gain_string);
    } else {
      Gain = element->FindElementValueAsNumber("gain");
    }
  }

  if (Type == "AEROSURFACE_SCALE") {
    scale_element = element->FindElement("domain");
    if (scale_element) {
      if (scale_element->FindElement("max") && scale_element->FindElement("min") )
      {
        InMax = scale_element->FindElementValueAsNumber("max");
        InMin = scale_element->FindElementValueAsNumber("min");
      }
    }
    scale_element = element->FindElement("range");
    if (!scale_element) throw(string("No range supplied for aerosurface scale component"));
    if (scale_element->FindElement("max") && scale_element->FindElement("min") )
    {
      OutMax = scale_element->FindElementValueAsNumber("max");
      OutMin = scale_element->FindElementValueAsNumber("min");
    } else {
      cerr << "Maximum and minimum output values must be supplied for the "
              "aerosurface scale component" << endl;
      exit(-1);
    }
    ZeroCentered = true;
    zero_centered = element->FindElement("zero_centered");
    //ToDo if zero centered, then mins must be <0 and max's must be >0
    if (zero_centered) {
      sZeroCentered = element->FindElementValue("zero_centered");
      if (sZeroCentered == string("0") || sZeroCentered == string("false")) {
        ZeroCentered = false;
      }
    }
  }

  if (Type == "SCHEDULED_GAIN") {
    if (element->FindElement("table")) {
      Table = new FGTable(PropertyManager, element->FindElement("table"));
    } else {
      cerr << "A table must be provided for the scheduled gain component" << endl;
      exit(-1);
    }
  }

  FGFCSComponent::bind();

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
  double SchedGain = 1.0;

  Input = InputNodes[0]->getDoubleValue() * InputSigns[0];

  if (GainPropertyNode != 0) Gain = GainPropertyNode->getDoubleValue() * GainPropertySign;

  if (Type == "PURE_GAIN") {                       // PURE_GAIN

    Output = Gain * Input;

  } else if (Type == "SCHEDULED_GAIN") {           // SCHEDULED_GAIN

    SchedGain = Table->GetValue();
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

    Output *= Gain;
  }

  Clip();
  if (IsOutput) SetOutput();

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

void FGGain::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      if (InputSigns[0] < 0)
        cout << "      INPUT: -" << InputNodes[0]->GetName() << endl;
      else
        cout << "      INPUT: " << InputNodes[0]->GetName() << endl;

      if (GainPropertyNode != 0) {
        cout << "      GAIN: " << GainPropertyNode->GetName() << endl;
      } else {
        cout << "      GAIN: " << Gain << endl;
      }
      if (IsOutput) {
        for (unsigned int i=0; i<OutputNodes.size(); i++)
          cout << "      OUTPUT: " << OutputNodes[i]->getName() << endl;
      }
      if (Type == "AEROSURFACE_SCALE") {
        cout << "      In/Out Mapping:" << endl;
        cout << "        Input MIN: " << InMin << endl;
        cout << "        Input MAX: " << InMax << endl;
        cout << "        Output MIN: " << OutMin << endl;
        cout << "        Output MAX: " << OutMax << endl;
      }
      if (Table != 0) {
        cout << "      Scheduled by table: " << endl;
        Table->Print();
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGGain" << endl;
    if (from == 1) cout << "Destroyed:    FGGain" << endl;
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
