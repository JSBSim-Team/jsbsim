/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGfdmSocket.cpp
 Author:       Jon S. Berndt
 Date started: 11/08/99
 Purpose:      Encapsulates a socket
 Called by:    FGOutput, et. al.

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

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
This class excapsulates a socket for simple data writing

HISTORY
--------------------------------------------------------------------------------
11/08/99   JSB   Created
11/08/07   HDW   Added Generic Socket Send

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include "FGfdmSocket.h"
#include "string_utilities.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace JSBSim {

static const char *IdSrc = "$Id: FGfdmSocket.cpp,v 1.27 2010/05/13 03:07:59 jberndt Exp $";
static const char *IdHdr = ID_FDMSOCKET;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGfdmSocket::FGfdmSocket(const string& address, int port, int protocol)
{
  sckt = sckt_in = 0;
  connected = false;

  #if defined(_MSC_VER) || defined(__MINGW32__)
    WSADATA wsaData;
    int wsaReturnCode;
    wsaReturnCode = WSAStartup(MAKEWORD(1,1), &wsaData);
    if (wsaReturnCode == 0) cout << "Winsock DLL loaded ..." << endl;
    else cout << "Winsock DLL not initialized ..." << endl;
  #endif

  if (!is_number(address)) {
    if ((host = gethostbyname(address.c_str())) == NULL) {
      cout << "Could not get host net address by name..." << endl;
    }
  } else {
    unsigned int ip;
    ip = inet_addr(address.c_str());
    if ((host = gethostbyaddr((char*)&ip, address.size(), PF_INET)) == NULL) {
      cout << "Could not get host net address by number..." << endl;
    }
  }

  if (host != NULL) {
    if (protocol == ptUDP) {  //use udp protocol
       sckt = socket(AF_INET, SOCK_DGRAM, 0);
       cout << "Creating UDP socket on port " << port << endl;
    }
    else { //use tcp protocol
       sckt = socket(AF_INET, SOCK_STREAM, 0);
       cout << "Creating TCP socket on port " << port << endl;
    }

    if (sckt >= 0) {  // successful
      memset(&scktName, 0, sizeof(struct sockaddr_in));
      scktName.sin_family = AF_INET;
      scktName.sin_port = htons(port);
      memcpy(&scktName.sin_addr, host->h_addr_list[0], host->h_length);
      int len = sizeof(struct sockaddr_in);
      if (connect(sckt, (struct sockaddr*)&scktName, len) == 0) {   // successful
        cout << "Successfully connected to socket for output ..." << endl;
        connected = true;
      } else {                // unsuccessful
        cout << "Could not connect to socket for output ..." << endl;
      }
    } else {          // unsuccessful
      cout << "Could not create socket for FDM output, error = " << errno << endl;
    }
  }
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGfdmSocket::FGfdmSocket(const string& address, int port)
{
  sckt = sckt_in = 0;
  connected = false;

  #if defined(_MSC_VER) || defined(__MINGW32__)
    WSADATA wsaData;
    int wsaReturnCode;
    wsaReturnCode = WSAStartup(MAKEWORD(1,1), &wsaData);
    if (wsaReturnCode == 0) cout << "Winsock DLL loaded ..." << endl;
    else cout << "Winsock DLL not initialized ..." << endl;
  #endif

  cout << "... Socket Configuration Sanity Check ..." << endl;
  cout << "Host name...   " << address << ",  Port...  " << port << "." << endl;
  cout << "Host name... (char)  " << address.c_str() << "." << endl;

  if (!is_number(address)) {
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
        cout << "Successfully connected to socket for output ..." << endl;
        connected = true;
      } else {                // unsuccessful
        cout << "Could not connect to socket for output ..." << endl;
      }
    } else {          // unsuccessful
      cout << "Could not create socket for FDM output, error = " << errno << endl;
    }
  }
  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGfdmSocket::FGfdmSocket(int port)
{
  connected = false;
  unsigned long NoBlock = true;

  #if defined(_MSC_VER) || defined(__MINGW32__)
    WSADATA wsaData;
    int wsaReturnCode;
    wsaReturnCode = WSAStartup(MAKEWORD(1,1), &wsaData);
    if (wsaReturnCode == 0) cout << "Winsock DLL loaded ..." << endl;
    else cerr << "Winsock DLL not initialized ..." << endl;
  #endif

  sckt = socket(AF_INET, SOCK_STREAM, 0);

  if (sckt >= 0) {  // successful
    memset(&scktName, 0, sizeof(struct sockaddr_in));
    scktName.sin_family = AF_INET;
    scktName.sin_port = htons(port);
    int len = sizeof(struct sockaddr_in);
    if (bind(sckt, (struct sockaddr*)&scktName, len) == 0) {   // successful
      cout << "Successfully bound to socket for input on port " << port << endl;
      if (listen(sckt, 5) >= 0) { // successful listen()
        #if defined(_MSC_VER) || defined(__MINGW32__)
          ioctlsocket(sckt, FIONBIO, &NoBlock);
          sckt_in = accept(sckt, (struct sockaddr*)&scktName, &len);
        #else
          ioctl(sckt, FIONBIO, &NoBlock);
          sckt_in = accept(sckt, (struct sockaddr*)&scktName, (socklen_t*)&len);
        #endif
      } else {
        cerr << "Could not listen ..." << endl;
      }
      connected = true;
    } else {                // unsuccessful
      cerr << "Could not bind to socket for input ..." << endl;
    }
  } else {          // unsuccessful
    cerr << "Could not create socket for FDM input, error = " << errno << endl;
  }

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGfdmSocket::~FGfdmSocket()
{
  if (sckt) shutdown(sckt,2);
  if (sckt_in) shutdown(sckt_in,2);
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGfdmSocket::Receive(void)
{
  char buf[1024];
  int len = sizeof(struct sockaddr_in);
  int num_chars=0;
  unsigned long NoBlock = true;
  string data;      // todo: should allocate this with a standard size as a
                    // class attribute and pass as a reference?

  if (sckt_in <= 0) {
    #if defined(_MSC_VER) || defined(__MINGW32__)
      sckt_in = accept(sckt, (struct sockaddr*)&scktName, &len);
    #else
      sckt_in = accept(sckt, (struct sockaddr*)&scktName, (socklen_t*)&len);
    #endif
    if (sckt_in > 0) {
      #if defined(_MSC_VER) || defined(__MINGW32__)
         ioctlsocket(sckt_in, FIONBIO,&NoBlock);
      #else
         ioctl(sckt_in, FIONBIO, &NoBlock);
      #endif
      send(sckt_in, "Connected to JSBSim server\nJSBSim> ", 35, 0);
    }
  }

  if (sckt_in > 0) {
    while ((num_chars = recv(sckt_in, buf, sizeof buf, 0)) > 0) {
      data.append(buf, num_chars);
    }

#if defined(_MSC_VER)
    // when nothing received and the error isn't "would block"
    // then assume that the client has closed the socket.
    if (num_chars == 0) {
        DWORD err = WSAGetLastError ();
        if (err != WSAEWOULDBLOCK) {
            printf ("Socket Closed. back to listening\n");
            closesocket (sckt_in);
            sckt_in = -1;
        }
    }
#endif
  }

  return data;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGfdmSocket::Reply(const string& text)
{
  int num_chars_sent=0;

  if (sckt_in >= 0) {
    num_chars_sent = send(sckt_in, text.c_str(), text.size(), 0);
    send(sckt_in, "JSBSim> ", 8, 0);
  } else {
    cerr << "Socket reply must be to a valid socket" << endl;
    return -1;
  }
  return num_chars_sent;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Close(void)
{
  close(sckt_in);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Clear(void)
{
  buffer.str(string());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Clear(const string& s)
{
  Clear();
  buffer << s << ' ';
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Append(const char* item)
{
  if (buffer.tellp() > 0) buffer << ',';
  buffer << item;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Append(double item)
{
  if (buffer.tellp() > 0) buffer << ',';
  buffer << std::setw(12) << std::setprecision(7) << item;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Append(long item)
{
  if (buffer.tellp() > 0) buffer << ',';
  buffer << std::setw(12) << item;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Send(void)
{
  buffer << '\n';
  string str = buffer.str();
  if ((send(sckt,str.c_str(),str.size(),0)) <= 0) {
    perror("send");
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Send(const char *data, int length)
{
  if ((send(sckt,data,length,0)) <= 0) {
    perror("send");
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
