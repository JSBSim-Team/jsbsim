/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGModelFunctions.h
Author: Jon Berndt
Date started: August 2010

 ------------- Copyright (C) 2010  Jon S. Berndt (jon@jsbsim.org) -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGMODELFUNCTIONS_H
#define FGMODELFUNCTIONS_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include <vector>
#include "math/FGFunction.h"
#include "input_output/FGPropertyManager.h"
#include "input_output/FGXMLElement.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MODELFUNCTIONS "$Id: FGModelFunctions.h,v 1.5 2012/04/13 13:25:52 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

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
  virtual ~FGModelFunctions();
  void RunPreFunctions(void);
  void RunPostFunctions(void);
  bool Load(Element* el, FGPropertyManager* PropertyManager, std::string prefix="");
  void PreLoad(Element* el, FGPropertyManager* PropertyManager, std::string prefix="");
  void PostLoad(Element* el, FGPropertyManager* PropertyManager, std::string prefix="");

  /** Gets the strings for the current set of functions.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the descriptive names for all functions */
  std::string GetFunctionStrings(const std::string& delimeter) const;

  /** Gets the function values.
      @param delimeter either a tab or comma string depending on output type
      @return a string containing the numeric values for the current set of
      functions */
  std::string GetFunctionValues(const std::string& delimeter) const;

protected:
  std::vector <FGFunction*> PreFunctions;
  std::vector <FGFunction*> PostFunctions;
  std::vector <double*> interface_properties;
};

} // namespace JSBSim

#endif
