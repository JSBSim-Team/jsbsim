/*******************************************************************************

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

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGfdmSocket.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGfdmSocket::FGfdmSocket(string address, int port)
{
  size = 0;

  #if defined(__BORLANDC__) || defined(_MSC_VER)
    WSADATA wsaData;
    int PASCAL FAR wsaReturnCode;
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
      } else {                // unsuccessful
        cout << "Could not connect to socket ..." << endl;
      }
    } else {          // unsuccessful
      cout << "Could not create socket for FDM, error = " << errno << endl;
    }
  }
}

FGfdmSocket::~FGfdmSocket(void)
{
  if (sckt) shutdown(sckt,2);
  #ifdef __BORLANDC__
    WSACleanup();
  #endif
}

void FGfdmSocket::Clear(void)
{
  buffer = "";
  size = 0;
}

void FGfdmSocket::Append(const char* item)
{
  if (size == 0) buffer += string(item);
  else buffer += string(",") + string(item);
  size++;
}

void FGfdmSocket::Append(float item)
{
  char s[25];

  sprintf(s,"%12.7f\0",item);

  if (size == 0) buffer += string(s);
  else buffer += string(",") + string(s);
  size++;
}

void FGfdmSocket::Append(long item)
{
  char s[25];

  sprintf(s,"%12d\0",item);

  if (size == 0) buffer += string(s);
  else buffer += string(",") + string(s);
  size++;
}

void FGfdmSocket::Send(void)
{
  buffer += string("\n");
  if ((send(sckt,buffer.c_str(),buffer.size(),0)) <= 0) {
    perror("send");
  } else {
  }
}

