/*******************************************************************************
 
 Header:       FGInitialCondition.h
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
7/1/99   TP   Created
 
 
FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
 
The purpose of this class is to take a set of initial conditions and provide
a kinematically consistent set of body axis velocity components, euler
angles, and altitude.  This class does not attempt to trim the model i.e.
the sim will most likely start in a very dynamic state (unless, of course,
you have chosen your IC's wisely) even after setting it up with this class.
 
********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGINITIALCONDITION_H
#define FGINITIALCONDITION_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGMatrix.h"

#define ID_INITIALCONDITION "$Header"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

typedef enum { setvt, setvc, setve, setmach, setuvw, setned } speedset;

#define jsbFPSTOKTS 0.5924838
#define jsbKTSTOFPS 1.6878099


/* USAGE NOTES
   With a valid object of FGFDMExec and an aircraft model loaded
   FGInitialCondition fgic=new FGInitialCondition(FDMExec);
   fgic->SetVcalibratedKtsIC()
   fgic->SetAltitudeFtIC();
   .
   .
   .
   //to directly into Run
   FDMExec->GetState()->Initialize(fgic)
   delete fgic;
   FDMExec->Run()
   
   //or to loop the sim w/o integrating
   FDMExec->RunIC(fgic)
   
   
   
   Speed:
	 Since vc, ve, vt, and mach all represent speed, the remaining
	 three are recalculated each time one of them is set (using the
	 current altitude).  The most recent speed set is remembered so 
	 that if and when altitude is reset, the last set speed is used 
	 to recalculate the remaining three. Setting any of the body 
	 components forces a recalculation of vt and vt then becomes the
	 most recent speed set.
   
   Alpha,Gamma, and Theta:
     This class assumes that it will be used to set up the sim for a
	 steady, zero pitch rate condition. Since any two of those angles 
   specifies the third gamma (flight path angle) is favored when setting
   alpha and theta and alpha is favored when setting gamma. i.e.
    	set alpha : recalculate theta using gamma as currently set
		  set theta : recalculate alpha using gamma as currently set
		  set gamma : recalculate theta using alpha as currently set
 
	 The idea being that gamma is most interesting to pilots (since it 
	 is indicative of climb rate). 
	 
	 Setting climb rate is, for the purpose of this discussion, 
	 considered equivalent to setting gamma.
 
*/
class FGInitialCondition {
public:

  FGInitialCondition(FGFDMExec *fdmex);
  ~FGInitialCondition(void);
  
  inline speedset GetSpeedSet(void) { return lastSpeedSet; }

  void SetVcalibratedKtsIC(float tt);
  void SetVequivalentKtsIC(float tt);
  void SetVtrueKtsIC(float tt);
  void SetMachIC(float tt);

  void SetUBodyFpsIC(float tt);
  void SetVBodyFpsIC(float tt);
  void SetWBodyFpsIC(float tt);
  
  void SetVnorthFpsIC(float tt);
  void SetVeastFpsIC(float tt);
  void SetVdownFpsIC(float tt);
  
  void SetAltitudeFtIC(float tt);
  void SetAltitudeAGLFtIC(float tt);
  
  void SetSeaLevelRadiusFtIC(double tt);
  void SetTerrainAltitudeFtIC(double tt);
  
  //"vertical" flight path, recalculate theta
  inline void SetFlightPathAngleDegIC(float tt) { SetFlightPathAngleRadIC(tt*DEGTORAD); }
  void SetFlightPathAngleRadIC(float tt);
  //set speed first
  void SetClimbRateFpmIC(float tt);
  void SetClimbRateFpsIC(float tt);
  //use currently stored gamma, recalcualte theta
  inline void SetAlphaDegIC(float tt)      { alpha=tt*DEGTORAD; getTheta(); }
  inline void SetAlphaRadIC(float tt)      { alpha=tt; getTheta(); }
  //use currently stored gamma, recalcualte alpha
  inline void SetPitchAngleDegIC(float tt) { theta=tt*DEGTORAD; getAlpha(); }
  inline void SetPitchAngleRadIC(float tt) { theta=tt; getAlpha(); }

