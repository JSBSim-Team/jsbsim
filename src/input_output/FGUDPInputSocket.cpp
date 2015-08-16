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

IDENT(IdSrc,"$Id");
IDENT(IdHdr,ID_UDPINPUTSOCKET);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGUDPInputSocket::FGUDPInputSocket(FGFDMExec* fdmex) :
  FGInputType(fdmex),
  socket(0)
{
  rate = 20;
  SockPort = 5139;
  oldTimeStamp = 0.0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGUDPInputSocket::~FGUDPInputSocket()
{
  delete socket;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGUDPInputSocket::Load(Element* el)
{
  if (!FGInputType::Load(el))
    return false;
   
  rate = atoi(el->GetAttributeValue("rate").c_str());
  SetRate(0.5 + 1.0/(FDMExec->GetDeltaT()*rate));
   
  SockPort = atoi(el->GetAttributeValue("port").c_str());
  if (SockPort == 0) {
    cerr << endl << "No port assigned in input element" << endl;
    return false;
  }
  
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

bool FGUDPInputSocket::InitModel(void)
{
  if (FGInputType::InitModel()) {
    delete socket;
    socket = new FGfdmSocket(SockPort, FGfdmSocket::ptUDP, FGfdmSocket::dIN);

    if (socket == 0) return false;
    cout << "UDP input socket established on port " << SockPort << endl;
    return true;
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGUDPInputSocket::Read(bool Holding)
{

  if (socket == 0) return;
    
  data = socket->Receive();
 
  if (data.size() > 0) {
  
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
