/*******************************************************************************

 Header:       FGInitialCondition.cpp
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
INCLUDES
*******************************************************************************/

#include "FGInitialCondition.h"
#include "FGFDMExec.h"
#include "FGState.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"
#include "FGDefs.h"


FGInitialCondition::FGInitialCondition(FGFDMExec *fdmex)
{
  vt=vc=0;
  mach=0;
  alpha=beta=gamma=0;
  theta=phi=psi=0;
  altitude=hdot=0;
  latitude=longitude=0;
  
  atm=fdmex->GetAtmosphere();
}


FGInitialCondition::~FGInitialCondition(void) {};


/* void FGInitialCondition::SetVcalibratedKtsIC(float tt)
{
  vc=tt*KTSTOFPS;
  
 vt=sqrt(atm->GetDensity(0)/atm->GetDensity(altitude)*vc*vc);
  
  //mach=vt*sqrt(SHRATIO*Reng*atm->GetTemperature(altitude));
}
 */


void FGInitialCondition::SetVtrueKtsIC(float tt)
{
  vt=tt*KTSTOFPS;
  //vc=sqrt(atm->GetDensity(altitude)/atm->GetDensity(0)*vt*vt);
  //mach=vt*sqrt(SHRATIO*Reng*atm->GetTemperature(altitude));
}


/* void FGInitialCondition::SetMachIC(float tt)
{
  mach=tt;
  vt=mach*sqrt(SHRATIO*Reng*atm->GetTemperature(altitude));
  //vc=sqrt(atm->GetDensity(altitude)/atm->GetDensity(0)*vt*vt);
} */



void FGInitialCondition::SetAltitudeFtIC(float tt)
{
  altitude=tt;
  //mach=vt/sqrt(SHRATIO*Reng*atm->GetTemperature(altitude));
  //vc=sqrt(atm->GetDensity(altitude)/atm->GetDensity(0)*vt*vt);
}


void FGInitialCondition::SetFlightPathAngleDegIC(float tt)
{
  gamma=tt*DEGTORAD;
  theta=alpha+gamma;
}


void FGInitialCondition::SetAlphaDegIC(float tt)
{
  alpha=tt*DEGTORAD;
  theta=alpha+gamma;
}


void FGInitialCondition::SetBetaDegIC(float tt)
{
  beta=tt*DEGTORAD;
}


void FGInitialCondition::SetRollAngleDegIC(float tt)
{
  phi=tt*DEGTORAD;
}


void FGInitialCondition::SetPitchAngleDegIC(float tt)
{
  theta=tt*DEGTORAD;
  alpha=theta-gamma;
}


void FGInitialCondition::SetHeadingDegIC(float tt)
{
  psi=tt*DEGTORAD;
}


void FGInitialCondition::SetLatitudeDegIC(float tt)
{
  latitude=tt*DEGTORAD;
}


void FGInitialCondition::SetLongitudeDegIC(float tt)
{
  longitude=tt*DEGTORAD;
}


float FGInitialCondition::GetUBodyFpsIC(void)
{
  return vt*cos(alpha)*cos(beta);
}


float FGInitialCondition::GetVBodyFpsIC(void)
{
  return vt*sin(beta);
}


float FGInitialCondition::GetWBodyFpsIC(void)
{
  return vt*sin(alpha)*cos(beta);
}


float FGInitialCondition::GetThetaRadIC(void)
{
  return theta;
}


float FGInitialCondition::GetPhiRadIC(void)
{
  return phi;
}


float FGInitialCondition::GetPsiRadIC(void)
{
  return psi;
}


float FGInitialCondition::GetLatitudeRadIC(void)
{
  return latitude;
}


float FGInitialCondition::GetLongitudeRadIC(void)
{
  return longitude;
}


float FGInitialCondition::GetAltitudeFtIC(void)
{
  return altitude;
}

