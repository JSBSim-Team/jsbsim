/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

  The order of rotations used in this class corresponds to a 3-2-1 sequence,
  or Y-P-R, or Z-Y-X, if you prefer.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGROTATION_H
#define FGROTATION_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifdef FGFS
#  include <simgear/compiler.h>
#  include <math.h>
#else
#  if defined (sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
#endif

#include "FGModel.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

#define ID_ROTATION "$Id: FGRotation.h,v 1.34 2001/12/22 15:21:58 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGRotation : public FGModel
{
public:
  FGRotation(FGFDMExec*);
  ~FGRotation();

  bool Run(void);

  inline FGColumnVector3& GetPQR(void) {return vPQR;}
  inline double GetPQR(int axis) {return vPQR(axis);}
  inline FGColumnVector3& GetAeroPQR(void) {return vAeroPQR;}
  inline double GetAeroPQR(int axis) {return vAeroPQR(axis);}
  inline FGColumnVector3& GetPQRdot(void) {return vPQRdot;}
  inline double GetPQRdot(int idx) {return vPQRdot(idx);}
  inline FGColumnVector3& GetEuler(void) {return vEuler;}
  inline double GetEuler(int axis) {return vEuler(axis);}
  inline FGColumnVector3& GetEulerRates(void) { return vEulerRates; }
  inline double GetEulerRates(int axis) { return vEulerRates(axis); }
  inline void SetPQR(FGColumnVector3 tt) {vPQR = tt;}
  inline void SetPQR(double p, double q, double r) { vPQR(1)=p; vPQR(2)=q; vPQR(3)=r; };
  inline void SetEuler(FGColumnVector3 tt) {vEuler = tt;}
  
  inline double Getphi(void) {return vEuler(1);}
  inline double Gettht(void) {return vEuler(2);}
  inline double Getpsi(void) {return vEuler(3);}
  
  inline double GetCosphi(void) {return cPhi;}
  inline double GetCostht(void) {return cTht;}
  inline double GetCospsi(void) {return cPsi;}

  inline double GetSinphi(void) {return sPhi;}
  inline double GetSintht(void) {return sTht;}
  inline double GetSinpsi(void) {return sPsi;}

private:
  FGColumnVector3 vPQR;
  FGColumnVector3 vAeroPQR;
  FGColumnVector3 vPQRdot;
  FGColumnVector3 vMoments;
  FGColumnVector3 vEuler;
  FGColumnVector3 vEulerRates;
  FGColumnVector3 vlastPQRdot;
  
  double cTht,sTht;
  double cPhi,sPhi;
  double cPsi,sPsi;
  
  double Ixx, Iyy, Izz, Ixz;
  double dt;

  void GetState(void);

  void Debug(int from);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

