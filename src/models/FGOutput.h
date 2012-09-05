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
09/10/11   BC    Broke Down the Code in Several Classes

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGOUTPUT_H
#define FGOUTPUT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "input_output/FGOutputType.h"
#include "input_output/FGXMLFileRead.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_OUTPUT "$Id: FGOutput.h,v 1.26 2012/09/05 21:49:19 bcoconni Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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

    The class FGOutput is the manager of the outputs requested by the user. It
    manages a list of instances derived from the abstract class FGOutputType.
    @version $Id: FGOutput.h,v 1.26 2012/09/05 21:49:19 bcoconni Exp $
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGOutput : public FGModel, public FGXMLFileRead
{
public:
  FGOutput(FGFDMExec*);
  ~FGOutput();

  /** Initializes the instance. This method is called by FGFDMExec::RunIC().
      This is were the initialization of all classes derived from FGOutputType
      takes place. It is important that this method is not called prior
      to FGFDMExec::RunIC() so that the initialization process can be executed
      properly.
      @result true if the execution succeeded. */
  bool InitModel(void);
  /** Runs the Output model; called by the Executive.
      Can pass in a value indicating if the executive is directing the
      simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim
                     from advancing time. Some models may ignore this flag, such
                     as the Input model, which may need to be active to listen
                     on a socket for the "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding);
  /** Makes all the output instances to generate their ouput. This method does
      not check that the time step at which the output is requested is
      consistent with the output rate RATE_IN_HZ. Although Print is not a
      relevant name for outputs like SOCKET or FLIGHGEAR, it has been kept for
      backward compatibility. */
  void Print(void);
  /** Force an output instance to generate its output. The code executed is
      basically the same than the code of the method Print() except that the
      ouput is limited to the instance identified by the parameter of the
      method.
      @param idx ID of the instance that will generate its ouput */
  void ForceOutput(int idx);
  /** Reset the output prior to a restart of the simulation. This method should
      be called when the simulation is restarted with, for example, new initial
      conditions. When this method is executed the output instances can take
      special actions such as closing the current output file and open a new
      one with a different name. */
  void SetStartNewOutput(void);
  /** Overwrites the name identifier under which the output will be logged.
      This method is taken into account if it is called between Load() and
      FGFDMExec::RunIC() otherwise it is ignored until the next call to
      SetStartNewOutput().
      @param idx ID of the instance which name identifier will be changed
      @param name new name
      @result false if the instance does not exists. */
  bool SetOutputName(unsigned int idx, const std::string& name);
  /** Adds a new output instance to the Output Manager. The definition of the
      new output instance is read from a file.
      @param fname the name of the file from which the ouput directives should
                   be read.
      @return true if the execution succeeded. */
  bool SetDirectivesFile(const std::string& fname);
  /// Enables the output generation for all output instances.
  void Enable(void);
  /// Disables the output generation for all output instances.
  void Disable(void);
  /** Toggles the output generation for an ouput instances.
      @param idx ID of the output instance which output generation will be
                 toggled.
      @result false if the instance does not exist otherwise returns the status
              of the output generation (i.e. true if the output has been
              enabled, false if the output has been disabled) */
  bool Toggle(int idx);
  /** Modifies the output rate for all output instances.
      @param rate new output rate in Hz */
  void SetRate(double rate);
  /** Load the output directives and adds a new output instance to the Output
      Manager list.
      @param el XMLElement that is pointing to the output directives
      @result true if the execution succeeded. */
  bool Load(Element* el);
  /** Load the output directives and adds a new output instance to the Output
      Manager list. Unlike the Load() method, the new output instance is not
      generated from output directives read in a XML file but from a list of
      parameters.
      @param subSystems bitfield that describes the activated subsystems
      @param protocol network protocol for outputs directed to sockets
      @param type type of output
      @param port port to which the socket will be directed
      @param name file name to which the output will be directed
      @param outRate output rate in Hz
      @param outputProperties list of properties that should be output
      @result true if the execution succeeded. */
  bool Load(int subSystems, std::string protocol, std::string type,
            std::string port, std::string name, double outRate,
            std::vector<FGPropertyManager *> & outputProperties);
  /** Get the name identifier to which the output will be directed.
      @param idx ID of the output instance from which the name identifier must
                 be obtained
      @result the name identifier.*/
  string GetOutputName(unsigned int idx) const;

private:
  vector<FGOutputType*> OutputTypes;

  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

