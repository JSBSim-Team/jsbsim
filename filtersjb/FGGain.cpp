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

static const char *IdSrc = "$Id: FGGain.cpp,v 1.46 2003/01/12 11:50:31 jberndt Exp $";
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
  OutputPct=0;
  invert=false;
  ScheduledBy = 0;

  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();

  while ((token = AC_cfg->GetValue()) != string("/COMPONENT")) {
    *AC_cfg >> token;
    if (token == "INPUT") {
      token = AC_cfg->GetValue("INPUT");
      if (InputNodes.size() > 0) {
        cerr << "Gains can only accept one input" << endl;
      } else  {
        *AC_cfg >> token;
        InputNodes.push_back( resolveSymbol(token) );
      }  
    } else if (token == "GAIN") {
      *AC_cfg >> Gain;
    } else if (token == "MIN") {
      *AC_cfg >> Min;
    } else if (token == "MAX") {
      *AC_cfg >> Max;
    } else if (token == "INVERT") {
      invert=true;  
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
      OutputNode = PropertyManager->GetNode( sOutputIdx );

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
  if (Type == "PURE_GAIN") {
    Output = Gain * Input;
  } else if (Type == "SCHEDULED_GAIN") {
    LookupVal = ScheduledBy->getDoubleValue();
    SchedGain = Table->GetValue(LookupVal);
    Output = Gain * SchedGain * Input;
  } else if (Type == "AEROSURFACE_SCALE") {
    if (!invert) {
      OutputPct = Input;
      if (Input >= 0.0) Output = Input * Max;
      else Output = Input * -Min;
    } else {
      OutputPct=-1*Input;
      if (Input <= 0.0) Output = Input * -Max;
      else Output = Input * Min;
    } 
    Output *= Gain;
  }

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
      cout << "      INPUT: " << InputNodes[0]->getName() << endl;
      cout << "      GAIN: " << Gain << endl;
      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
      cout << "      MIN: " << Min << endl;
      cout << "      MAX: " << Max << endl;
      if(invert) cout << "      Invert mapping" << endl;
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

