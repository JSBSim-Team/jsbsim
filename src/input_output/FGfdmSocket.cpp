/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGfdmSocket.cpp
 Author:       Jon S. Berndt
 Date started: 11/08/99
 Purpose:      Encapsulates a socket
 Called by:    FGOutput, et. al.

 ------------- Copyright (C) 1999  Jon S. Berndt (jon@jsbsim.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

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

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <ws2tcpip.h>
#elif defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include <iomanip>
#include <cstring>
#include "FGfdmSocket.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

#ifndef _WIN32
// On BSD/Unix, defining the flags INVALID_SOCKET and SOCKET_ERROR to -1 allows
// using the same syntax for Windows and BSD/Unix platforms.
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef _WIN32
static bool LoadWinSockDLL(int debug_lvl)
{
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
    cerr << "Winsock DLL not initialized ..." << endl;
    return false;
  }

  if (debug_lvl > 0)
    cout << "Winsock DLL loaded ..." << endl;

  return true;
}
#endif

FGfdmSocket::FGfdmSocket(const string& address, int port, int protocol, int precision)
{
  sckt = sckt_in = INVALID_SOCKET;
  Protocol = (ProtocolType)protocol;
  connected = false;
  struct addrinfo *addr = nullptr;
  this->precision = precision;

#ifdef _WIN32
  if (!LoadWinSockDLL(debug_lvl)) return;
#endif

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  if (protocol == ptUDP)
    hints.ai_socktype = SOCK_DGRAM;
  else
    hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  if (!is_number(address))
    hints.ai_flags = AI_ADDRCONFIG;
  else
    hints.ai_flags = AI_NUMERICHOST;

  int failure = getaddrinfo(address.c_str(), NULL, &hints, &addr);
  if (failure || !addr) {
    cerr << "Could not get host net address " << address;

    if (hints.ai_flags == AI_NUMERICHOST)
       cerr << " by number..." << endl;
    else
      cerr << " by name..." << endl;

    cerr  << gai_strerror(failure) << endl;

    freeaddrinfo(addr);
    return;
  }

  sckt = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

  if (debug_lvl > 0) {
    if (protocol == ptUDP)  //use udp protocol
      cout << "Creating UDP socket on port " << port << endl;
    else //use tcp protocol
      cout << "Creating TCP socket on port " << port << endl;
  }

  if (sckt != INVALID_SOCKET) {  // successful
    int len = sizeof(struct sockaddr_in);
    memcpy(&scktName, addr->ai_addr, len);
    scktName.sin_port = htons(port);

    if (connect(sckt, (struct sockaddr*)&scktName, len) == 0) {   // successful
      if (debug_lvl > 0)
        cout << "Successfully connected to socket for output ..." << endl;
      connected = true;
    } else                // unsuccessful
      cerr << "Could not connect to socket for output ..." << endl;
  } else          // unsuccessful
    cerr << "Could not create socket for FDM output, error = " << errno << endl;

  freeaddrinfo(addr);

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// assumes TCP or UDP socket on localhost, for inbound datagrams
FGfdmSocket::FGfdmSocket(int port, int protocol, int precision)
{
  sckt = INVALID_SOCKET;
  connected = false;
  Protocol = (ProtocolType)protocol;
  string ProtocolName;
  this->precision = precision;

#ifdef _WIN32
  if (!LoadWinSockDLL(debug_lvl)) return;
#endif

  if (Protocol == ptUDP) {  //use udp protocol
    ProtocolName = "UDP";
    sckt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef _WIN32
    u_long NonBlock = 1; // True
    ioctlsocket(sckt, FIONBIO, &NonBlock);
#else
    int flags = fcntl(sckt, F_GETFL, 0);
    fcntl(sckt, F_SETFL, flags | O_NONBLOCK);
#endif
  }
  else {
    ProtocolName = "TCP";
    sckt = socket(AF_INET, SOCK_STREAM, 0);
  }

  if (debug_lvl > 0)
    cout << "Creating input " << ProtocolName << " socket on port " << port
         << endl;

  if (sckt != INVALID_SOCKET) {
    memset(&scktName, 0, sizeof(struct sockaddr_in));
    scktName.sin_family = AF_INET;
    scktName.sin_port = htons(port);

    if (Protocol == ptUDP)
      scktName.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen_t len = sizeof(struct sockaddr_in);
    if (bind(sckt, (struct sockaddr*)&scktName, len) != SOCKET_ERROR) {
      if (debug_lvl > 0)
        cout << "Successfully bound to " << ProtocolName
             << " input socket on port " << port << endl << endl;

      if (Protocol == ptTCP) {
        if (listen(sckt, 5) != SOCKET_ERROR) { // successful listen()
#ifdef _WIN32
          u_long NoBlock = 1;
          ioctlsocket(sckt, FIONBIO, &NoBlock);
#else
          int flags = fcntl(sckt, F_GETFL, 0);
          fcntl(sckt, F_SETFL, flags | O_NONBLOCK);
#endif
          sckt_in = accept(sckt, (struct sockaddr*)&scktName, &len);
          connected = true;
        } else
          cerr << "Could not listen ..." << endl;
      } else
        connected = true;
    } else                // unsuccessful
        cerr << "Could not bind to " << ProtocolName << " input socket, error = "
             << errno << endl;
  } else          // unsuccessful
      cerr << "Could not create " << ProtocolName << " socket for input, error = "
           << errno << endl;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGfdmSocket::~FGfdmSocket()
{
  if (sckt != INVALID_SOCKET) shutdown(sckt,2);
  if (sckt_in != INVALID_SOCKET) shutdown(sckt_in,2);
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGfdmSocket::Receive(void)
{
  char buf[1024];
  socklen_t len = sizeof(struct sockaddr_in);
  string data;      // todo: should allocate this with a standard size as a
                    // class attribute and pass as a reference?

  if (sckt_in == INVALID_SOCKET && Protocol == ptTCP) {
    sckt_in = accept(sckt, (struct sockaddr*)&scktName, &len);
    if (sckt_in != INVALID_SOCKET) {
#ifdef _WIN32
      u_long NoBlock = 1;
      ioctlsocket(sckt_in, FIONBIO, &NoBlock);
#else
      int flags = fcntl(sckt_in, F_GETFL, 0);
      fcntl(sckt_in, F_SETFL, flags | O_NONBLOCK);
#endif
      send(sckt_in, "Connected to JSBSim server\n\rJSBSim> ", 36, 0);
    }
  }

  if (sckt_in != INVALID_SOCKET) {
    int num_chars;

    while ((num_chars = recv(sckt_in, buf, sizeof buf, 0)) > 0) {
      data.append(buf, num_chars);
    }

#ifdef _WIN32
    // when nothing received and the error isn't "would block"
    // then assume that the client has closed the socket.
    if (num_chars == 0) {
      DWORD err = WSAGetLastError();
      if (err != WSAEWOULDBLOCK) {
        cout << "Socket Closed. Back to listening" << endl;
        closesocket(sckt_in);
        sckt_in = INVALID_SOCKET;
      }
    }
#endif
  }

  // this is for FGUDPInputSocket
  if (sckt != INVALID_SOCKET && Protocol == ptUDP) {
    struct sockaddr addr;
    socklen_t fromlen = sizeof addr;
    int num_chars = recvfrom(sckt, buf, sizeof buf, 0, (struct sockaddr*)&addr, &fromlen);
    if (num_chars > 0) data.append(buf, num_chars);
  }

  return data;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGfdmSocket::Reply(const string& text)
{
  int num_chars_sent=0;

  if (sckt_in != INVALID_SOCKET) {
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
#ifdef _WIN32
  closesocket(sckt_in);
#else
  close(sckt_in);
#endif
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
  buffer << std::setw(12) << std::setprecision(precision) << item;
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

void FGfdmSocket::WaitUntilReadable(void)
{
  if (sckt_in == INVALID_SOCKET)
    return;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sckt_in, &fds);
#ifdef _WIN32
  select(0, &fds, nullptr, nullptr, nullptr);
#else
  select(sckt_in+1, &fds, nullptr, nullptr, nullptr);
#endif

  /*
    If you want to check select return status:

    int recVal = select(sckt_in+1, &fds, nullptr, nullptr, nullptr);
    recVal: received value
    0,             if socket timeout
    -1,            if socket error
    anithing else, if socket is readable
  */
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
    }
  }
}
}
