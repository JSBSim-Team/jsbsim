/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Header:       FGScript.h
 Author:       Jon Berndt
 Date started: 12/21/2001

 ------------- Copyright (C) 2001  Jon S. Berndt (jsb@hal-pc.org) -------------

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
12/21/01   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGSCRIPT_HEADER_H
#define FGSCRIPT_HEADER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGJSBBase.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include <vector>

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FGSCRIPT "$Id: FGScript.h,v 1.1 2001/12/22 00:12:39 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Encapsulates the JSBSim scripting capability.
    @author Jon S. Berndt
    @version $Id: FGScript.h,v 1.1 2001/12/22 00:12:39 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGScript : public FGJSBBase
{
public:
  /// Default constructor
  FGScript(FGFDMExec* exec);

  /// Default destructor
  ~FGScript();

  /** Loads a script to drive JSBSim (usually in standalone mode).
      The language is the Simple Script Directives for JSBSim (SSDJ).
      @param script the filename (including path name, if any) for the script.
      @return true if successful */
  bool LoadScript(string script);

  /** This function is called each pass through the executive Run() method IF
      scripting is enabled. */
  void RunScript(void);

private:
  enum eAction {
    FG_RAMP  = 1,
    FG_STEP  = 2,
    FG_EXP   = 3
  };

  enum eType {
    FG_VALUE = 1,
    FG_DELTA = 2,
    FG_BOOL  = 3
  };

  struct condition {
    vector <eParam>  TestParam;
    vector <eParam>  SetParam;
    vector <double>  TestValue;
    vector <double>  SetValue;
    vector <string>  Comparison;
    vector <double>  TC;
    vector <bool>    Persistent;
    vector <eAction> Action;
    vector <eType>   Type;
    vector <bool>    Triggered;
    vector <double>  newValue;
    vector <double>  OriginalValue;
    vector <double>  StartTime;
    vector <double>  EndTime;

    condition() {
    }
  };

  bool Scripted;

  string  ScriptName;
  double  StartTime;
  double  EndTime;
  vector <struct condition> Conditions;

  FGFDMExec* FDMExec;
  FGState* State;
  void Debug(int from);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

