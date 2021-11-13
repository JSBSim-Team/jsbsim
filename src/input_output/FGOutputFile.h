/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGOutputFile.h
 Author:       Bertrand Coconnier
 Date started: 09/10/11

 ------------- Copyright (C) 2011 Bertrand Coconnier  -------------

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

#ifndef FGOUTPUTFILE_H
#define FGOUTPUTFILE_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFDMExec.h"
#include "FGOutputType.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Abstract class that provide functions that are generic to all the outputs
    that are directed to a file. A new class derived from FGOutputFile should
    be created for each file format that JSBSim is able to output.

    This class provides all the machinery necessary to manage the file naming
    including the sequence in which the file should be opened then closed. The
    logic of SetStartNewOutput() is also managed in this class. Derived class
    should normally not need to reimplement this method. In most cases, derived
    classes only need to implement the methods OpenFile(), CloseFile() and
    Print().
 */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGOutputFile : public FGOutputType
{
public:
  /// Constructor
  FGOutputFile(FGFDMExec* fdmex);

  /// Destructor : closes the file.
  ~FGOutputFile() override { CloseFile(); }

  /** Init the output directives from an XML file.
      @param element XML Element that is pointing to the output directives
  */
  bool Load(Element* el) override;

  /** Initializes the instance. This method basically opens the file to which
      outputs will be directed.
      @result true if the execution succeeded.
   */
  bool InitModel(void) override;
  /** Reset the output prior to a restart of the simulation. This method should
      be called when the simulation is restarted with, for example, new initial
      conditions. The current file is closed and reopened with a new name. The
      new name is contructed from the base file name set by the class
      constructor or SetOutputName() and is appended with an underscore _ and
      an ID that is incremented at each call to this method.
  */
  void SetStartNewOutput(void) override;
  /** Overwrites the name identifier under which the output will be logged.
      For this method to take effect, it must be called prior to
      FGFDMExec::RunIC(). If it is called after, it will not take effect before
      the next call to SetStartNewOutput().
      @param name new name */
  void SetOutputName(const std::string& fname) override {
    Name = (FDMExec->GetOutputPath()/fname).utf8Str();
    runID_postfix = -1;
    Filename = SGPath();
  }
  /** Generate the output. This is a pure method so it must be implemented by
      the classes that inherits from FGOutputFile.
   */
  void Print(void) override = 0;

protected:
  SGPath Filename;

  /// Opens the file
  virtual bool OpenFile(void) = 0;
  /// Closes the file
  virtual void CloseFile(void) {}

private:
  int runID_postfix;
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
