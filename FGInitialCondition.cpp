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

FGInitialCondition::FGInitialCondition(FGFDMExec *FDMExec) {
  vt=vc=ve=0;
  mach=0;
  alpha=beta=gamma=0;
  theta=phi=psi=0;
  altitude=hdot=0;
  latitude=longitude=0;
  u=v=w=0;
  lastSpeedSet=setvt;
  if(FDMExec != NULL ) {
    fdmex=FDMExec;
    fdmex->GetPosition()->Seth(altitude);
    fdmex->GetAtmosphere()->Run();
  } else {
    cout << "FGInitialCondition: This class requires a pointer to an valid FGFDMExec object" << endl;
  }

}


FGInitialCondition::~FGInitialCondition(void) {}


void FGInitialCondition::SetVcalibratedKtsIC(float tt) {

  if(getMachFromVcas(&mach,tt*KTSTOFPS)) {
    //cout << "Mach: " << mach << endl;
    lastSpeedSet=setvc;
    vc=tt*KTSTOFPS;
    vt=mach*fdmex->GetAtmosphere()->GetSoundSpeed();
    ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
    //cout << "Vt: " << vt*FPSTOKTS << " Vc: " << vc*FPSTOKTS << endl;
  }
  else {
    cout << "Failed to get Mach number for given Vc and altitude, Vc unchanged." << endl;
    cout << "Please mail the set of initial conditions used to apeden@earthlink.net" << endl;
  }
}

void FGInitialCondition::SetVequivalentKtsIC(float tt) {
  ve=tt*KTSTOFPS;
  lastSpeedSet=setve;
  vt=ve*1/sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
  mach=vt/fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
}

void FGInitialCondition::SetVtrueKtsIC(float tt) {
  vt=tt*KTSTOFPS;
  lastSpeedSet=setvt;
  mach=vt/fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
  ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
}

void FGInitialCondition::SetMachIC(float tt) {
  mach=tt;
  lastSpeedSet=setmach;
  vt=mach*fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
  ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
  //cout << "Vt: " << vt*FPSTOKTS << " Vc: " << vc*FPSTOKTS << endl;
}



void FGInitialCondition::SetClimbRateFpmIC(float tt) {

  if(vt > 0.1) {
    hdot=tt/60;
    gamma=asin(hdot/vt);
  }
}

void FGInitialCondition::SetFlightPathAngleRadIC(float tt) {
  gamma=tt;
  getTheta();
  hdot=vt*sin(tt);
}


void FGInitialCondition::SetUBodyFpsIC(float tt) {
  u=tt;
  vt=sqrt(u*u+v*v+w*w);
  lastSpeedSet=setvt;
}

  

void FGInitialCondition::SetVBodyFpsIC(float tt) {
  v=tt;
  vt=sqrt(u*u+v*v+w*w);
  lastSpeedSet=setvt;
}

void FGInitialCondition::SetWBodyFpsIC(float tt) {
  w=tt;
  vt=sqrt(u*u+v*v+w*w);
  lastSpeedSet=setvt;
}


void FGInitialCondition::SetAltitudeFtIC(float tt) {
  altitude=tt;
  fdmex->GetPosition()->Seth(altitude);
  fdmex->GetAtmosphere()->Run();

  //lets try to make sure the user gets what they intended

  switch(lastSpeedSet) {
  case setvt:
    SetVtrueKtsIC(vt*FPSTOKTS);
    break;
  case setvc:
    SetVcalibratedKtsIC(vc*FPSTOKTS);
    break;
  case setve:
    SetVequivalentKtsIC(ve*FPSTOKTS);
    break;
  case setmach:
    SetMachIC(mach);
    break;
  }
}

void FGInitialCondition::SetAltitudeAGLFtIC(float tt) {
  fdmex->GetPosition()->SetDistanceAGL(tt);
  altitude=fdmex->GetPosition()->Geth();
  SetAltitudeFtIC(altitude);
}  

bool FGInitialCondition::getMachFromVcas(float *Mach,float vcas) {
 
  bool result=false;
  float guess=1.5;
  xlo=xhi=0;
  xmin=0;xmax=50;
  sfunc=&FGInitialCondition::calcVcas;
  if(findInterval(vcas,guess)) {
    if(solve(&mach,vcas))
      result=true;
  }    
  return result;
}

bool FGInitialCondition::getAlpha(void) {
  bool result=false;
  float guess=theta-gamma;
  xlo=xhi=0;
  xmin=fdmex->GetAircraft()->GetAlphaCLMin();
  xmax=fdmex->GetAircraft()->GetAlphaCLMax();
  sfunc=&FGInitialCondition::GammaEqOfAlpha;
  if(findInterval(0,guess)){
    if(solve(&alpha,0)){
      result=true;
    }
  }
  return result;
}      
    
bool FGInitialCondition::getTheta(void) {
  bool result=false;
  float guess=alpha+gamma;
  xlo=xhi=0;
  xmin=-89;xmax=89;
  sfunc=&FGInitialCondition::GammaEqOfTheta;
  if(findInterval(0,guess)){
    if(solve(&theta,0)){
      result=true;
    }
  }
  return result;
}      
  


