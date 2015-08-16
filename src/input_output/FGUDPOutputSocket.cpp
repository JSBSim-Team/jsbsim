/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGUDPOutputSocket.cpp
 Author:       David Culp
 Date started: 03/31/15
 Purpose:      Manage output of property values to a UDP socket
 Called by:    FGOutput

 ------------- Copyright (C) 2015 David Culp ----------------

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
This class sends comma-separated strings over a UDP socket.  The format is that required
by the QtJSBSim application.

HISTORY
--------------------------------------------------------------------------------
03/31/15   DC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstring>
#include <cstdlib>

#include "FGUDPOutputSocket.h"
#include "FGFDMExec.h"
#include "input_output/FGPropertyManager.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

IDENT(IdSrc,"$Id: FGUDPOutputSocket.cpp,v 1.2 2015/08/16 13:19:52 bcoconni Exp $");
IDENT(IdHdr,ID_UDPOUTPUTSOCKET);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGUDPOutputSocket::FGUDPOutputSocket(FGFDMExec* fdmex) :
  FGOutputType(fdmex),
  socket(0)
{
  FDMExec = fdmex;
  PropertyManager = fdmex->GetPropertyManager();
  root = PropertyManager->GetNode(); 
  root->SetDouble("simulation/null", 0.0);
  SockName = "localhost";
  SockPort = 5138;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGUDPOutputSocket::~FGUDPOutputSocket()
{
  delete socket;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGUDPOutputSocket::Load(Element* el)
{

  Element *property_element = el->FindElement("property");

  while (property_element) {
    string property_str = property_element->GetDataLine();
    FGPropertyNode* node = PropertyManager->GetNode(property_str);
    if (!node) {
      node = PropertyManager->GetNode("simulation/null");
    }
    OutputProperties.push_back(node);
    property_element = el->FindNextElement("property");
  }

  double outRate = 1.0;
  if (!el->GetAttributeValue("rate").empty()) {
    outRate = el->GetAttributeValueAsNumber("rate");
  }
  SetRateHz(outRate);
  
  SockPort = atoi(el->GetAttributeValue("port").c_str());
  if (SockPort == 0) {
    cerr << endl << "No port assigned for output." << endl;
    return false;
  }

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGUDPOutputSocket::InitModel(void)
{
  if (FGOutputType::InitModel()) {
    delete socket;
    socket = new FGfdmSocket(SockName, SockPort, FGfdmSocket::ptUDP);

    if (socket == 0) return false;
    if (!socket->GetConnectStatus()) return false;

    return true;
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGUDPOutputSocket::Print(void)
{

  if (socket == 0) return;
  if (!socket->GetConnectStatus()) return;

  socket->Clear();
  socket->Append(FDMExec->GetSimTime());

  for (unsigned int i=0;i<OutputProperties.size();i++) {
    socket->Append(OutputProperties[i]->getDoubleValue());
  }

  socket->Send();
}

}
