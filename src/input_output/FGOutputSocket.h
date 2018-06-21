/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGOutputSocket.h
 Author:       Bertrand Coconnier
 Date started: 09/10/11

 ------------- Copyright (C) 2011 Bertrand Coconnier -------------

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

HISTORY
--------------------------------------------------------------------------------
09/10/11   BC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGOUTPUTSOCKET_H
#define FGOUTPUTSOCKET_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGOutputType.h"
#include "input_output/net_fdm.hxx"
#include "input_output/FGfdmSocket.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_OUTPUTSOCKET "$Id: FGOutputSocket.h,v 1.3 2013/01/12 18:08:40 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Implements the output to a socket. This class outputs data to a socket
    according to the JSBSim format. It can be inherited as a generic class that
    provides services for socket outputs. For instance FGOutputFG inherits
    FGOutputSocket for the socket management but outputs data with a format
    different than FGOutputSocket.
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGOutputSocket : public FGOutputType
{
public:
  /** Constructor. */
  FGOutputSocket(FGFDMExec* fdmex);

  /** Destructor. */
  ~FGOutputSocket();

  /** Overwrites the name identifier under which the output will be logged.
      This method is taken into account if it is called before
      FGFDMExec::RunIC() otherwise it is ignored until the next call to
      SetStartNewOutput().
      @param name new name in the form "hostname:port/proto"
                  hostname could be an ip, port a numerical value and
                  proto should be UDP or TCP (the default if omitted)
  */
  virtual void SetOutputName(const std::string& name);

  /** Overwrites the bool variable waitSocketReply, false by default.
      @param wait true/false
  */
  virtual void SetWaitSocketReply(const bool wait);

  /** Retrieves the value of waitSocketReply.
      @result the bool value of waitSocketReply */
  inline bool IsWaitSocketReply() {
	  return waitSocketReply;
  }

  /** Init the output directives from an XML file.
      @param element XML Element that is pointing to the output directives
  */
  virtual bool Load(Element* el);

  /** Initializes the instance. This method basically opens the socket to which
      outputs will be directed.
      @result true if the execution succeeded.
   */
  bool InitModel(void);
  /// Generates the output.
  void Print(void);

  /// Parse the string read from socket and apply commands
  void ParseReply(const std::string& data);

  /** Outputs a status thru the socket. This method issues a message prepended
      by the string "<STATUS>" to the socket.
      @param out_str status message
   */
  void SocketStatusOutput(const std::string& out_str);

protected:
  virtual void PrintHeaders(void);

  std::string SockName;
  unsigned int SockPort;
  FGfdmSocket::ProtocolType SockProtocol;
  FGfdmSocket* socket;

  bool waitSocketReply;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

