/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFDMExec.h
 Author:       Jon Berndt
 Date started: 11/17/98

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

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
11/17/98   JSB   Created
7/31/99     TP   Added RunIC function that runs the sim so that every frame
                 begins with the IC values from the given FGInitialCondition 
	  	  	  	   object and dt=0. 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFDMEXEC_HEADER_H
#define FGFDMEXEC_HEADER_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FDMEXEC "$Id: FGFDMExec.h,v 1.19 2001/02/25 00:56:58 jberndt Exp $"

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <vector>
#  else
#    include <vector.h>
#  endif
#else
#  include <vector>
#endif

#include "FGModel.h"
#include "FGInitialCondition.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

struct condition {
  vector <eParam>  TestParam;
  vector <eParam>  SetParam;
  vector <float>   TestValue;
  vector <float>   SetValue;
  vector <string>  Comparison;
  vector <float>   TC;
  vector <int>     Repeat;
  vector <eAction> Action;
  vector <eType>   Type;
};

class FGState;
class FGAtmosphere;
class FGFCS;
class FGPropulsion;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPosition;
class FGAuxiliary;
class FGOutput;
class FGInitialCondition;

class FGFDMExec
{
public:
  FGFDMExec(void);
  ~FGFDMExec(void);

  FGModel* FirstModel;

  bool Initialize(void);
  int  Schedule(FGModel* model, int rate);
  bool Run(void);
  bool RunIC(FGInitialCondition *fgic);
  void Freeze(void) {frozen = true;}
  void Resume(void) {frozen = false;}

  bool SetEnginePath(string path)   {EnginePath = path; return true;}
  bool SetAircraftPath(string path) {AircraftPath = path; return true;}
  bool SetScriptPath(string path)   {ScriptPath = path; return true;}

  bool LoadModel(string AircraftPath, string EnginePath, string model);
  bool LoadScript(string script);
  bool RunScript(void);

  inline FGState* GetState(void)              {return State;}
  inline FGAtmosphere* GetAtmosphere(void)    {return Atmosphere;}
  inline FGFCS* GetFCS(void)                  {return FCS;}
  inline FGPropulsion* GetPropulsion(void)    {return Propulsion;}
  inline FGAircraft* GetAircraft(void)        {return Aircraft;}
  inline FGTranslation* GetTranslation(void)  {return Translation;}
  inline FGRotation* GetRotation(void)        {return Rotation;}
  inline FGPosition* GetPosition(void)        {return Position;}
  inline FGAuxiliary* GetAuxiliary(void)      {return Auxiliary;}
  inline FGOutput* GetOutput(void)            {return Output;}
  
  inline string GetEnginePath(void)          {return EnginePath;}
  inline string GetAircraftPath(void)        {return AircraftPath;}

private:
  bool frozen;
  bool terminate;
  int  Error;
  bool modelLoaded;

  string AircraftPath;
  string EnginePath;
  string ScriptPath;
  string ScriptName;
  float  StartTime;
  float  EndTime;
  vector <struct condition> Conditions;

  FGState*       State;
  FGAtmosphere*  Atmosphere;
  FGFCS*         FCS;
  FGPropulsion*  Propulsion;
  FGAircraft*    Aircraft;
  FGTranslation* Translation;
  FGRotation*    Rotation;
  FGPosition*    Position;
  FGAuxiliary*   Auxiliary;
  FGOutput*      Output;
  
  bool Allocate(void);
  bool DeAllocate(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