float FGInitialCondition::GammaEqOfTheta(float Theta) {
  float a,b,c;
  
  a=cos(alpha)*cos(beta)*sin(Theta);
  b=sin(beta)*sin(phi);
  c=sin(alpha)*cos(beta)*cos(phi);
  return sin(gamma)-a+(b+c)*cos(Theta);
}

float FGInitialCondition::GammaEqOfAlpha(float Alpha) {
  float a,b,c;
  
  a=cos(Alpha)*cos(beta)*sin(theta);
  b=sin(beta)*sin(phi);
  c=sin(Alpha)*cos(beta)*cos(phi);
  return sin(gamma)-a+(b+c)*cos(theta);
}

  
float FGInitialCondition::calcVcas(float Mach) {

  float p=fdmex->GetAtmosphere()->GetPressure();
  float psl=fdmex->GetAtmosphere()->GetPressureSL();
  float rhosl=fdmex->GetAtmosphere()->GetDensitySL();
  float pt,A,B,D,vcas;
  if(Mach < 0) Mach=0;
  if(Mach < 1)    //calculate total pressure assuming isentropic flow
    pt=p*pow((1 + 0.2*Mach*Mach),3.5);
  else {
    // shock in front of pitot tube, we'll assume its normal and use
    // the Rayleigh Pitot Tube Formula, i.e. the ratio of total
    // pressure behind the shock to the static pressure in front


    //the normal shock assumption should not be a bad one -- most supersonic
    //aircraft place the pitot probe out front so that it is the forward
    //most point on the aircraft.  The real shock would, of course, take
    //on something like the shape of a rounded-off cone but, here again,
    //the assumption should be good since the opening of the pitot probe
    //is very small and, therefore, the effects of the shock curvature
    //should be small as well. AFAIK, this approach is fairly well accepted
    //within the aerospace community

    B = 5.76*Mach*Mach/(5.6*Mach*Mach - 0.8);

    // The denominator above is zero for Mach ~ 0.38, for which
    // we'll never be here, so we're safe

    D = (2.8*Mach*Mach-0.4)*0.4167;
    pt = p*pow(B,3.5)*D;
  }

  A = pow(((pt-p)/psl+1),0.28571);
  vcas = sqrt(7*psl/rhosl*(A-1));
  //cout << "calcVcas: vcas= " << vcas*FPSTOKTS << " mach= " << Mach << " pressure: " << pt << endl;
  return vcas;
}

bool FGInitialCondition::findInterval(float x,float guess) {
  //void find_interval(inter_params &ip,eqfunc f,float y,float constant, int &flag){

  int i=0;
  bool found=false;
  float flo,fhi,fguess;
  float lo,hi,step;
  step=0.1;
  fguess=(this->*sfunc)(guess)-x;
  lo=hi=guess;
  do {
    step=2*step;
    lo-=step;
    hi+=step;
    if(lo < xmin) lo=xmin;
    if(hi > xmax) hi=xmax;
    i++;
    flo=(this->*sfunc)(lo)-x;
    fhi=(this->*sfunc)(hi)-x;
    if(flo*fhi <=0) {  //found interval with root
      found=true;
      if(flo*fguess <= 0) {  //narrow interval down a bit
        hi=lo+step;    //to pass solver interval that is as
        //small as possible
      }
      else if(fhi*fguess <= 0) {
        lo=hi-step;
      }
    }
    //cout << "findInterval: i=" << i << " Lo= " << lo << " Hi= " << hi << endl;
  }
  while((found == 0) && (i <= 100));
  xlo=lo;
  xhi=hi;
  return found;
}




bool FGInitialCondition::solve(float *y,float x) {  
  float x1,x2,x3,f1,f2,f3,d,d0;
  float eps=1E-5;
  float const relax =0.9;
  int i;
  bool success=false;
  
   //initializations
  d=1;
  
    x1=xlo;x3=xhi;
    f1=(this->*sfunc)(x1)-x;
    f3=(this->*sfunc)(x3)-x;
    d0=fabs(x3-x1);
  
    //iterations
    i=0;
    while ((fabs(d) > eps) && (i < 100)) {
      d=(x3-x1)/d0;
      x2=x1-d*d0*f1/(f3-f1);
      
      f2=(this->*sfunc)(x2)-x;
      //cout << "solve x1,x2,x3: " << x1 << "," << x2 << "," << x3 << endl;
      //cout << "                " << f1 << "," << f2 << "," << f3 << endl;

      if(fabs(f2) <= 0.001) {
        x1=x3=x2;
      } else if(f1*f2 <= 0.0) {
        x3=x2;
        f3=f2;
        f1=relax*f1;
      } else if(f2*f3 <= 0) {
        x1=x2;
        f1=f2;
        f3=relax*f3;
      }
      //cout << i << endl;
      i++;
    }//end while
    if(i < 100) {
      success=true;
      *y=x2;
    }

  //cout << "Success= " << success << " Vcas: " << vcas*FPSTOKTS << " Mach: " << x2 << endl;
  return success;
}
