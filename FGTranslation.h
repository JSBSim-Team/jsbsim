/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
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
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGTRANSLATION_H
#define FGTRANSLATION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  ifdef SG_HAVE_STD_INCLUDES
#    include <cmath>
#  else
#    include <math.h>
#  endif
#else
#  if defined(sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#include "FGModel.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_TRANSLATION "$Id: FGTranslation.h,v 1.48 2004/01/13 17:35:06 dpculp Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models the translation aspects of the EOM.
    Note: The order of rotations used in this class corresponds to a 3-2-1 sequence,
    or Y-P-R, or Z-Y-X, if you prefer.
    @see Cooke, Zyda, Pratt, and McGhee, "NPSNET: Flight Simulation Dynamic Modeling
    Using Quaternions", Presence, Vol. 1, No. 4, pp. 404-420  Naval Postgraduate
    School, January 1994
    @see D. M. Henderson, "Euler Angles, Quaternions, and Transformation Matrices",
    JSC 12960, July 1977
    @see Richard E. McFarland, "A Standard Kinematic Model for Flight Simulation at
    NASA-Ames", NASA CR-2497, January 1975
    @see Barnes W. McCormick, "Aerodynamics, Aeronautics, and Flight Mechanics",
    Wiley & Sons, 1979 ISBN 0-471-03032-5
    @see Bernard Etkin, "Dynamics of Flight, Stability and Control", Wiley & Sons,
    1982 ISBN 0-471-08936-2
  */

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGTranslation : public FGModel {
public:
  FGTranslation(FGFDMExec*);
  ~FGTranslation();
  
  inline double           GetUVW   (int idx) const { return vUVW(idx); }
  inline FGColumnVector3& GetUVW   (void)    { return vUVW; }
  inline FGColumnVector3& GetUVWdot(void)    { return vUVWdot; }
  inline double           GetUVWdot(int idx) const { return vUVWdot(idx); }
  inline FGColumnVector3& GetAeroUVW (void)    { return vAeroUVW; }
  inline double           GetAeroUVW (int idx) const { return vAeroUVW(idx); }

  double Getalpha(void) const { return alpha; }
  double Getbeta (void) const { return beta; }
  inline double GetMagBeta(void) const { return fabs(beta); }
  double Getqbar (void) const { return qbar; }
  double GetqbarUW (void) const { return qbarUW; }
  double GetqbarUV (void) const { return qbarUV; }
  inline double GetVt   (void) const { return Vt; }
  double GetMach (void) const { return Mach; }
  double GetMachU(void) const { return vMachUVW(eU); }
  double Getadot (void) const { return adot; }
  double Getbdot (void) const { return bdot; }

  void SetUVW(FGColumnVector3 tt) { vUVW = tt; }
  void SetAeroUVW(FGColumnVector3 tt) { vAeroUVW = tt; }

  inline void Setalpha(double tt) { alpha = tt; }
  inline void Setbeta (double tt) { beta  = tt; }
  inline void Setqbar (double tt) { qbar = tt; }
  inline void SetqbarUW (double tt) { qbarUW = tt; }
  inline void SetqbarUV (double tt) { qbarUV = tt; }
  inline void SetVt   (double tt) { Vt = tt; }
  inline void SetMach (double tt) { Mach=tt; }
  inline void Setadot (double tt) { adot = tt; }
  inline void Setbdot (double tt) { bdot = tt; }

  inline void SetAB(double t1, double t2) { alpha=t1; beta=t2; }
  
  bool Run(void);

  void bind(void);
  void unbind(void);

private:
  FGColumnVector3 vUVW;
  FGColumnVector3 vUVWdot;
  FGColumnVector3 vUVWdot_prev[4];
  FGMatrix33      mVel;
  FGColumnVector3 vAeroUVW;
  FGColumnVector3 vMachUVW;

  double Vt, Mach;
  double qbar, qbarUW, qbarUV;
  double dt;
  double alpha, beta;
  double adot,bdot;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

