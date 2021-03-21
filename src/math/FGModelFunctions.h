/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGModelFunctions.h
Author: Jon Berndt
Date started: August 2010

 ------------- Copyright (C) 2010  Jon S. Berndt (jon@jsbsim.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMODELFUNCTIONS_H
#define FGMODELFUNCTIONS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <memory>

#include "FGJSBBase.h"
#include "input_output/FGPropertyReader.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFunction;
class Element;
class FGPropertyManager;
class FGFDMExec;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** The model functions class provides the capability for loading, storing, and
    executing arbitrary functions.
    For certain classes, such as the engine, aerodynamics, ground reactions,
    mass balance, etc., it can be useful to incorporate special functions that
    can operate on the local model parameters before and/or after the model
    executes. For example, there is no inherent chamber pressure calculation
    done in the rocket engine model. However, an arbitrary function can be added
    to a specific rocket engine XML configuration file. It would be tagged with
    a "pre" or "post" type attribute to denote whether the function is to be
    executed before or after the standard model algorithm.
    @author Jon Berndt
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGModelFunctions
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGModelFunctions : public FGJSBBase
{
public:
  void RunPreFunctions(void);
  void RunPostFunctions(void);
  bool Load(Element* el, FGFDMExec* fdmex, std::string prefix="");
  void PreLoad(Element* el, FGFDMExec* fdmex, std::string prefix="");
  void PostLoad(Element* el, FGFDMExec* fdmex, std::string prefix="");

  /** Gets the strings for the current set of functions.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the descriptive names for all functions */
  std::string GetFunctionStrings(const std::string& delimeter) const;

  /** Gets the function values.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the numeric values for the current set of
      functions */
  std::string GetFunctionValues(const std::string& delimeter) const;

  /** Get one of the "pre" function
      @param name the name of the requested function.
      @return a pointer to the function (NULL if not found)
   */
  std::shared_ptr<FGFunction> GetPreFunction(const std::string& name);

protected:
  std::vector <std::shared_ptr<FGFunction>> PreFunctions;
  std::vector <std::shared_ptr<FGFunction>> PostFunctions;
  FGPropertyReader LocalProperties;

  virtual bool InitModel(void);
};

} // namespace JSBSim

#endif