  inline void SetBetaDegIC(float tt)       { beta=tt*DEGTORAD; getTheta();}
  inline void SetBetaRadIC(float tt)       { beta=tt; getTheta(); }
  
  inline void SetRollAngleDegIC(float tt) { phi=tt*DEGTORAD; getTheta(); }
  inline void SetRollAngleRadIC(float tt) { phi=tt; getTheta(); }

  inline void SetTrueHeadingDegIC(float tt)   { psi=tt*DEGTORAD; }
  inline void SetTrueHeadingRadIC(float tt)   { psi=tt; }

  inline void SetLatitudeDegIC(float tt)  { latitude=tt*DEGTORAD; }
  inline void SetLatitudeRadIC(float tt)  { latitude=tt; }

  inline void SetLongitudeDegIC(float tt) { longitude=tt*DEGTORAD; }
  inline void SetLongitudeRadIC(float tt) { longitude=tt; }

  inline float GetVcalibratedKtsIC(void) { return vc*jsbFPSTOKTS; }
  inline float GetVequivalentKtsIC(void) { return ve*jsbFPSTOKTS; }
  inline float GetVtrueKtsIC(void) { return vt*jsbFPSTOKTS; }
  inline float GetVtrueFpsIC(void) { return vt; }
  inline float GetMachIC(void) { return mach; }

  inline float GetAltitudeFtIC(void) { return altitude; }

  inline float GetFlightPathAngleDegIC(void) { return gamma*RADTODEG; }
  inline float GetFlightPathAngleRadIC(void) { return gamma; }

  inline float GetClimbRateFpmIC(void) { return hdot*60; }
  inline float GetClimbRateFpsIC(void) { return hdot; }

  inline float GetAlphaDegIC(void)      { return alpha*RADTODEG; }
  inline float GetAlphaRadIC(void)      { return alpha; }

  inline float GetPitchAngleDegIC(void) { return theta*RADTODEG; }
  inline float GetPitchAngleRadIC(void) { return theta; }


  inline float GetBetaDegIC(void)       { return beta*RADTODEG; }
  inline float GetBetaRadIC(void)       { return beta*RADTODEG; }

  inline float GetRollAngleDegIC(void) { return phi*RADTODEG; }
  inline float GetRollAngleRadIC(void) { return phi; }

  inline float GetHeadingDegIC(void)   { return psi*RADTODEG; }
  inline float GetHeadingRadIC(void)   { return psi; }

  inline float GetLatitudeDegIC(void)  { return latitude*RADTODEG; }
  inline float GetLatitudeRadIC(void) { return latitude; }

  inline float GetLongitudeDegIC(void) { return longitude*RADTODEG; }
  inline float GetLongitudeRadIC(void) { return longitude; }

  inline float GetUBodyFpsIC(void) { return vt*cos(alpha)*cos(beta); }
  inline float GetVBodyFpsIC(void) { return vt*sin(beta); }
  inline float GetWBodyFpsIC(void) { return vt*sin(alpha)*cos(beta); }

  inline float GetThetaRadIC(void) { return theta; }
  inline float GetPhiRadIC(void)   { return phi; }
  inline float GetPsiRadIC(void)   { return psi; }



private:
  float vt,vc,ve;
  float alpha,beta,gamma,theta,phi,psi;
  float mach;
  float altitude,hdot;
  float latitude,longitude;
  float u,v,w;
  float vnorth,veast,vdown;
  double sea_level_radius;
  double terrain_altitude;
  double radius_to_vehicle;
  
  float xlo, xhi,xmin,xmax;
  
  typedef float (FGInitialCondition::*fp)(float x);
  fp sfunc;

  speedset lastSpeedSet;

  FGFDMExec *fdmex;
  
  
  bool getAlpha(void);
  bool getTheta(void);
  bool getMachFromVcas(float *Mach,float vcas);
  
  float GammaEqOfTheta(float Theta);
  float GammaEqOfAlpha(float Alpha);
  float calcVcas(float Mach);
  void calcUVWfromNED(void);
  
  bool findInterval(float x,float guess);
  bool solve(float *y, float x);
};

#endif
