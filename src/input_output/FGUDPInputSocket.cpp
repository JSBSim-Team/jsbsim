/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGUDPInputSocket.cpp
 Author:       Dave Culp
 Date started: 02/19/15
 Purpose:      Manage input of data from a UDP socket
 Called by:    FGInput

 ------------- Copyright (C) 2015 Dave Culp --------------

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
This class establishes a UDP socket and reads data from it.

HISTORY
--------------------------------------------------------------------------------
02/19/15   DC   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstring>
#include <cstdlib>
#include <sstream>

#include "FGUDPInputSocket.h"
#include "FGFDMExec.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGUDPInputSocket::FGUDPInputSocket(FGFDMExec* fdmex) :
  FGInputSocket(fdmex), rate(20), oldTimeStamp(0.0)
{
  SockPort = 5139;
  SockProtocol = FGfdmSocket::ptUDP;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGUDPInputSocket::Load(Element* el)
{
  if (!FGInputSocket::Load(el))
    return false;
   
  rate = atoi(el->GetAttributeValue("rate").c_str());
  SetRate(0.5 + 1.0/(FDMExec->GetDeltaT()*rate));
  
  Element *property_element = el->FindElement("property");

  while (property_element) {
    string property_str = property_element->GetDataLine();
    FGPropertyNode* node = PropertyManager->GetNode(property_str);
    if (!node) {
      cerr << fgred << highint << endl << "  No property by the name "
           << property_str << " can be found." << reset << endl;
    } else {
      InputProperties.push_back(node);
    }
    property_element = el->FindNextElement("property");
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGUDPInputSocket::Read(bool Holding)
{
  if (socket == 0) return;
    
  data = socket->Receive();
 
  if (!data.empty()) {
  
    vector<string> tokens;
    stringstream ss(data);
    string temp;
    while (getline(ss, temp, ',')) {
       tokens.push_back(temp);
    }
    
    vector<double> values;
  
    for (unsigned int i=0; i<tokens.size(); i++) {
      values.push_back( atof(tokens[i].c_str()) );
     }
     
    if (values[0] < oldTimeStamp) {
      return;
    } else {
      oldTimeStamp = values[0];
    }
     
    // the zeroeth value is the time stamp 
    if ((values.size() - 1) != InputProperties.size()) {
      cerr << endl << "Mismatch between UDP input property and value counts." << endl;
      return;
    }
    
    for (unsigned int i=1; i<values.size(); i++) {
      InputProperties[i-1]->setDoubleValue(values[i]);
    }
  }
}

}
