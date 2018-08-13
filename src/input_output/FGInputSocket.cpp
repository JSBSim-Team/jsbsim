/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGInputSocket.cpp
 Author:       Paul Chavent
 Date started: 01/20/15
 Purpose:      Manage input of sim parameters to a socket
 Called by:    FGInput

 ------------- Copyright (C) 2015 Paul Chavent -------------

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
This is the place where you create input routines to dump data for perusal
later.

HISTORY
--------------------------------------------------------------------------------
01/20/15   PC   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iomanip>

#include "FGInputSocket.h"
#include "FGFDMExec.h"
#include "models/FGAircraft.h"
#include "input_output/FGXMLElement.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGInputSocket::FGInputSocket(FGFDMExec* fdmex) :
  FGInputType(fdmex), socket(0), SockProtocol(FGfdmSocket::ptTCP),
  BlockingInput(false)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGInputSocket::~FGInputSocket()
{
  delete socket;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInputSocket::Load(Element* el)
{
  if (!FGInputType::Load(el))
    return false;

  SockPort = atoi(el->GetAttributeValue("port").c_str());

  if (SockPort == 0) {
    cerr << endl << "No port assigned in input element" << endl;
    return false;
  }

  string action = el->GetAttributeValue("action");
  if (to_upper(action) == "BLOCKING_INPUT")
    BlockingInput = true;

  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGInputSocket::InitModel(void)
{
  if (FGInputType::InitModel()) {
    delete socket;
    socket = new FGfdmSocket(SockPort, SockProtocol);

    if (socket == 0) return false;
    if (!socket->GetConnectStatus()) return false;

    return true;
  }

  return false;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGInputSocket::Read(bool Holding)
{
  string line, token;
  size_t start=0, string_start=0, string_end=0;
  double value=0;
  FGPropertyNode* node=0;

  if (socket == 0) return;
  if (!socket->GetConnectStatus()) return;

  if (BlockingInput)
    socket->WaitUntilReadable(); // block until a transmission is received
  data = socket->Receive(); // read data

  if (data.size() > 0) {
    // parse lines
    while (1) {
      string_start = data.find_first_not_of("\r\n", start);
      if (string_start == string::npos) break;
      string_end = data.find_first_of("\r\n", string_start);
      if (string_end == string::npos) break;
      line = data.substr(string_start, string_end-string_start);
      if (line.size() == 0) break;

      // now parse individual line
      vector <string> tokens = split(line,' ');

      string command="", argument="", str_value="";
      if (tokens.size() > 0) {
        command = to_lower(tokens[0]);
        if (tokens.size() > 1) {
          argument = trim(tokens[1]);
          if (tokens.size() > 2) {
            str_value = trim(tokens[2]);
          }
        }
      }

      if (command == "set") {                       // SET PROPERTY

        if (argument.size() == 0) {
          socket->Reply("No property argument supplied.\n");
          break;
        }
        try {
          node = PropertyManager->GetNode(argument);
        } catch(...) {
          socket->Reply("Badly formed property query\n");
          break;
        }

        if (node == 0) {
          socket->Reply("Unknown property\n");
          break;
        } else if (!node->hasValue()) {
          socket->Reply("Not a leaf property\n");
          break;
        } else {
          value = atof(str_value.c_str());
          node->setDoubleValue(value);
        }
        socket->Reply("set successful\n");

      } else if (command == "get") {             // GET PROPERTY

        if (argument.size() == 0) {
          socket->Reply("No property argument supplied.\n");
          break;
        }
        try {
          node = PropertyManager->GetNode(argument);
        } catch(...) {
          socket->Reply("Badly formed property query\n");
          break;
        }

        if (node == 0) {
          socket->Reply("Unknown property\n");
          break;
        } else if (!node->hasValue()) {
          if (Holding) { // if holding can query property list
            string query = FDMExec->QueryPropertyCatalog(argument);
            socket->Reply(query);
          } else {
            socket->Reply("Must be in HOLD to search properties\n");
          }
        } else {
          ostringstream buf;
          buf << argument << " = " << setw(12) << setprecision(6) << node->getDoubleValue() << endl;
          socket->Reply(buf.str());
        }

      } else if (command == "hold") {               // PAUSE

        FDMExec->Hold();
        socket->Reply("Holding\n");

      } else if (command == "resume") {             // RESUME

        FDMExec->Resume();
        socket->Reply("Resuming\n");

      } else if (command == "iterate") {            // ITERATE

        int argumentInt;
        istringstream (argument) >> argumentInt;
        if (argument.size() == 0) {
          socket->Reply("No argument supplied for number of iterations.\n");
          break;
        }
        if ( !(argumentInt > 0) ){
          socket->Reply("Required argument must be a positive Integer.\n");
          break;
        }
        FDMExec->EnableIncrementThenHold( argumentInt );
        FDMExec->Resume();
        socket->Reply("Iterations performed\n");

      } else if (command == "quit") {               // QUIT

        // close the socket connection
        socket->Reply("Closing connection\n");
        socket->Close();

      } else if (command == "info") {               // INFO

        // get info about the sim run and/or aircraft, etc.
        ostringstream info;
        info << "JSBSim version: " << JSBSim_version << endl;
        info << "Config File version: " << needed_cfg_version << endl;
        info << "Aircraft simulated: " << FDMExec->GetAircraft()->GetAircraftName() << endl;
        info << "Simulation time: " << setw(8) << setprecision(3) << FDMExec->GetSimTime() << endl;
        socket->Reply(info.str());

      } else if (command == "help") {               // HELP

        socket->Reply(
        " JSBSim Server commands:\n\n"
        "   get {property name}\n"
        "   set {property name} {value}\n"
        "   hold\n"
        "   resume\n"
        "   iterate {value}\n"
        "   help\n"
        "   quit\n"
        "   info\n\n");

      } else {
        socket->Reply(string("Unknown command: ") +  token + string("\n"));
      }

      start = string_end;
    }
  }

}

}
