/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Module:       FGPosition.cpp
 Author:       Jon S. Berndt
 Date started: 01/05/99
 Purpose:      Integrate the EOM to determine instantaneous position
 Called by:    FGFDMExec
 
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
This class encapsulates the integration of rates and accelerations to get the
current position of the aircraft.

HISTORY
--------------------------------------------------------------------------------
01/05/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
[1] Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
    Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
    School, January 1994
[2] D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
    JSC 12960, July 1977
[3] Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
    NASA-Ames", NASA CR-2497, January 1975
[4] Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
    Wiley & Sons, 1979 ISBN 0-471-03032-5
[5] Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
    1982 ISBN 0-471-08936-2

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <cmath>
#    include <iomanip>
#  else
#    include <math.h>
#    include <iomanip.h>
#  endif
#else
#  if defined(sgi) && !defined(_GNUC_)
#    include <math.h>
#    include <iomanip.h>
#  else
#    include <cmath>
#    include <iomanip>
#  endif
#endif

#include "FGPosition.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGMassBalance.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

static const char *IdSrc = "$Id: FGPosition.cpp,v 1.39 2001/07/29 22:15:18 jberndt Exp $";
static const char *IdHdr = ID_POSITION;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

extern float globalTriNormal[3];
extern double globalSceneryAltitude;
extern double globalSeaLevelRadius;

FGPosition::FGPosition(FGFDMExec* fdmex) : FGModel(fdmex),
    vVel(3),
    vVelDot(3),
    vRunwayNormal(3)
{
  Name = "FGPosition";
  LongitudeDot = LatitudeDot = RadiusDot = 0.0;
  lastLongitudeDot = lastLatitudeDot = lastRadiusDot = 0.0;
  Longitude = Latitude = 0.0;
  gamma = Vt = Vground = 0.0;
  h = 3.0;                                 // Est. height of aircraft cg off runway
  SeaLevelRadius = EARTHRAD;               // For initialization ONLY
  Radius         = SeaLevelRadius + h;
  RunwayRadius   = SeaLevelRadius;
  DistanceAGL    = Radius - RunwayRadius;  // Geocentric
  vRunwayNormal(3) = -1.0;                 // Initialized for standalone mode
  b =1;
  

  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGPosition::~FGPosition()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGPosition" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/*
Purpose: Called on a schedule to perform Positioning algorithms
Notes:   [TP] Make sure that -Vt <= hdot <= Vt, which, of course, should always
         be the case
         [JB] Run in standalone mode, SeaLevelRadius will be EARTHRAD. In FGFS,
         SeaLevelRadius is stuffed from FGJSBSim in JSBSim.cxx each pass.
*/

bool FGPosition:: Run(void) {
  double cosLat;
  double hdot_Vt;

  if (!FGModel::Run()) {
    GetState();

    Vground = sqrt( vVel(eNorth)*vVel(eNorth) + vVel(eEast)*vVel(eEast) );
    psigt =  atan2(vVel(eEast), vVel(eNorth));
    if(psigt < 0.0)
      psigt += 2*M_PI;

    invMass   = 1.0 / MassBalance->GetMass();
    Radius    = h + SeaLevelRadius;
    invRadius = 1.0 / Radius;

    cosLat = cos(Latitude);
    if (cosLat != 0) LongitudeDot = vVel(eEast) / (Radius * cosLat);

    LatitudeDot = vVel(eNorth) * invRadius;
    RadiusDot   = -vVel(eDown);

    Longitude += 0.5*dt*rate*(LongitudeDot + lastLongitudeDot);
    Latitude  += 0.5*dt*rate*(LatitudeDot + lastLatitudeDot);
    Radius    += 0.5*dt*rate*(RadiusDot + lastRadiusDot);

    h = Radius - SeaLevelRadius;           // Geocentric

    DistanceAGL = Radius - RunwayRadius;   // Geocentric
    
    hoverb = DistanceAGL/b;

    if (Vt > 0) {
      hdot_Vt = RadiusDot/Vt;
      if (fabs(hdot_Vt) <= 1) gamma = asin(hdot_Vt);
    } else {
      gamma = 0.0;
    }

    lastLatitudeDot  = LatitudeDot;
    lastLongitudeDot = LongitudeDot;
    lastRadiusDot    = RadiusDot;

    return false;

  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::GetState(void) {
  dt = State->Getdt();

  Vt        = Translation->GetVt();
  vVel      = State->GetTb2l() * Translation->GetUVW();
  vVelDot   = State->GetTb2l() * Translation->GetUVWdot();
  
  b = Aircraft->GetWingSpan();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::Seth(double tt) {
 h = tt;
 Radius    = h + SeaLevelRadius;
 DistanceAGL = Radius - RunwayRadius;   // Geocentric
 hoverb = DistanceAGL/b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::SetDistanceAGL(double tt) {
  DistanceAGL=tt;
  Radius = RunwayRadius + DistanceAGL;
  h = Radius - SeaLevelRadius;
  hoverb = DistanceAGL/b;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGPosition::Debug(void)
{
    //TODO: Add your source code here
}

