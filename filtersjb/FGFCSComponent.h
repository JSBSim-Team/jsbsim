/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFCSComponent.h
 Author:       Jon S. Berndt
 Date started: 05/01/2000

 ------------- Copyright (C)  -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
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

#ifdef FGFS
#  include <simgear/compiler.h>
#endif

#include <string>
#include <vector>
#include "../FGJSBBase.h"
#include "../FGPropertyManager.h"


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FCSCOMPONENT "$Id: FGFCSComponent.h,v 1.33 2002/09/22 18:15:11 apeden Exp $"

using std::string;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Base class for JSBSim Flight Control System Components.
    The Flight Control System (FCS) for JSBSim consists of the FCS container
    class (see \URL[FGFCS]{FGFCS.html}), the FGFCSComponent base class, and the
    component classes from which can be constructed a string, or channel. See:
    <ul>
    <li>\URL[Switch Component]{FGSwitch.html}</li>
    <li>\URL[Gain Component]{FGGain.html}</li>
    <li>\URL[Flaps Component]{FGFlaps.html}</li>
    <li>\URL[Filter Component]{FGFilter.html}</li>
    <li>\URL[Deadband Component]{FGDeadBand.html}</li>
    <li>\URL[Summer Component]{FGSummer.html}</li>
    <li>\URL[Gradient Component]{FGGradient.html}</li>
    </ul>
    @author Jon S. Berndt
    @version $Id: FGFCSComponent.h,v 1.33 2002/09/22 18:15:11 apeden Exp $
    @see Documentation for the FGFCS class, and for the configuration file class
         FGConfigFile.
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGFCSComponent : public FGJSBBase
{
public:
  /// Constructor
  FGFCSComponent(FGFCS*);
  /// Destructor
  virtual ~FGFCSComponent();

  virtual bool Run(void);
  virtual void SetOutput(void);
  inline double GetOutput (void) const {return Output;}
  inline FGPropertyManager* GetOutputNode(void) { return OutputNode; }
  inline string GetName(void) const {return Name;}
  inline string GetType(void) const { return Type; }
  virtual double GetOutputPct(void) const { return 0; }
  
  virtual void bind(FGPropertyManager *node);
  
  FGPropertyManager* resolveSymbol(string token);
  
protected:
   /// Pilot/Aircraft, FCS, Autopilot inputs
  enum eInputType {itPilotAC, itFCS, itAP, itBias} InputType;
  FGFCS* fcs;
  FGPropertyManager* PropertyManager;
  string Type;
  string Name;
  int ID;
  vector<FGPropertyManager*> InputNodes;
  int InputIdx;
  double Input;
  FGPropertyManager* OutputNode;
  double Output;
  bool IsOutput;
  virtual void Debug(int from);
};

#include "../FGFCS.h"

#endif

