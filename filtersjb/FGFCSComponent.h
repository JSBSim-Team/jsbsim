
/*******************************************************************************

 Header:       FGFCSComponent.h
 Author:       
 Date started: 

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

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGFCSCOMPONENT_H
#define FGFCSCOMPONENT_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  include STL_STRING
   FG_USING_STD(string);
#else
#  include <string>
#endif

/*******************************************************************************
DEFINES
*******************************************************************************/

class FGFCS;

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFCSComponent
{
private:

protected:
  FGFCS* fcs;
  string Type;
  string Name;
  enum {itPilotAC, itFCS, itAP} InputType; // Pilot/Aircraft, FCS, Autopilot inputs
  int ID;
  int QueueOrder;
  int InputIdx;
  float Input;
  int OutputIdx;
  float Output;
  bool IsOutput;

public:
  FGFCSComponent(FGFCS*);
  ~FGFCSComponent ( ) { }       //Destructor

  virtual bool Run (void);
  inline float GetOutput (void) {return Output;}
  inline string GetName(void) {return Name;}
  void SetOutput(void);
};

#include "FGFCS.h"

#endif

