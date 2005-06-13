/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGfdmSocket.cpp
 Author:       Jon S. Berndt
 Date started: 11/08/99
 Purpose:      Encapsulates a socket
 Called by:    FGOutput, et. al.

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
This class excapsulates a socket for simple data writing

HISTORY
--------------------------------------------------------------------------------
11/08/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGfdmSocket.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGfdmSocket.cpp,v 1.2 2005/06/13 00:54:43 jberndt Exp $";
static const char *IdHdr = ID_FDMSOCKET;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGfdmSocket::FGfdmSocket(string address, int port)
{
  size = 0;
  connected = false;

  #if defined(__BORLANDC__) || defined(_MSC_VER) || defined(__MINGW32__)
    WSADATA wsaData;
    int wsaReturnCode;
    wsaReturnCode = WSAStartup(MAKEWORD(1,1), &wsaData);
    if (wsaReturnCode == 0) cout << "Winsock DLL loaded ..." << endl;
    else cout << "Winsock DLL not initialized ..." << endl;
  #endif

  if (address.find_first_not_of("0123456789.",0) != address.npos) {
    if ((host = gethostbyname(address.c_str())) == NULL) {
      cout << "Could not get host net address by name..." << endl;
    }
  } else {
    if ((host = gethostbyaddr(address.c_str(), address.size(), PF_INET)) == NULL) {
      cout << "Could not get host net address by number..." << endl;
    }
  }

  if (host != NULL) {
    cout << "Got host net address..." << endl;
    sckt = socket(AF_INET, SOCK_STREAM, 0);

    if (sckt >= 0) {  // successful
      memset(&scktName, 0, sizeof(struct sockaddr_in));
      scktName.sin_family = AF_INET;
      scktName.sin_port = htons(port);
      memcpy(&scktName.sin_addr, host->h_addr_list[0], host->h_length);
      int len = sizeof(struct sockaddr_in);
      if (connect(sckt, (struct sockaddr*)&scktName, len) == 0) {   // successful
        cout << "Successfully connected to socket ..." << endl;
        connected = true;
      } else {                // unsuccessful
        cout << "Could not connect to socket ..." << endl;
      }
    } else {          // unsuccessful
      cout << "Could not create socket for FDM, error = " << errno << endl;
    }
  }
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGfdmSocket::~FGfdmSocket()
{
  #ifndef macintosh
  if (sckt) shutdown(sckt,2);
  #endif
  
  #ifdef __BORLANDC__
    WSACleanup();
  #endif
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Clear(void)
{
  buffer = "";
  size = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Append(const char* item)
{
  if (size == 0) buffer += string(item);
  else buffer += string(",") + string(item);
  size++;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Append(double item)
{
  char s[25];

  sprintf(s,"%12.7f",item);

  if (size == 0) buffer += string(s);
  else buffer += string(",") + string(s);
  size++;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Append(long item)
{
  char s[25];

  sprintf(s,"%12ld",item);

  if (size == 0) buffer += string(s);
  else buffer += string(",") + string(s);
  size++;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Send(void)
{
  buffer += string("\n");
  if ((send(sckt,buffer.c_str(),buffer.size(),0)) <= 0) {
    perror("send");
  } else {
  }
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

void FGfdmSocket::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGfdmSocket" << endl;
    if (from == 1) cout << "Destroyed:    FGfdmSocket" << endl;
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
