/*******************************************************************************
 
 Header:       FGEngine.h
 Author:       Jon S. Berndt
 Date started: 01/21/99
 
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
 
FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
 
Based on Flightgear code, which is based on LaRCSim. This class simulates
a generic engine.
 
HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGEngine_H
#define FGEngine_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include STL_STRING
FG_USING_STD(string);
#else
#  include <string>
#endif

/*******************************************************************************
DEFINES
*******************************************************************************/

using std::string;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFDMExec;
class FGState;
class FGAtmosphere;
class FGFCS;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPosition;
class FGAuxiliary;
class FGOutput;

class FGEngine {
public:
  FGEngine(FGFDMExec*, string, string, int);
  ~FGEngine(void);

  enum EngineType {etUnknown, etRocket, etPiston, etTurboProp, etTurboJet};

  float  GetThrottle(void) { return Throttle; }
  float  GetThrust(void) { return Thrust; }
  float  GetThrottleMin(void) { return MinThrottle; }
  float  GetThrottleMax(void) { return MaxThrottle; }
  bool   GetStarved(void) { return Starved; }
  bool   GetFlameout(void) { return Flameout; }
  int    GetType(void) { return Type; }
  string GetName() { return Name; }

  void SetStarved(bool tt) {
    Starved = tt;
  }
  void SetStarved(void) {
    Starved = true;
  }

  float CalcThrust(void);
  float CalcFuelNeed(void);
  float CalcOxidizerNeed(void);

private:
  string Name;
  EngineType Type;
  float X, Y, Z;
  float SLThrustMax;
  float VacThrustMax;
  float SLFuelFlowMax;
  float SLOxiFlowMax;
  float MaxThrottle;
  float MinThrottle;

  float BrakeHorsePower;
  float SpeedSlope;
  float SpeedIntercept;
  float AltitudeSlope;

  float Thrust;
  float Throttle;
  float FuelNeed, OxidizerNeed;
  bool  Starved;
  bool  Flameout;
  float PctPower;
  int   EngineNumber;

  FGFDMExec*      FDMExec;
  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGOutput*       Output;

protected:
  float CalcRocketThrust(void);
  float CalcPistonThrust(void);

};

/******************************************************************************/
#endif
