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
#include "FGJSBBase.h"
#include "FGAtmosphere.h"
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_INITIALCONDITION "$Id: FGInitialCondition.h,v 1.37 2002/04/02 05:34:26 jberndt Exp $"

typedef enum { setvt, setvc, setve, setmach, setuvw, setned, setvg } speedset;
typedef enum { setwned, setwmd, setwhc } windset; 

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES, and NOTES [use "class documentation" below for API docs]
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Takes a set of initial conditions and provide a kinematically consistent set
    of body axis velocity components, euler angles, and altitude.  This class
    does not attempt to trim the model i.e. the sim will most likely start in a
    very dynamic state (unless, of course, you have chosen your IC's wisely)
    even after setting it up with this class.

   USAGE NOTES

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
   @author Anthony K. Peden
   @version $Id: FGInitialCondition.h,v 1.37 2002/04/02 05:34:26 jberndt Exp $
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGInitialCondition.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Header File </a>
    @see <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/jsbsim/JSBSim/FGInitialCondition.cpp?rev=HEAD&content-type=text/vnd.viewcvs-markup">
         Source File </a>
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGInitialCondition : public FGJSBBase
{
public:
  /// Constructor
  FGInitialCondition(FGFDMExec *fdmex);
  /// Destructor
  ~FGInitialCondition();

  void SetVcalibratedKtsIC(double tt);
  void SetVequivalentKtsIC(double tt);
  inline void SetVtrueKtsIC(double tt)   { SetVtrueFpsIC(tt*ktstofps);   }
  inline void SetVgroundKtsIC(double tt) { SetVgroundFpsIC(tt*ktstofps); }
  void SetMachIC(double tt);
  
  inline void SetAlphaDegIC(double tt)      { SetAlphaRadIC(tt*degtorad); }
  inline void SetBetaDegIC(double tt)       { SetBetaRadIC(tt*degtorad);}
  
  inline void SetPitchAngleDegIC(double tt) { SetPitchAngleRadIC(tt*degtorad); }
  inline void SetRollAngleDegIC(double tt)  { SetRollAngleRadIC(tt*degtorad);}
  inline void SetTrueHeadingDegIC(double tt){ SetTrueHeadingRadIC(tt*degtorad); }
  
  void SetClimbRateFpmIC(double tt);
  inline void SetFlightPathAngleDegIC(double tt) { SetFlightPathAngleRadIC(tt*degtorad); }

  void SetAltitudeFtIC(double tt);
  void SetAltitudeAGLFtIC(double tt);
  
  void SetSeaLevelRadiusFtIC(double tt);
  void SetTerrainAltitudeFtIC(double tt);

  inline void SetLatitudeDegIC(double tt)  { latitude=tt*degtorad; }
  inline void SetLongitudeDegIC(double tt) { longitude=tt*degtorad; }

  
  inline double GetVcalibratedKtsIC(void) const { return vc*fpstokts; }
  inline double GetVequivalentKtsIC(void) const { return ve*fpstokts; }
  inline double GetVgroundKtsIC(void) const { return vg*fpstokts; }
  inline double GetVtrueKtsIC(void) const { return vt*fpstokts; }
  inline double GetMachIC(void) const { return mach; }
  
  inline double GetClimbRateFpmIC(void) const { return hdot*60; }
  inline double GetFlightPathAngleDegIC(void)const  { return gamma*radtodeg; }
  
  inline double GetAlphaDegIC(void) const { return alpha*radtodeg; }
  inline double GetBetaDegIC(void) const  { return beta*radtodeg; }
  
  inline double GetPitchAngleDegIC(void) const { return theta*radtodeg; }
  inline double GetRollAngleDegIC(void) const { return phi*radtodeg; }
  inline double GetHeadingDegIC(void) const { return psi*radtodeg; }

  inline double GetLatitudeDegIC(void) const { return latitude*radtodeg; }
  inline double GetLongitudeDegIC(void) const { return longitude*radtodeg; }
  
  inline double GetAltitudeFtIC(void) const { return altitude; }
  inline double GetAltitudeAGLFtIC(void) const { return altitude - terrain_altitude; }
  
  inline double GetSeaLevelRadiusFtIC(void) const { return sea_level_radius; }
  inline double GetTerrainAltitudeFtIC(void) const { return terrain_altitude; }

  void SetVgroundFpsIC(double tt);
  void SetVtrueFpsIC(double tt);
  void SetUBodyFpsIC(double tt);
  void SetVBodyFpsIC(double tt);
  void SetWBodyFpsIC(double tt);
  void SetVnorthFpsIC(double tt);
  void SetVeastFpsIC(double tt);
  void SetVdownFpsIC(double tt);
  
  void SetWindNEDFpsIC(double wN, double wE, double wD);
 
  void SetWindMagKtsIC(double mag);
  void SetWindDirDegIC(double dir);
 
  void SetHeadWindKtsIC(double head);
  void SetCrossWindKtsIC(double cross);// positive from left
 
  void SetWindDownKtsIC(double wD);                                          
  
  void SetClimbRateFpsIC(double tt);
  inline double GetVgroundFpsIC(void) const  { return vg; }
  inline double GetVtrueFpsIC(void) const { return vt; }
  inline double GetWindUFpsIC(void) const { return uw; }
  inline double GetWindVFpsIC(void) const { return vw; }
  inline double GetWindWFpsIC(void) const { return ww; }
  inline double GetWindNFpsIC(void) const { return wnorth; }
  inline double GetWindEFpsIC(void) const { return weast; }
  inline double GetWindDFpsIC(void) const { return wdown; }
  inline double GetWindFpsIC(void)  const { return sqrt(wnorth*wnorth + weast*weast); }
  double GetWindDirDegIC(void); 
  inline double GetClimbRateFpsIC(void) const { return hdot; }
  double GetUBodyFpsIC(void);
  double GetVBodyFpsIC(void);
  double GetWBodyFpsIC(void);
  void SetFlightPathAngleRadIC(double tt);
  void SetAlphaRadIC(double tt);
  void SetPitchAngleRadIC(double tt);
  void SetBetaRadIC(double tt);
  void SetRollAngleRadIC(double tt);
  void SetTrueHeadingRadIC(double tt);
  inline void SetLatitudeRadIC(double tt) { latitude=tt; }
  inline void SetLongitudeRadIC(double tt) { longitude=tt; }
  inline double GetFlightPathAngleRadIC(void) const { return gamma; }
  inline double GetAlphaRadIC(void) const      { return alpha; }
  inline double GetPitchAngleRadIC(void) const { return theta; }
  inline double GetBetaRadIC(void) const       { return beta; }
  inline double GetRollAngleRadIC(void) const  { return phi; }
  inline double GetHeadingRadIC(void) const   { return psi; }
  inline double GetLatitudeRadIC(void) const { return latitude; }
  inline double GetLongitudeRadIC(void) const { return longitude; }
  inline double GetThetaRadIC(void) const { return theta; }
  inline double GetPhiRadIC(void)  const  { return phi; }
  inline double GetPsiRadIC(void) const   { return psi; }

  inline speedset GetSpeedSet(void) { return lastSpeedSet; }
  inline windset GetWindSet(void) { return lastWindSet; }
  
  bool Load(string acpath, string acname, string rstname);
  
  void bind(void);
  void unbind(void);

  
private:
  double vt,vc,ve,vg;
  double mach;
  double altitude,hdot;
  double latitude,longitude;
  double u,v,w;
  double uw,vw,ww;
  double vnorth,veast,vdown;
  double wnorth,weast,wdown;
  double whead, wcross, wdir, wmag;
  double sea_level_radius;
  double terrain_altitude;
  double radius_to_vehicle;

  double  alpha, beta, theta, phi, psi, gamma;
  double salpha,sbeta,stheta,sphi,spsi,sgamma;
  double calpha,cbeta,ctheta,cphi,cpsi,cgamma;

  double xlo, xhi,xmin,xmax;

  typedef double (FGInitialCondition::*fp)(double x);
  fp sfunc;

  speedset lastSpeedSet;
  windset lastWindSet;

  FGFDMExec *fdmex;
  FGPropertyManager *PropertyManager;

  bool getAlpha(void);
  bool getTheta(void);
  bool getMachFromVcas(double *Mach,double vcas);

  double GammaEqOfTheta(double Theta);
  double GammaEqOfAlpha(double Alpha);
  double calcVcas(double Mach);
  void calcUVWfromNED(void);
  void calcWindUVW(void);

  bool findInterval(double x,double guess);
  bool solve(double *y, double x);
  void Debug(int from);
};

#endif

