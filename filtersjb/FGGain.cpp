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

static const char *IdSrc = "$Id: FGGain.cpp,v 1.54 2005/01/27 12:23:11 jberndt Exp $";
static const char *IdHdr = ID_GAIN;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGGain::FGGain(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                   AC_cfg(AC_cfg)
{
  string token;
  string strScheduledBy;
  string sOutputIdx;

  State = fcs->GetState();

  Gain = 1.000;
  Rows = 0;
  Min = Max = 0.0;
  OutputPct = 0;
  invert = false;
  ScheduledBy = 0;
  clip = false;
  clipmin = clipmax = 0.0;

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/COMPONENT")) {
    *AC_cfg >> token;
    if (token == "INPUT") {
      *AC_cfg >> token;

      if (token[0] == '-') {
        invert = true;
        token.erase(0,1);
      }

      if (InputNodes.size() > 0) {
        cerr << "Gains can only accept one input" << endl;
      } else  {
        InputNodes.push_back( resolveSymbol(token) );
      }

    } else if (token == "GAIN") {
      *AC_cfg >> Gain;
    } else if (token == "MIN") {
      *AC_cfg >> Min;
    } else if (token == "MAX") {
      *AC_cfg >> Max;
    } else if (token == "CLIPTO") {
      *AC_cfg >> clipmin >> clipmax;
      if (clipmax > clipmin) {
        clip = true;
      }
    } else if (token == "INVERT") {
      invert = true;
      cerr << endl << "The INVERT keyword is being deprecated and will not be "
                      "supported in the future. Please use a minus sign in front "
                      "of an input property in the future." << endl << endl;
    } else if (token == "ROWS") {
      *AC_cfg >> Rows;
      Table = new FGTable(Rows);
    } else if (token == "SCHEDULED_BY") {
      token = AC_cfg->GetValue("SCHEDULED_BY");
      *AC_cfg >> strScheduledBy;
      ScheduledBy = PropertyManager->GetNode( strScheduledBy );
    } else if (token == "OUTPUT") {
      IsOutput = true;
      *AC_cfg >> sOutputIdx;
      OutputNode = PropertyManager->GetNode( sOutputIdx, true );
    } else {
      AC_cfg->ResetLineIndexToZero();
      *Table << *AC_cfg;
    }
  }

  FGFCSComponent::bind();
  if (Type == "AEROSURFACE_SCALE")
    treenode->Tie( "output-norm", this, &FGGain::GetOutputPct );

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
  double LookupVal = 0;

  FGFCSComponent::Run(); // call the base class for initialization of Input
  Input = InputNodes[0]->getDoubleValue();

  if (invert) Input = -Input;

  if (Type == "PURE_GAIN") {                       // PURE_GAIN

    Output = Gain * Input;

  } else if (Type == "SCHEDULED_GAIN") {           // SCHEDULED_GAIN

    LookupVal = ScheduledBy->getDoubleValue();
    SchedGain = Table->GetValue(LookupVal);
    Output = Gain * SchedGain * Input;

  } else if (Type == "AEROSURFACE_SCALE") {        // AEROSURFACE_SCALE

    OutputPct = Input;
    if (Input >= 0.0) Output = Input * Max;
    else Output = Input * -Min;
    Output *= Gain;

  }

  if (clip) {
    if (Output > clipmax)      Output = clipmax;
    else if (Output < clipmin) Output = clipmin;
  }

  if (IsOutput) SetOutput();

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGGain::convert(void)
{
  cout << endl;
  cout << "        <component name=\"" << Name << "\" type=\"" << Type << "\">" << endl;

  cout << "            <input>" << (InputNodes[0]->GetFullyQualifiedName()).substr(12) << "</input>" << endl;

  if (Gain != 1.0)
    cout << "            <gain>" << Gain << "</gain>" << endl;

  if (Type == "PURE_GAIN") {                       // PURE_GAIN
  } else if (Type == "SCHEDULED_GAIN") {           // SCHEDULED_GAIN
  } else if (Type == "AEROSURFACE_SCALE") {        // AEROSURFACE_SCALE
    cout << "            <limit>" << endl;
    cout << "                <min>" << Min << "</min>" << endl;
    cout << "                <max>" << Max << "</max>" << endl;
    cout << "            </limit>" << endl;
  }

  if (clip) {
    cout << "            <clip>" << endl;
    cout << "                <min>" << clipmin << "</min>" << endl;
    cout << "                <max>" << clipmax << "</max>" << endl;
    cout << "            </clip>" << endl;
  }

  if (IsOutput)
    cout << "            <output>" << (OutputNode->GetFullyQualifiedName()).substr(12) << "</output>" << endl;

  cout << "        </component>" << endl;
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
      if (invert)
        cout << "      INPUT: -" << InputNodes[0]->getName() << endl;
      else
        cout << "      INPUT: " << InputNodes[0]->getName() << endl;

      cout << "      GAIN: " << Gain << endl;
      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
      cout << "      MIN: " << Min << endl;
      cout << "      MAX: " << Max << endl;
      if (ScheduledBy != 0) {
        cout << "      Scheduled by parameter: " << ScheduledBy->getName() << endl;
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
