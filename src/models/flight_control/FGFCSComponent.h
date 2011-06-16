/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFCSComponent.h
 Author:       Jon S. Berndt
 Date started: 05/01/2000

 ------------- Copyright (C)  -------------

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

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFCSCOMPONENT_H
#define FGFCSCOMPONENT_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include "math/FGPropertyValue.h"
#include <string>
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FCSCOMPONENT "$Id: FGFCSComponent.h,v 1.20 2011/06/16 03:39:38 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;
class FGPropertyManager;
class Element;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for JSBSim Flight Control System Components.
    The Flight Control System (FCS) for JSBSim consists of the FCS container
    class (see FGFCS), the FGFCSComponent base class, and the
    component classes from which can be constructed a string, or channel. See:

    - FGSwitch
    - FGGain
    - FGKinemat
    - FGFilter
    - FGDeadBand
    - FGSummer
    - FGSensor
    - FGFCSFunction
    - FGPID
    - FGAccelerometer
    - FGGyro
    - FGActuator

    @author Jon S. Berndt
    @version $Id: FGFCSComponent.h,v 1.20 2011/06/16 03:39:38 jberndt Exp $
    @see Documentation for the FGFCS class, and for the configuration file class
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFCSComponent : public FGJSBBase
{
public:
  /// Constructor
  FGFCSComponent(FGFCS* fcs, Element* el);
  /// Destructor
  virtual ~FGFCSComponent();

  virtual bool Run(void);
  virtual void SetOutput(void);
  double GetOutput (void) const {return Output;}
  std::string GetName(void) const {return Name;}
  std::string GetType(void) const { return Type; }
  virtual double GetOutputPct(void) const { return 0; }

protected:
  FGFCS* fcs;
  FGPropertyManager* PropertyManager;
  FGPropertyManager* treenode;
  std::vector <FGPropertyManager*> OutputNodes;
  FGPropertyManager* ClipMinPropertyNode;
  FGPropertyManager* ClipMaxPropertyNode;
  std::vector <FGPropertyValue*> InputNodes;
  std::vector <std::string> InputNames;
  std::vector <float> InputSigns;
  std::vector <double> output_array;
  std::string Type;
  std::string Name;
  double Input;
  double Output;
  double clipmax, clipmin;
  double delay_time;
  unsigned int delay;
  int index;
  float clipMinSign, clipMaxSign;
  double dt;
  bool IsOutput;
  bool clip;

  void Delay(void);
  void Clip(void);
  virtual void bind();
  virtual void Debug(int from);
};

} //namespace JSBSim

#include "../FGFCS.h"

#endif

