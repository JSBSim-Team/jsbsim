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

CAVEAT: This class makes use of alpha=theta-gamma. This means that setting
        any of the three with this class is only valid for steady state
        (all accels zero) and zero pitch rate.  One example where this
        would produce invalid results is setting up for a trim in a pull-up
        or pushover (both have nonzero pitch rate).  Maybe someday...

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

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

typedef enum { none,setvt, setvc, setve, setmach } speedset;

/* USAGE NOTES
   FGFDMExec:
     A valid FGFDMExec object is required at instantiation by this
	 class.  This is so that FGAtmosphere can be used. 
   
   Speed:
	 Since vc, ve, vt, and mach all represent speed, the remaining
	 three are recalculated each time one of them is set (using the
	 current altitude).  The most recent speed set is remembered so 
	 that if and when altitude is reset, the last set speed is used 
	 to recalculate the remaining three. 
   
   Alpha,Gamma, and Theta:
     This class assumes that it will be used to set up the sim for a
	 steady, zero pitch rate condition. This entails the assumption
	 that alpha=theta-gamma. Since any two of those angles specifies 
	 the third (again, for zero pitch rate) gamma (flight path angle) 
	 is favored when setting alpha and theta and alpha is favored when 
	 setting gamma. i.e.
    	  set alpha : recalculate theta using gamma as currently set
		  set theta : recalculate alpha using gamma as currently set
		  set gamma : recalculate theta using alpha as currently set

	 The idea being that gamma is most interesting to pilots (since it 
	 is indicative of climb rate). 
	 
	 Setting climb rate is, for the purpose of this discussion, 
	 considered equivalent to setting gamma.

*/
class FGInitialCondition
{
public:

    FGInitialCondition(FGFDMExec *fdmex);
    ~FGInitialCondition(void);

    void SetVcalibratedKtsIC(float tt);
    void SetVequivalentKtsIC(float tt);
    void SetVtrueKtsIC(float tt);
    void SetMachIC(float tt);

    void SetAltitudeFtIC(float tt);

    //"vertical" flight path, recalculate theta
    inline void SetFlightPathAngleDegIC(float tt) { gamma=tt*DEGTORAD;theta=alpha+gamma;}
    //set speed first
    void SetClimbRateFpmIC(float tt);
    //use currently stored gamma, recalcualte theta
    inline void SetAlphaDegIC(float tt)      { alpha=tt*DEGTORAD;theta=alpha+gamma;}
    //use currently stored gamma, recalcualte alpha
    inline void SetPitchAngleDegIC(float tt) { theta=tt*DEGTORAD;alpha=theta-gamma;}

    inline void SetBetaDegIC(float tt)       { beta=tt*DEGTORAD; }

    inline void SetRollAngleDegIC(float tt) { phi=tt*DEGTORAD; }
    inline void SetHeadingDegIC(float tt)   { psi=tt*DEGTORAD; }

    inline void SetLatitudeDegIC(float tt)  { latitude=tt*DEGTORAD; }
    inline void SetLongitudeDegIC(float tt) { longitude=tt*DEGTORAD; }


    inline float GetVcalibratedKtsIC(void) { return vc*FPSTOKTS; }
    inline float GetVequivalentKtsIC(void){ return ve*FPSTOKTS; }
    inline float GetVtrueKtsIC(void){ return vt*FPSTOKTS; }
    inline float GetMachIC(void){ return mach; }

    inline float GetAltitudeFtIC(void){ return altitude; }

    inline float GetFlightPathAngleDegIC(void) { return gamma*RADTODEG;}
    inline float GetClimbRateFpmIC(void) { return hdot*60; }
    inline float GetAlphaDegIC(void)      { return alpha*RADTODEG;}
    inline float GetPitchAngleDegIC(void) { return theta*RADTODEG;}

    inline float GetBetaDegIC(void)       { return beta*RADTODEG; }

    inline float GetRollAngleDegIC(void) { return phi*RADTODEG; }
    inline float GetHeadingDegIC(void)   { return psi*RADTODEG; }

    inline float GetLatitudeDegIC(void)  { return latitude*RADTODEG; }
    inline float GetLongitudeDegIC(void) { return longitude*RADTODEG; }


    inline float GetUBodyFpsIC(void) { return vt*cos(alpha)*cos(beta); }
    inline float GetVBodyFpsIC(void) { return vt*sin(beta);}
    inline float GetWBodyFpsIC(void) { return vt*sin(alpha)*cos(beta);}

    inline float GetThetaRadIC(void) { return theta;}
    inline float GetPhiRadIC(void)   { return phi;}
    inline float GetPsiRadIC(void)   { return psi; }

    inline float GetLatitudeRadIC(void) { return latitude; }
    inline float GetLongitudeRadIC(void) { return longitude; }


private:
    float vt,vc,ve;
    float alpha,beta,gamma,theta,phi,psi;
    float mach;
    float altitude,hdot;
    float latitude,longitude;
    speedset lastSpeedSet;

    FGFDMExec *fdmex;

    float calcVcas(float Mach);
    bool findMachInterval(float *mlo, float *mhi,float vcas);
    bool getMachFromVcas(float *Mach,float vcas);
};

#endif
