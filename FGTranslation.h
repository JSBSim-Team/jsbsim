/*******************************************************************************

 Header:       FGTranslation.h
 Author:       Jon Berndt
 Date started: 12/02/98

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
12/02/98   JSB   Created

********************************************************************************
COMMENTS, REFERENCES,  and NOTES
********************************************************************************
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

  The order of rotations used in this class corresponds to a 3-2-1 sequence,
  or Y-P-R, or Z-Y-X, if you prefer.

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGTRANSLATION_H
#define FGTRANSLATION_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  include <cmath>
#endif

#include "FGModel.h"

using namespace std;

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGTranslation : public FGModel
{
public:
   FGTranslation(FGFDMExec*);
   ~FGTranslation(void);

   inline float GetU(void) {return U;}
   inline float GetV(void) {return V;}
   inline float GetW(void) {return W;}

   inline float GetUdot(void) {return Udot;}
   inline float GetVdot(void) {return Vdot;}
   inline float GetWdot(void) {return Wdot;}

   inline float Getalpha(void) {return alpha;}
   inline float Getbeta (void) {return beta; }
   inline float Getgamma(void) {return gamma;}

   inline void SetU(float tt) {U = tt;}
   inline void SetV(float tt) {V = tt;}
   inline void SetW(float tt) {W = tt;}

   inline void SetUVW(float t1, float t2, float t3) {U=t1; V=t2; W=t3;}

   inline void Setalpha(float tt) {alpha = tt;}
   inline void Setbeta (float tt) {beta  = tt;}
   inline void Setgamma(float tt) {gamma = tt;}

   inline void SetABG(float t1, float t2, float t3) {alpha=t1; beta=t2; gamma=t3;}

   bool Run(void);

protected:

private:
  float U, V, W;                 // Body frame velocities owned by FGTranslation
  float P, Q, R;
  float Vt, qbar, mach;
  float Udot, Vdot, Wdot;
  float lastUdot, lastVdot, lastWdot;
  float phi, tht, psi;
  float Fx, Fy, Fz;
  float Mass, dt;
  float alpha, beta, gamma;
  float rho;

  void GetState(void);
  void PutState(void);
};

/******************************************************************************/
#endif
