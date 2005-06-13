/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGGain.cpp
 Author:       Jon S. Berndt
 Date started: 4/2000

 ------------- Copyright (C) 2000 -------------

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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGGain.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGGain.cpp,v 1.2 2005/06/13 00:54:45 jberndt Exp $";
static const char *IdHdr = ID_GAIN;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGGain::FGGain(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element)
{
  Element *scale_element;
  string strScheduledBy;

  Gain = 1.000;
  Rows = 0;
  Table = 0;
  Min = Max = OutputPct = 0.0;

  if ( element->FindElement("gain") ) {
    Gain = element->FindElementValueAsNumber("gain");
  }

  if (Type == "PURE_GAIN") {
    if ( !element->FindElement("gain") ) {
      cerr << "No GAIN supplied for PURE_GAIN component: " << Name << endl;
      exit(-1);
    }
  }

  if (Type == "AEROSURFACE_SCALE") {
    scale_element = element->FindElement("limit");
    if (scale_element->FindElement("max") && scale_element->FindElement("min") )
    {
      Max = scale_element->FindElementValueAsNumber("max");
      Min = scale_element->FindElementValueAsNumber("min");
    } else {
      cerr << "A maximum and minimum scale value must be supplied for the "
              "aerosurface scale component" << endl;
      exit(-1);
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

// this output related to normalization may not be accurate. It assumes the input
// will be in a rnage from 0 or -1 to +1. This may not always be the case.

//  if (Type == "AEROSURFACE_SCALE")
//    treenode->Tie( "output-norm", this, &FGGain::GetOutputPct );

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGGain::~FGGain()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGGain::Run(void )
{
  double SchedGain = 1.0;

  Input = InputNodes[0]->getDoubleValue() * InputSigns[0];

  if (Type == "PURE_GAIN") {                       // PURE_GAIN

    Output = Gain * Input;

  } else if (Type == "SCHEDULED_GAIN") {           // SCHEDULED_GAIN

    SchedGain = Table->GetValue();
    Output = Gain * SchedGain * Input;

  } else if (Type == "AEROSURFACE_SCALE") {        // AEROSURFACE_SCALE

    OutputPct = Input;
    if (Input >= 0.0) Output = Input * Max;
    else Output = Input * -Min;
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
        cout << "      INPUT: -" << InputNodes[0]->getName() << endl;
      else
        cout << "      INPUT: " << InputNodes[0]->getName() << endl;

      cout << "      GAIN: " << Gain << endl;
      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
      if (Type == "AEROSURFACE_SCALE") {
        cout << "      MIN: " << Min << endl;
        cout << "      MAX: " << Max << endl;
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
