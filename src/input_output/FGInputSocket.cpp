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

#include "FGInputSocket.h"
#include "FGFDMExec.h"
#include "models/FGAircraft.h"
#include "FGXMLElement.h"
#include "string_utilities.h"
#include "FGLog.h"

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
    FGXMLLogging log(FDMExec->GetLogger(), el, LogLevel::ERROR);
    log << "No port assigned in input element\n";
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
  if (!socket) return;
  if (!socket->GetConnectStatus()) return;

  if (BlockingInput)
    socket->WaitUntilReadable(); // block until a transmission is received

  string raw_data = socket->Receive(); // read data

  if (!raw_data.empty()) {
    size_t start = 0;

    data += raw_data;

    // parse lines
    while (1) {
      size_t string_start = data.find_first_not_of("\r\n", start);
      if (string_start == string::npos) break;
      size_t string_end = data.find_first_of("\r\n", string_start);
      if (string_end == string::npos) break;
      string line = data.substr(string_start, string_end-string_start);
      if (line.empty()) break;

      // now parse individual line
      vector <string> tokens = split(line,' ');

      string command, argument, str_value;
      if (!tokens.empty()) {
        command = to_lower(tokens[0]);
        if (tokens.size() > 1) {
          argument = trim(tokens[1]);
          if (tokens.size() > 2) {
            str_value = trim(tokens[2]);
          }
        }
      }

      if (command == "set") {                       // SET PROPERTY
        SGPropertyNode* node = nullptr;

        if (argument.empty()) {
          socket->Reply("No property argument supplied.\r\n");
          break;
        }
        try {
          node = PropertyManager->GetNode(argument);
        } catch(...) {
          socket->Reply("Badly formed property query\r\n");
          break;
        }

        if (!node) {
          socket->Reply("Unknown property\r\n");
          break;
        } else if (!node->hasValue()) {
          socket->Reply("Not a leaf property\r\n");
          break;
        } else {
          try {
            double value = atof_locale_c(str_value);
            node->setDoubleValue(value);
          } catch(InvalidNumber& e) {
            string msg(e.what());
            msg += "\r\n";
            socket->Reply(msg);
            break;
          }
        }
        socket->Reply("set successful\r\n");

      } else if (command == "get") {             // GET PROPERTY
        SGPropertyNode* node = nullptr;

        if (argument.empty()) {
          socket->Reply("No property argument supplied.\r\n");
          break;
        }
        try {
          node = PropertyManager->GetNode(argument);
        } catch(...) {
          socket->Reply("Badly formed property query\r\n");
          break;
        }

        if (!node) {
          socket->Reply("Unknown property\r\n");
          break;
        } else if (!node->hasValue()) {
          if (Holding) { // if holding can query property list
            string query = FDMExec->QueryPropertyCatalog(argument, "\r\n");
            socket->Reply(query);
          } else {
            socket->Reply("Must be in HOLD to search properties\r\n");
          }
        } else {
          ostringstream buf;
          buf << argument << " = " << setw(12) << setprecision(6) << node->getDoubleValue() << '\r' << endl;
          socket->Reply(buf.str());
        }

      } else if (command == "hold") {               // PAUSE

        FDMExec->Hold();
        socket->Reply("Holding\r\n");

      } else if (command == "resume") {             // RESUME

        FDMExec->Resume();
        socket->Reply("Resuming\r\n");

      } else if (command == "iterate") {            // ITERATE

        int argumentInt;
        istringstream (argument) >> argumentInt;
        if (argument.empty()) {
          socket->Reply("No argument supplied for number of iterations.\r\n");
          break;
        }
        if ( !(argumentInt > 0) ){
          socket->Reply("Required argument must be a positive Integer.\r\n");
          break;
        }
        FDMExec->EnableIncrementThenHold( argumentInt );
        FDMExec->Resume();
        socket->Reply("Iterations performed\r\n");

      } else if (command == "quit") {               // QUIT

        // close the socket connection
        socket->Send("Closing connection\r\n");
        socket->Close();

      } else if (command == "info") {               // INFO

        // get info about the sim run and/or aircraft, etc.
        ostringstream info;
        info << "JSBSim version: " << JSBSim_version << "\r\n";
        info << "Config File version: " << needed_cfg_version << "\r\n";
        info << "Aircraft simulated: " << FDMExec->GetAircraft()->GetAircraftName() << "\r\n";
        info << "Simulation time: " << setw(8) << setprecision(3) << FDMExec->GetSimTime() << '\r' << endl;
        socket->Reply(info.str());

      } else if (command == "help") {               // HELP

        socket->Reply(
        " JSBSim Server commands:\r\n\r\n"
        "   get {property name}\r\n"
        "   set {property name} {value}\r\n"
        "   hold\r\n"
        "   resume\r\n"
        "   iterate {value}\r\n"
        "   help\r\n"
        "   quit\r\n"
        "   info\r\n\r\n");

      } else {
        socket->Reply(string("Unknown command: ") + command + "\r\n");
      }

      start = string_end;
    }

    // Remove processed commands.
    size_t last_crlf = data.find_last_of("\r\n");
    if (last_crlf != string::npos) {
      if (last_crlf < data.length()-1)
        data = data.substr(last_crlf+1);
      else
        data.clear();
    }
  }

}

}
