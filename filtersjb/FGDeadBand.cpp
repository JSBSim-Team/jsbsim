/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGDeadBand.cpp
 Author:       Jon S. Berndt
 Date started: 11/1999
 
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

#include "FGDeadBand.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGDeadBand.cpp,v 1.25 2003/01/22 15:53:37 jberndt Exp $";
static const char *IdHdr = ID_DEADBAND;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGDeadBand::FGDeadBand(FGFCS* fcs, FGConfigFile* AC_cfg) : FGFCSComponent(fcs),
                                                           AC_cfg(AC_cfg)
{
  Type = AC_cfg->GetValue("TYPE");
  Name = AC_cfg->GetValue("NAME");
  AC_cfg->GetNextConfigLine();
  string token;

  clipmax = clipmin = 0.0;
  clip = false;
  gain = 1.0;
  width = 0.0;

  while ((token = AC_cfg->GetValue()) != string("/COMPONENT")) {
    *AC_cfg >> token;
    if (token == "INPUT") {
      if (InputNodes.size() > 0) {
        cerr << "Deadband can only accept one input" << endl;
      } else  {
        *AC_cfg >> token;
        InputNodes.push_back(resolveSymbol(token));
      }  
    } else if (token == "WIDTH") {
      *AC_cfg >> width;
    } else if (token == "CLIPTO") {
      *AC_cfg >> clipmin >> clipmax;
      if (clipmax > clipmin) clip = true;
    } else if (token == "GAIN") {
      *AC_cfg >> gain;
    } else if (token == "OUTPUT") {
      *AC_cfg >> token;
      OutputNode = PropertyManager->GetNode(token);
    }
  }
  FGFCSComponent::bind();
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGDeadBand::~FGDeadBand()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGDeadBand::Run(void )
{
  FGFCSComponent::Run(); // call the base class for initialization of Input

  Input = InputNodes[0]->getDoubleValue();

  if (Input < -width/2.0) {
    Output = (Input + width/2.0)*gain;
  } else if (Input > width/2.0) {
    Output = (Input - width/2.0)*gain;
  } else {
    Output = 0.0;
  }

  if (clip) {
    if (Output > clipmax)      Output = clipmax;
    else if (Output < clipmin) Output = clipmin;
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

void FGDeadBand::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "      INPUT: " << InputNodes[0]->getName() << endl;
      cout << "      DEADBAND WIDTH: " << width << endl;
      cout << "      GAIN: " << gain << endl;
      if (clip) cout << "      CLIPTO: " << clipmin 
                                  << ", " << clipmax << endl;
      if (IsOutput) cout << "      OUTPUT: " << OutputNode->getName() << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGDeadBand" << endl;
    if (from == 1) cout << "Destroyed:    FGDeadBand" << endl;
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
