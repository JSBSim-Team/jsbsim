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

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGInitialCondition
{
  public:

    FGInitialCondition(FGFDMExec *fdmex);
    ~FGInitialCondition(void);

    void SetVcalibratedKtsIC(float tt);
    void SetVtrueKtsIC(float tt);
    void SetMachIC(float tt); 

    void SetAltitudeFtIC(float tt);
    void SetFlightPathAngleDegIC(float tt);  //"vertical" flight path, solve for alpha using speed
    //void SetClimbRateFpmIC(float tt);      //overwrite gamma
    void SetAlphaDegIC(float tt);            //use currently stored gamma
    void SetBetaDegIC(float tt);
    void SetRollAngleDegIC(float tt);
    void SetPitchAngleDegIC(float tt);       //use currently stored gamma
    void SetHeadingDegIC(float tt);
    void SetLatitudeDegIC(float tt);
    void SetLongitudeDegIC(float tt);

    float GetUBodyFpsIC(void);
    float GetVBodyFpsIC(void);
    float GetWBodyFpsIC(void);

    float GetThetaRadIC(void);
    float GetPhiRadIC(void);
    float GetPsiRadIC(void);

    float GetLatitudeRadIC(void);
    float GetLongitudeRadIC(void);

    float GetAltitudeFtIC(void);

  private:
    float vt,vc;
    float alpha,beta,gamma,theta,phi,psi;
    float mach;
    float altitude,hdot;
    float u,v,w;
    float latitude,longitude;

    FGFDMExec *fdmex;
	
	float calcVcas(float Mach);
	bool findMachInterval(float *mlo, float *mhi,float vcas);
	bool getMachFromVcas(float *Mach,float vcas);
};

#endif
