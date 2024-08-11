/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGfdmSocket.h
 Author:       Jon S. Berndt
 Date started: 11/08/99

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

HISTORY
--------------------------------------------------------------------------------
11/08/99   JSB   Created
11/08/07   HDW   Added Generic Socket Send

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGfdmSocket_H
#define FGfdmSocket_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
  #include <winsock.h>
  #include <io.h>
  #undef ERROR
#else
  #include <netdb.h>
#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/**
 * @brief The FGfdmSocket class enables JSBSim to communicate via sockets.
 *
 * This class provides functionality for sending and receiving data over a socket
 * connection.
 * It can behave as both a client and/or a server, depending on the constructor used.
 * The socket can use either UDP or TCP protocol for communication.
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGfdmSocket : public FGJSBBase
{
public:
  /**
   * @brief Construct a client socket.
   *
   * @param address The IP address or hostname of the server to connect to.
   * @param port The port number to connect to.
   * @param protocol The protocol to use for communication (ptUDP or ptTCP).
   * @param precision The precision to use for floating-point numbers (default is 7).
   */
  FGfdmSocket(const std::string& address, int port, int protocol, int precision = 7);

  /**
   * @brief Construct a server socket.
   *
   * @param port The port number to listen on.
   * @param protocol The protocol to use for communication (ptUDP or ptTCP).
   * @param precision The precision to use for floating-point numbers (default is 7).
   */
  FGfdmSocket(int port, int protocol, int precision = 7);

  ~FGfdmSocket();

  /// Send the internal buffer over the socket connection.
  void Send(void);

  /**
   * @brief Send the specified data over the socket connection.
   *
   * @param data The data to send.
   * @param length The length of the data.
   */
  void Send(const char *data, int length);
  void Send(const std::string& data) { Send(data.c_str(), data.length()); }

  /**
   * @brief Receive data from the socket connection.
   *
   * @return The received data as a string.
   */
  std::string Receive(void);

  /**
   * @brief Send a reply to the client ending by a prompt "JSBSim>"
   *
   * @param text The reply text to send.
   * @return The number of bytes sent.
   */
  int Reply(const std::string& text);

  /**
   * @brief Append the specified string to the internal buffer.
   *
   * @param s The string to append.
   */
  void Append(const std::string& s) {Append(s.c_str());}

  /**
   * @brief Append the specified C-style string to the internal buffer.
   *
   * @param s The C-style string to append.
   */
  void Append(const char*);

  /**
   * @brief Append the specified double value to the internal buffer.
   *
   * @param value The double value to append.
   */
  void Append(double value);

  /**
   * @brief Append the specified long value to the internal buffer.
   *
   * @param value The long value to append.
   */
  void Append(long value);

  /// Clear the internal buffer.
  void Clear(void);

  /**
   * @brief Clear the internal buffer and appends the specified string.
   *
   * @param s The string to append after clearing the buffer.
   */
  void Clear(const std::string& s);

  /// Close the socket connection if the protocol is TCP.
  void Close(void);

  /**
   * @brief Return the connection status of the socket.
   *
   * @return True if the socket is connected, false otherwise.
   */
  bool GetConnectStatus(void) {return connected;}

  /// Wait until the TCP socket is readable.
  void WaitUntilReadable(void);

  enum ProtocolType {ptUDP, ptTCP};

private:
#if defined(_MSC_VER) || defined(__MINGW32__)
  SOCKET sckt;
  SOCKET sckt_in;
#else
  int sckt;
  int sckt_in;
#endif
  ProtocolType Protocol;
  struct sockaddr_in scktName;
  struct hostent *host;
  std::ostringstream buffer;
  int precision;
  bool connected;
  void LogSocketError(const std::string& msg);
  void Debug(int from);
};
}
#endif
