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
#include <iostream>
#include <sstream>
#include <cstring>
#include <assert.h>

#include "FGfdmSocket.h"
#include "input_output/string_utilities.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

// Defines that make BSD/Unix sockets and Windows sockets syntax look alike.
#ifndef _WIN32
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SD_BOTH SHUT_RDWR
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
          connected = true;
#ifdef _WIN32
          u_long NoBlock = 1;
          ioctlsocket(sckt, FIONBIO, &NoBlock);
#else
          int flags = fcntl(sckt, F_GETFL, 0);
          fcntl(sckt, F_SETFL, flags | O_NONBLOCK);
#endif
          sckt_in = accept(sckt, (struct sockaddr*)&scktName, &len);
        } else {
          closesocket(sckt);
          sckt = INVALID_SOCKET;
          cerr << "Could not listen ..." << endl;
        }
      } else
        connected = true;
    } else {                // unsuccessful
      closesocket(sckt);
      sckt = INVALID_SOCKET;
      cerr << "Could not bind to " << ProtocolName << " input socket, error = "
           << errno << endl;
    }
  } else          // unsuccessful
      cerr << "Could not create " << ProtocolName << " socket for input, error = "
           << errno << endl;

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGfdmSocket::~FGfdmSocket()
{
  // Release the file descriptors to the OS.
  if (sckt_in != INVALID_SOCKET) shutdown(sckt_in, SD_BOTH);
  if (sckt != INVALID_SOCKET) closesocket(sckt);
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

string FGfdmSocket::Receive(void)
{
  char buf[1024];
  string data;      // TODO should allocate this with a standard size as a
                    // class attribute and pass as a reference?

  if (Protocol == ptTCP){
    if (sckt_in == INVALID_SOCKET) {
      socklen_t len = sizeof(struct sockaddr_in);
      sckt_in = accept(sckt, (struct sockaddr*)&scktName, &len);
      if (sckt_in != INVALID_SOCKET) {
#ifdef _WIN32
        u_long NoBlock = 1;
        ioctlsocket(sckt_in, FIONBIO, &NoBlock);
#else
        int flags = fcntl(sckt_in, F_GETFL, 0);
        fcntl(sckt_in, F_SETFL, flags | O_NONBLOCK);
#endif
        if (send(sckt_in, "Connected to JSBSim server\r\nJSBSim> ", 36, 0) == SOCKET_ERROR)
          LogSocketError("Receive - TCP connection acknowledgement");
      }
    }

    if (sckt_in != INVALID_SOCKET) {
      int num_chars;

      while ((num_chars = recv(sckt_in, buf, sizeof buf, 0)) > 0)
        data.append(buf, num_chars);

      if (num_chars == SOCKET_ERROR || num_chars == 0) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK)
#else
        if (errno != EWOULDBLOCK)
#endif
        {
          LogSocketError("Receive - TCP data reception");
          // when nothing received and the error isn't "would block"
          // then assume that the client has closed the socket.
          cout << "Socket Closed. Back to listening" << endl;
          closesocket(sckt_in);
          sckt_in = INVALID_SOCKET;
        }
      }
    }
  }

  // this is for FGUDPInputSocket
  if (sckt != INVALID_SOCKET && Protocol == ptUDP) {
    struct sockaddr addr;
    socklen_t fromlen = sizeof addr;
    int num_chars = recvfrom(sckt, buf, sizeof buf, 0, (struct sockaddr*)&addr, &fromlen);
    if (num_chars > 0) data.append(buf, num_chars);
    if (num_chars == SOCKET_ERROR) {
#ifdef _WIN32
      if (WSAGetLastError() != WSAEWOULDBLOCK)
#else
      if (errno != EWOULDBLOCK)
#endif
        LogSocketError("Receive - UDP data reception");
    }
  }

  return data;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int FGfdmSocket::Reply(const string& text)
{
  int num_chars_sent=0;
  assert(Protocol == ptTCP);

  if (sckt_in != INVALID_SOCKET) {
    num_chars_sent = send(sckt_in, text.c_str(), text.size(), 0);
    if (num_chars_sent == SOCKET_ERROR) LogSocketError("Reply - Send data");
    if (send(sckt_in, "JSBSim> ", 8, 0) == SOCKET_ERROR) LogSocketError("Reply - Prompt");
  } else {
    cerr << "Socket reply must be to a valid socket" << endl;
    return -1;
  }
  return num_chars_sent;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Close(void)
{
  assert(Protocol == ptTCP);
  closesocket(sckt_in);
  sckt_in = INVALID_SOCKET;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Clear(void)
{
  buffer.str("");
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
  Send(str.c_str(), str.size());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::Send(const char *data, int length)
{
  if (Protocol == ptTCP && sckt_in != INVALID_SOCKET) {
    if ((send(sckt_in, data, length, 0)) == SOCKET_ERROR) LogSocketError("Send - TCP data sending");
    return;
  }

  if (Protocol == ptUDP && sckt != INVALID_SOCKET) {
    if ((send(sckt, data, length, 0)) == SOCKET_ERROR) LogSocketError("Send - UDP data sending");
    return;
  }

  cerr << "Data sending must be to a valid socket" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::WaitUntilReadable(void)
{
  assert(Protocol == ptTCP);
  if (sckt_in == INVALID_SOCKET) return;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sckt_in, &fds);

  int result = select(FD_SETSIZE, &fds, nullptr, nullptr, nullptr);

  if (result == 0) {
    cerr << "Socket timeout." << endl;
    return;
  } else if (result != SOCKET_ERROR)
    return;

  LogSocketError("WaitUntilReadable");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGfdmSocket::LogSocketError(const std::string& msg)
{
  // An error has occurred, display the error message.
  cerr << "Socket error in " << msg << ": ";
#ifdef _WIN32
  LPSTR errorMessage = nullptr;
  DWORD errorCode = WSAGetLastError();
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessage, 0, nullptr);
  cerr << errorMessage << endl;
  LocalFree(errorMessage);
#else
  cerr << strerror(errno) << endl;
#endif
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
