/*******************************************************************************

 Header:       FGRotation.h
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

#ifndef FGROTATION_H
#define FGROTATION_H

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

class FGRotation : public FGModel
{
public:
  FGRotation(FGFDMExec*);
  ~FGRotation(void);

  bool Run(void);

  inline float GetP(void) {return P;}
  inline float GetQ(void) {return Q;}
  inline float GetR(void) {return R;}

  inline float GetPdot(void) {return Pdot;}
  inline float GetQdot(void) {return Qdot;}
  inline float GetRdot(void) {return Rdot;}

  inline float Getphi(void) {return phi;}
  inline float Gettht(void) {return tht;}
  inline float Getpsi(void) {return psi;}

  inline float GetQ0(void) {return Q0;}
  inline float GetQ1(void) {return Q1;}
  inline float GetQ2(void) {return Q2;}
  inline float GetQ3(void) {return Q3;}

  inline void SetP(float tt) {P = tt;}
  inline void SetQ(float tt) {Q = tt;}
  inline void SetR(float tt) {R = tt;}

  inline void SetPQR(float t1, float t2, float t3) {P=t1;
                                                    Q=t2;
                                                    R=t3;}

  inline void Setphi(float tt) {phi = tt;}
  inline void Settht(float tt) {tht = tt;}
  inline void Setpsi(float tt) {psi = tt;}

  inline void SetEuler(float t1, float t2, float t3) {phi=t1;
                                                      tht=t2;
                                                      psi=t3;}

  inline void SetQ0123(float t1, float t2, float t3, float t4) {Q0=t1;
                                                                Q1=t2;
                                                                Q2=t3;
                                                                Q3=t4;}

protected:

private:
  float P, Q, R;
  float L, M, N;
  float Ixx, Iyy, Izz, Ixz;
  float Q0, Q1, Q2, Q3;
  float phi, tht, psi;
  float Pdot, Qdot, Rdot;
  float Q0dot, Q1dot, Q2dot, Q3dot;
  float lastPdot, lastQdot, lastRdot;
  float lastQ0dot, lastQ1dot, lastQ2dot, lastQ3dot;
  float dt;
  float T[4][4];

  void GetState(void);
  void PutState(void);
};

/******************************************************************************/
#endif
