/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGInputType.h
 Author:       Paul Chavent
 Date started: 01/20/15

 ------------- Copyright (C) 2015 Paul Chavent -------------

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
01/20/15   PC    Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGINPUTTYPE_H
#define FGINPUTTYPE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "models/FGModel.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Abstract class to provide functions generic to all the input directives.
    This class is used by the input manager FGInput to manage a list of
    different input classes without needing to know the details of each one of
    them. It also provides the functions that are common to all the input
    classes.

    The class inherits from FGModelFunctions so it is possible to define
    functions that execute before or after the input is generated. Such
    functions need to be tagged with a "pre" or "post" type attribute to denote
    the sequence in which they should be executed.

    The class mimics some functionalities of FGModel (methods InitModel(),
    Run() and SetRate()). However it does not inherit from FGModel since it is
    conceptually different from the model paradigm.
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGInputType : public FGModel
{
public:
  /** Constructor (implement the FGModel interface).
      @param fdmex a pointer to the parent executive object
   */
  FGInputType(FGFDMExec* fdmex);

  /// Destructor
  ~FGInputType() override;

  /** Set the idx for this input instance
      @param idx ID of the input instance that is constructed
   */
  void SetIdx(unsigned int idx);

  /** Init the input directives from an XML file (implement the FGModel interface).
      @param element XML Element that is pointing to the input directives
  */
  bool Load(Element* el) override;

  /// Init the input model according to its configitation.
  bool InitModel(void) override;

  /** Executes the input directives (implement the FGModel interface).
      This method checks that the current time step matches the input
      rate and calls the registered "pre" functions, the input
      generation and finally the "post" functions.
      @result false if no error.
   */
  bool Run(bool Holding) override;

  /** Generate the input. This is a pure method so it must be implemented by
      the classes that inherits from FGInputType. The Read name may not be
      relevant to all inputs but it has been kept for backward compatibility.
   */
  virtual void Read(bool Holding) = 0;

  /// Enables the input generation.
  void Enable(void) { enabled = true; }
  /// Disables the input generation.
  void Disable(void) { enabled = false; }
  /** Toggles the input generation.
      @result the input generation status i.e. true if the input has been
              enabled, false if the input has been disabled. */
  bool Toggle(void) {enabled = !enabled; return enabled;}

  /** Overwrites the name identifier under which the input will be read.
      This method is taken into account if it is called before
      FGFDMExec::RunIC() otherwise it is ignored until the next call to
      SetStartNewInput().
      @param name new name */
  virtual void SetInputName(const std::string& name) { Name = name; }

  /** Get the name identifier to which the input will be directed.
      @result the name identifier.*/
  virtual const std::string& GetInputName(void) const { return Name; }

protected:
  unsigned int InputIdx;
  bool enabled;

  void Debug(int from) override;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
