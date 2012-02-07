/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGOutput.h
 Author:       Jon Berndt
 Date started: 12/2/98

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

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created
11/09/07   HDW   Added FlightGear Socket Interface

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGOUTPUT_H
#define FGOUTPUT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"

#include <fstream>

#include "input_output/FGXMLFileRead.h"
#include "input_output/net_fdm.hxx"
#include "input_output/FGfdmSocket.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_OUTPUT "$Id: FGOutput.h,v 1.25 2012/02/07 23:15:37 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGfdmSocket;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Handles simulation output.
    OUTPUT section definition

    The following specifies the way that JSBSim writes out data.
<pre>
    NAME is the filename you want the output to go to

    TYPE can be:
      CSV         Comma separated data. If a filename is supplied then the
                  data goes to that file. If "COUT" or "cout" is specified, the
                  data goes to stdout. If the filename is a null filename the
                  data goes to stdout, as well.
      SOCKET      Will eventually send data to a socket output, where NAME
                  would then be the IP address of the machine the data should
                  be sent to. DON'T USE THIS YET!
      FLIGHTGEAR  A socket is created for sending binary data packets to
                  an external instance of FlightGear for visuals.  Parameters
                  defining the socket are given on the \<output> line.
      TABULAR     Columnar data.
      TERMINAL    Output to terminal. NOT IMPLEMENTED YET!
      NONE        Specifies to do nothing. This setting makes it easy to turn on and
                  off the data output without having to mess with anything else.

      Examples:
</pre>
@code
<output name="localhost" type="FLIGHTGEAR" port="5500" protocol="tcp" rate="10"/>
@endcode
@code
<output name="B737_datalog.csv" type="CSV" rate="20">
   <property> velocities/vc-kts </property>
   <velocities> ON </velocities>
</output>
@endcode
<br>
<pre>
    The arguments that can be supplied, currently, are:

    RATE_IN_HZ  An integer rate in times-per-second that the data is output. This
                value may not be *exactly* what you want, due to the dependence
                on dt, the cycle rate for the FDM.

    The following parameters tell which subsystems of data to output:

    simulation       ON|OFF
    atmosphere       ON|OFF
    massprops        ON|OFF
    aerosurfaces     ON|OFF
    rates            ON|OFF
    velocities       ON|OFF
    forces           ON|OFF
    moments          ON|OFF
    position         ON|OFF
    coefficients     ON|OFF
    ground_reactions ON|OFF
    fcs              ON|OFF
    propulsion       ON|OFF
</pre>
    NOTE that Time is always output with the data.
    @version $Id: FGOutput.h,v 1.25 2012/02/07 23:15:37 bcoconni Exp $
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGOutput : public FGModel, public FGXMLFileRead
{
public:
  FGOutput(FGFDMExec*);
  ~FGOutput();

  bool InitModel(void);
  /** Runs the Output model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from 
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);

  void Print(void);
  void DelimitedOutput(const std::string&);
  void SocketOutput(void);
  void FlightGearSocketOutput(void);
  void SocketStatusOutput(const std::string&);
  void SocketDataFill(FGNetFDM* net);

  void SetType(const std::string& type);
  void SetProtocol(const std::string& protocol);
  void SetPort(const std::string& port);
  void SetStartNewFile(bool tt) {StartNewFile = tt;}
  void SetSubsystems(int tt) {SubSystems = tt;}
  void SetOutputFileName(const std::string& fname) {Filename = fname;}
  void SetDirectivesFile(const std::string& fname) {DirectivesFile = fname;}
  void SetRate(double rt);
  void Enable(void) { enabled = true; }
  void Disable(void) { enabled = false; }
  bool Toggle(void) {enabled = !enabled; return enabled;}

  bool Load(Element* el);
  bool Load(int subSystems, std::string protocol, std::string type, std::string port, 
                            std::string name, double outRate,
                            std::vector<FGPropertyManager *> & outputProperties);
  string GetOutputFileName(void) const {return Filename;}

  /// Subsystem types for specifying which will be output in the FDM data logging
  enum  eSubSystems {
    /** Subsystem: Simulation (= 1)          */ ssSimulation      = 1,
    /** Subsystem: Aerosurfaces (= 2)        */ ssAerosurfaces    = 2,
    /** Subsystem: Body rates (= 4)          */ ssRates           = 4,
    /** Subsystem: Velocities (= 8)          */ ssVelocities      = 8,
    /** Subsystem: Forces (= 16)             */ ssForces          = 16,
    /** Subsystem: Moments (= 32)            */ ssMoments         = 32,
    /** Subsystem: Atmosphere (= 64)         */ ssAtmosphere      = 64,
    /** Subsystem: Mass Properties (= 128)   */ ssMassProps       = 128,
    /** Subsystem: Coefficients (= 256)      */ ssAeroFunctions    = 256,
    /** Subsystem: Propagate (= 512)         */ ssPropagate       = 512,
    /** Subsystem: Ground Reactions (= 1024) */ ssGroundReactions = 1024,
    /** Subsystem: FCS (= 2048)              */ ssFCS             = 2048,
    /** Subsystem: Propulsion (= 4096)       */ ssPropulsion      = 4096
  } subsystems;

  FGNetFDM fgSockBuf;

private:
  enum {otNone, otCSV, otTab, otSocket, otTerminal, otFlightGear, otUnknown} Type;
  FGfdmSocket::ProtocolType Protocol;
  bool sFirstPass, dFirstPass, enabled;
  int SubSystems;
  int runID_postfix;
  bool StartNewFile;
  std::string output_file_name, delimeter, BaseFilename, Filename, DirectivesFile;
  std::string Port;
  std::ofstream datafile;
  FGfdmSocket* socket;
  std::vector <FGPropertyManager*> OutputProperties;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

