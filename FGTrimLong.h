/*******************************************************************************

 Header:       FGTrimLong.h
 Author:       Tony Peden
 Date started: 7/1/99

 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------

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
9/8/99   TP   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

This class takes the given set of IC's and finds the angle of attack, elevator,
and throttle setting required to fly steady level. This is currently for in-air
conditions only.  It is implemented using an iterative, one-axis-at-a-time 
scheme.  



********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGTRIMLONG_H
#define FGTRIMLONG_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFDMExec.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGTrimLong.h"

#define ELEV_MIN -90
#define ELEV_MAX 90

#define THROTTLE_MIN 0
#define THROTTLE_MAX 1

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/


class FGTrimLong
{
private:
    typedef float (FGTrimLong::*trimfp)(float);
    int Ncycles,Naxis,Debug;
    float Tolerance, A_Tolerance;
    float Alpha_Gain, Elev_Gain, Throttle_Gain;
    float alphaMin, alphaMax;
    float wdot,udot,qdot;

    trimfp udotf,wdotf,qdotf;
    FGFDMExec* fdmex;
    FGInitialCondition* fgic;

    void setThrottlesPct(float tt);
    bool checkLimits(trimfp fp,float min, float max);
    // returns false if no sign change in fp(min)*fp(max) => no solution
    bool solve(trimfp fp,float guess,float desired,float *result,float eps,int max_iterations);
    bool findInterval(trimfp fp, float *lo, float *hi,float guess,float desired,int max_iterations );

    float udot_func(float x);
    float wdot_func(float x);
    float qdot_func(float x);

public:
    FGTrimLong(FGFDMExec *FDMExec, FGInitialCondition *FGIC);
    ~FGTrimLong(void);

    bool DoTrim(void);

    inline void SetMaxCycles(int ii) { Ncycles = ii; }
    inline void SetMaxCyclesPerAxis(int ii) { Naxis = ii; }
    inline void SetTolerance(float tt) {Tolerance = tt; A_Tolerance = tt / 10; }
    //Debug level 1 shows results of each top-level iteration
    //Debug level 2 shows level 1 & results of each per-axis iteration
    inline void SetDebug(int level) { Debug = level; }
    inline void ClearDebug(void) { Debug = 0;}

};


#endif		









