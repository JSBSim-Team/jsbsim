/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGInput.h
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
19/01/15   PCH   Split the code in several classes (see input_output dir)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGINPUT_H
#define FGINPUT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGModel.h"
#include "input_output/FGInputType.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Handles simulation input.
    INPUT section definition

    The following specifies the way that JSBSim writes out data.
<pre>
    NAME is the filename you want the input to come from

    TYPE can be:
      SOCKET      Will eventually send data to a socket input, where NAME
                  would then be the IP address of the machine the data should
                  be sent to. DON'T USE THIS YET!
      NONE        Specifies to do nothing. This setting makes it easy to turn on and
                  off the data input without having to mess with anything else.

      Examples:
</pre>
@code
<input type="SOCKET" port="4321"/>
@endcode
<br>

    The class FGInput is the manager of the inputs requested by the user. It
    manages a list of instances derived from the abstract class FGInputType.
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGInput : public FGModel
{
public:
  FGInput(FGFDMExec*);
  ~FGInput() override;

  /** Load the input directives and adds a new input instance to the Input
      Manager list.
      @param el XMLElement that is pointing to the input directives
      @result true if the execution succeeded. */
  bool Load(Element* el) override;

  /** Initializes the instance. This method is called by FGFDMExec::RunIC().
      This is were the initialization of all classes derived from FGInputType
      takes place. It is important that this method is not called prior
      to FGFDMExec::RunIC() so that the initialization process can be executed
      properly.
      @result true if the execution succeeded. */
  bool InitModel(void) override;

  /** Runs the Input model; called by the Executive
      Can pass in a value indicating if the executive is directing the simulation to Hold.
      @param Holding if true, the executive has been directed to hold the sim from
                     advancing time. Some models may ignore this flag, such as the Input
                     model, which may need to be active to listen on a socket for the
                     "Resume" command to be given.
      @return false if no error */
  bool Run(bool Holding) override;

  /** Adds a new input instance to the Input Manager. The definition of the
      new input instance is read from a file.
      @param fname the name of the file from which the ouput directives should
                   be read.
      @return true if the execution succeeded. */
  bool SetDirectivesFile(const SGPath& fname);

  /// Enables the input generation for all input instances.
  void Enable(void) { enabled = true; }
  /// Disables the input generation for all input instances.
  void Disable(void) { enabled = false; }
  /** Toggles the input generation of each input instance.
      @param idx ID of the input instance which input generation will be
                 toggled.
      @result false if the instance does not exist otherwise returns the status
              of the input generation (i.e. true if the input has been
              enabled, false if the input has been disabled) */
  bool Toggle(int idx);

  /** Overwrites the name identifier under which the input will be logged.
      This method is taken into account if it is called between Load() and
      FGFDMExec::RunIC() otherwise it is ignored until the next call to
      SetStartNewInput().
      @param idx ID of the instance which name identifier will be changed
      @param name new name
      @result false if the instance does not exists. */
  bool SetInputName(unsigned int idx, const std::string& name);
  /** Get the name identifier to which the input will be directed.
      @param idx ID of the input instance from which the name identifier must
                 be obtained
      @result the name identifier.*/
  std::string GetInputName(unsigned int idx) const;

private:
  std::vector<FGInputType*> InputTypes;
  bool enabled;

  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
