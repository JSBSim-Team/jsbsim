/*******************************************************************************

 Header:       FGfdmSocket.h
 Author:       Jon S. Berndt
 Date started: 11/08/99

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

HISTORY
--------------------------------------------------------------------------------
11/08/99   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGfdmSocket_H
#define FGfdmSocket_H

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
*******************************************************************************/

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include <stdio.h>

#ifdef FGFS
#  pragma message("FGFS defined")
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <iostream>
#    include <fstream>
#  else
#    include <iostream.h>
#    include <fstream.h>
#  endif
#else
#  pragma message("FGFS not defined")
#  include <iostream>
#  include <fstream>
#endif

#include <string>
#include <sys/types.h>

#if defined(__BORLANDC__) || defined(_MSC_VER)
  #include <winsock.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <errno.h>
#endif

/*******************************************************************************
DEFINITIONS
*******************************************************************************/

using std::cout;
using std::endl;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

using std::string;

class FGfdmSocket {
public:
  FGfdmSocket(string, int);
  ~FGfdmSocket(void);
  void Send(void);
  void Append(const char*);
  void Append(float);
  void Append(long);
  void Clear(void);

private:
  int sckt;
  int size;
  struct sockaddr_in scktName;
  struct hostent *host;
  string buffer;
};

#endif
