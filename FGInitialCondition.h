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

typedef enum { setvt, setvc, setve, setmach, setuvw, setned, setvg } speedset;

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

  void SetVcalibratedKtsIC(float tt);
  void SetVequivalentKtsIC(float tt);
  inline void SetVtrueKtsIC(float tt)   { SetVtrueFpsIC(tt*jsbKTSTOFPS);   }
  inline void SetVgroundKtsIC(float tt) { SetVgroundFpsIC(tt*jsbKTSTOFPS); }
  void SetMachIC(float tt);
  
  inline void SetAlphaDegIC(float tt)      { SetAlphaRadIC(tt*DEGTORAD); }
  inline void SetBetaDegIC(float tt)       { SetBetaRadIC(tt*DEGTORAD);}
  
  inline void SetPitchAngleDegIC(float tt) { SetPitchAngleRadIC(tt*DEGTORAD); }
  inline void SetRollAngleDegIC(float tt)  { SetRollAngleRadIC(tt*DEGTORAD);}
  inline void SetTrueHeadingDegIC(float tt){ SetTrueHeadingRadIC(tt*DEGTORAD); }
  
  void SetClimbRateFpmIC(float tt);
  inline void SetFlightPathAngleDegIC(float tt) { SetFlightPathAngleRadIC(tt*DEGTORAD); }

  void SetAltitudeFtIC(float tt);
  void SetAltitudeAGLFtIC(float tt);
  
  void SetSeaLevelRadiusFtIC(double tt);
  void SetTerrainAltitudeFtIC(double tt);

  inline void SetLatitudeDegIC(float tt)  { latitude=tt*DEGTORAD; }
  inline void SetLongitudeDegIC(float tt) { longitude=tt*DEGTORAD; }

  
  inline float GetVcalibratedKtsIC(void) { return vc*jsbFPSTOKTS; }
  inline float GetVequivalentKtsIC(void) { return ve*jsbFPSTOKTS; }
  inline float GetVgroundKtsIC(void) { return vg*jsbFPSTOKTS; }
  inline float GetVtrueKtsIC(void) { return vt*jsbFPSTOKTS; }
  inline float GetMachIC(void) { return mach; }
  
  inline float GetClimbRateFpmIC(void) { return hdot*60; }
  inline float GetFlightPathAngleDegIC(void) { return gamma*RADTODEG; }
  
  inline float GetAlphaDegIC(void)      { return alpha*RADTODEG; }
  inline float GetBetaDegIC(void)       { return beta*RADTODEG; }
  
  inline float GetPitchAngleDegIC(void) { return theta*RADTODEG; }
  inline float GetRollAngleDegIC(void) { return phi*RADTODEG; }
  inline float GetHeadingDegIC(void)   { return psi*RADTODEG; }

  inline float GetLatitudeDegIC(void)  { return latitude*RADTODEG; }
  inline float GetLongitudeDegIC(void) { return longitude*RADTODEG; }
  
  inline float GetAltitudeFtIC(void) { return altitude; }
  inline float GetAltitudeAGLFtIC(void) { return altitude - terrain_altitude; }
  
  inline float GetSeaLevelRadiusFtIC(void)  { return sea_level_radius; }
  inline float GetTerrainAltitudeFtIC(void) { return terrain_altitude; }

  void SetVgroundFpsIC(float tt);
  void SetVtrueFpsIC(float tt);
  void SetUBodyFpsIC(float tt);
  void SetVBodyFpsIC(float tt);
  void SetWBodyFpsIC(float tt);
  void SetVnorthFpsIC(float tt);
  void SetVeastFpsIC(float tt);
  void SetVdownFpsIC(float tt);
  void SetWindNEDFpsIC(float wN, float wE, float wD);
  void SetClimbRateFpsIC(float tt);
  inline float GetVgroundFpsIC(void) { return vg; }
  inline float GetVtrueFpsIC(void) { return vt; }
  inline float GetWindUFpsIC(void) { return uw; }
  inline float GetWindVFpsIC(void) { return vw; }
  inline float GetWindWFpsIC(void) { return ww; }
  inline float GetWindNFpsIC(void) { return wnorth; }
  inline float GetWindEFpsIC(void) { return weast; }
  inline float GetWindDFpsIC(void) { return wdown; }
  inline float GetClimbRateFpsIC(void) { return hdot; }
  float GetUBodyFpsIC(void);
  float GetVBodyFpsIC(void);
  float GetWBodyFpsIC(void);
  void SetFlightPathAngleRadIC(float tt);
  void SetAlphaRadIC(float tt);
  void SetPitchAngleRadIC(float tt);
  void SetBetaRadIC(float tt);
  void SetRollAngleRadIC(float tt);
  void SetTrueHeadingRadIC(float tt);
  inline void SetLatitudeRadIC(float tt)  { latitude=tt; }
  inline void SetLongitudeRadIC(float tt) { longitude=tt; }
  inline float GetFlightPathAngleRadIC(void) { return gamma; }
  inline float GetAlphaRadIC(void)      { return alpha; }
  inline float GetPitchAngleRadIC(void) { return theta; }
  inline float GetBetaRadIC(void)       { return beta; }
  inline float GetRollAngleRadIC(void) { return phi; }
  inline float GetHeadingRadIC(void)   { return psi; }
  inline float GetLatitudeRadIC(void) { return latitude; }
  inline float GetLongitudeRadIC(void) { return longitude; }
  inline float GetThetaRadIC(void) { return theta; }
  inline float GetPhiRadIC(void)   { return phi; }
  inline float GetPsiRadIC(void)   { return psi; }

  inline speedset GetSpeedSet(void) { return lastSpeedSet; }

private:
  float vt,vc,ve,vg;
  float mach;
  float altitude,hdot;
  float latitude,longitude;
  float u,v,w;
  float uw,vw,ww;
  float vnorth,veast,vdown;
  float wnorth,weast,wdown;
  double sea_level_radius;
  double terrain_altitude;
  double radius_to_vehicle;

  float  alpha, beta, theta, phi, psi, gamma;
  float salpha,sbeta,stheta,sphi,spsi,sgamma;
  float calpha,cbeta,ctheta,cphi,cpsi,cgamma;

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
  void calcWindUVW(void);

  bool findInterval(float x,float guess);
  bool solve(float *y, float x);
  void Debug(void);
};

#endif

