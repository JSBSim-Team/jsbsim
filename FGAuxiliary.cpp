/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 
 Module:       FGAuxiliary.cpp
 Author:       Tony Peden, Jon Berndt
 Date started: 01/26/99
 Purpose:      Calculates additional parameters needed by the visual system, etc.
 Called by:    FGSimExec
 
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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
This class calculates various auxiliary parameters.

REFERENCES
  Anderson, John D. "Introduction to Flight", 3rd Edition, McGraw-Hill, 1989
                    pgs. 112-126
HISTORY
--------------------------------------------------------------------------------
01/26/99   JSB   Created

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGAuxiliary.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGPosition.h"
#include "FGOutput.h"
#include "FGMatrix.h"

static const char *IdSrc = "$Id: FGAuxiliary.cpp,v 1.15 2001/04/25 22:47:59 jberndt Exp $";
static const char *IdHdr = ID_AUXILIARY;

extern short debug_lvl;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAuxiliary::FGAuxiliary(FGFDMExec* fdmex) : FGModel(fdmex),
  vPilotAccel(3),
  vToEyePt(3)
{
  Name = "FGAuxiliary";
  vcas = veas = mach = qbar = pt = 0;
  psl = rhosl = 1;
  earthPosAngle = 0.0;
  
  if (debug_lvl & 2) cout << "Instantiated: " << Name << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAuxiliary::~FGAuxiliary()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGAuxiliary" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAuxiliary::Run()
{
  float A,B,D;

  if (!FGModel::Run()) {
    GetState();
    if (mach < 1) {   //calculate total pressure assuming isentropic flow
      pt=p*pow((1 + 0.2*mach*mach),3.5);
    } else {
      // shock in front of pitot tube, we'll assume its normal and use
      // the Rayleigh Pitot Tube Formula, i.e. the ratio of total
      // pressure behind the shock to the static pressure in front

      B = 5.76*mach*mach/(5.6*mach*mach - 0.8);

      // The denominator above is zero for Mach ~ 0.38, for which
      // we'll never be here, so we're safe

      D = (2.8*mach*mach-0.4)*0.4167;
      pt = p*pow(B,3.5)*D;
    }

    A = pow(((pt-p)/psl+1),0.28571);
    vcas = sqrt(7*psl/rhosl*(A-1));
    veas = sqrt(2*qbar/rhosl);

    // vPilotAccel = Translation->GetUVWdot() + Aircraft->GetXYZep() * Rotation->GetPQRdot();
    
    vToEyePt = Aircraft->GetXYZep() - MassBalance->GetXYZcg();
    vPilotAccel = Translation->GetUVWdot()
                    + Rotation->GetPQRdot() * vToEyePt
                    + Rotation->GetPQR() * (Rotation->GetPQR() * vToEyePt);

    earthPosAngle += State->Getdt()*OMEGA_EARTH;
    return false;
  } else {
    return true;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGAuxiliary::GetHeadWind(void)
{
  float psiw,vw,psi;

  psiw = Atmosphere->GetWindPsi();
  psi = Rotation->Getpsi();
  vw = Atmosphere->GetWindNED().Magnitude();

  return -vw*cos(psiw - psi);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

float FGAuxiliary::GetCrossWind(void)
{
  float psiw,vw,psi;

  psiw = Atmosphere->GetWindPsi();
  psi = Rotation->Getpsi();
  vw = Atmosphere->GetWindNED().Magnitude();

  return  vw*sin(psiw - psi);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::GetState(void)
{
  qbar = Translation->Getqbar();
  mach = Translation->GetMach();
  p = Atmosphere->GetPressure();
  rhosl = Atmosphere->GetDensitySL();
  psl = Atmosphere->GetPressureSL();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAuxiliary::Debug(void)
{
    //TODO: Add your source code here
}

