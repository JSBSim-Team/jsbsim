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
#include "FGMatrix33.h"
#include "FGColumnVector3.h"
#include "FGColumnVector4.h"

static const char *IdSrc = "$Id: FGAuxiliary.cpp,v 1.18 2001/08/29 17:56:15 jberndt Exp $";
static const char *IdHdr = ID_AUXILIARY;

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

    // Pilot sensed accelerations are calculated here. This is used
    // for the coordinated turn ball instrument. Motion base platforms sometimes
    // use the derivative of pilot sensed accelerations as the driving parameter,
    // rather than straight accelerations.
    //
    // The theory behind pilot-sensed calculations is presented:
    //
    // For purposes of discussion and calculation, assume for a minute that the
    // pilot is in space and motionless in inertial space. She will feel
    // no accelerations. If the aircraft begins to accelerate along any axis or
    // axes (without rotating), the pilot will sense those accelerations. If
    // any rotational moment is applied, the pilot will sense an acceleration
    // due to that motion in the amount:
    //
    // [wdot X R]  +  [w X (w X R)]
    //   Term I          Term II
    //
    // where:
    //
    // wdot = omegadot, the rotational acceleration rate vector
    // w    = omega, the rotational rate vector
    // R    = the vector from the aircraft CG to the pilot eyepoint
    //
    // The sum total of these two terms plus the acceleration of the aircraft
    // body axis gives the acceleration the pilot senses in inertial space.
    // In the presence of a large body such as a planet, a gravity field also
    // provides an accelerating attraction. This acceleration can be transformed
    // from the reference frame of the planet so as to be expressed in the frame
    // of reference of the aircraft. This gravity field accelerating attraction
    // is felt by the pilot as a force on her tushie as she sits in her aircraft
    // on the runway awaiting takeoff clearance.
    //
    // In JSBSim the acceleration of the body frame in inertial space is given
    // by the F = ma relation. If the vForces vector is divided by the aircraft
    // mass, the acceleration vector is calculated. The gravity field is already
    // included in the vForces vector. the term wdot is equivalent to the JSBSim
    // vPQRdot vector, and the w parameter is equivalent to vPQR. The radius R is
    // calculated below in the vector vToEyePt.
        
    vToEyePt = Aircraft->GetXYZep() - MassBalance->GetXYZcg();

    vPilotAccel = Aircraft->GetForces()/MassBalance->GetMass()
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

