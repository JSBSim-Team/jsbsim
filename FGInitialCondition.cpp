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
#include "FGConfigFile.h"

static const char *IdSrc = "$Id: FGInitialCondition.cpp,v 1.39 2001/11/12 05:06:27 jberndt Exp $";
static const char *IdHdr = ID_INITIALCONDITION;

//******************************************************************************

FGInitialCondition::FGInitialCondition(FGFDMExec *FDMExec)
{
  vt=vc=ve=vg=0;
  mach=0;
  alpha=beta=gamma=0;
  theta=phi=psi=0;
  altitude=hdot=0;
  latitude=longitude=0;
  u=v=w=0;
  uw=vw=ww=0;
  vnorth=veast=vdown=0;
  wnorth=weast=wdown=0;
  whead=wcross=0;
  wdir=wmag=0;
  lastSpeedSet=setvt;
  lastWindSet=setwned;
  sea_level_radius = FDMExec->GetInertial()->RefRadius();
  radius_to_vehicle = FDMExec->GetInertial()->RefRadius();
  terrain_altitude = 0;

  salpha=sbeta=stheta=sphi=spsi=sgamma=0;
  calpha=cbeta=ctheta=cphi=cpsi=cgamma=1;

  if(FDMExec != NULL ) {
    fdmex=FDMExec;
    fdmex->GetPosition()->Seth(altitude);
    fdmex->GetAtmosphere()->Run();
  } else {
    cout << "FGInitialCondition: This class requires a pointer to a valid FGFDMExec object" << endl;
  }

  if (debug_lvl & 2) cout << "Instantiated: FGInitialCondition" << endl;
}

//******************************************************************************

FGInitialCondition::~FGInitialCondition()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGInitialCondition" << endl;
}

//******************************************************************************

void FGInitialCondition::SetVcalibratedKtsIC(float tt) {

  if(getMachFromVcas(&mach,tt*ktstofps)) {
    //cout << "Mach: " << mach << endl;
    lastSpeedSet=setvc;
    vc=tt*ktstofps;
    vt=mach*fdmex->GetAtmosphere()->GetSoundSpeed();
    ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
    //cout << "Vt: " << vt*fpstokts << " Vc: " << vc*fpstokts << endl;
  }
  else {
    cout << "Failed to get Mach number for given Vc and altitude, Vc unchanged." << endl;
    cout << "Please mail the set of initial conditions used to apeden@earthlink.net" << endl;
  }
}

//******************************************************************************

void FGInitialCondition::SetVequivalentKtsIC(float tt) {
  ve=tt*ktstofps;
  lastSpeedSet=setve;
  vt=ve*1/sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
  mach=vt/fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
}

//******************************************************************************

void FGInitialCondition::SetVgroundFpsIC(float tt) {
  float ua,va,wa;
  float vxz;

  vg=tt;
  lastSpeedSet=setvg;
  vnorth = vg*cos(psi); veast = vg*sin(psi); vdown = 0;
  calcUVWfromNED();
  ua = u + uw; va = v + vw; wa = w + ww;
  vt = sqrt( ua*ua + va*va + wa*wa );
  alpha = beta = 0;
  vxz = sqrt( u*u + w*w );
  if( w != 0 ) alpha = atan2( w, u );
  if( vxz != 0 ) beta = atan2( v, vxz );
  mach=vt/fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
  ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
}

//******************************************************************************

void FGInitialCondition::SetVtrueFpsIC(float tt) {
  vt=tt;
  lastSpeedSet=setvt;
  mach=vt/fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
  ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
}

//******************************************************************************

void FGInitialCondition::SetMachIC(float tt) {
  mach=tt;
  lastSpeedSet=setmach;
  vt=mach*fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
  ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
  //cout << "Vt: " << vt*fpstokts << " Vc: " << vc*fpstokts << endl;
}

//******************************************************************************

void FGInitialCondition::SetClimbRateFpmIC(float tt) {
  SetClimbRateFpsIC(tt/60.0);
}

//******************************************************************************

void FGInitialCondition::SetClimbRateFpsIC(float tt) {

  if(vt > 0.1) {
    hdot=tt;
    gamma=asin(hdot/vt);
    sgamma=sin(gamma); cgamma=cos(gamma);
  }
}

//******************************************************************************

void FGInitialCondition::SetFlightPathAngleRadIC(float tt) {
  gamma=tt;
  sgamma=sin(gamma); cgamma=cos(gamma);
  getTheta();
  hdot=vt*sgamma;
}

//******************************************************************************

void FGInitialCondition::SetAlphaRadIC(float tt) {
  alpha=tt;
  salpha=sin(alpha); calpha=cos(alpha);
  getTheta();
}

//******************************************************************************

void FGInitialCondition::SetPitchAngleRadIC(float tt) {
  theta=tt;
  stheta=sin(theta); ctheta=cos(theta);
  getAlpha();
}

//******************************************************************************

void FGInitialCondition::SetBetaRadIC(float tt) {
  beta=tt;
  sbeta=sin(beta); cbeta=cos(beta);
  getTheta();
  
}

//******************************************************************************

void FGInitialCondition::SetRollAngleRadIC(float tt) {
  phi=tt;
  sphi=sin(phi); cphi=cos(phi);
  getTheta();
}

//******************************************************************************

void FGInitialCondition::SetTrueHeadingRadIC(float tt) {
    psi=tt;
    spsi=sin(psi); cpsi=cos(psi);
    calcWindUVW();
}

//******************************************************************************

void FGInitialCondition::SetUBodyFpsIC(float tt) {
  u=tt;
  vt=sqrt(u*u + v*v + w*w);
  lastSpeedSet=setuvw;
}

//******************************************************************************

void FGInitialCondition::SetVBodyFpsIC(float tt) {
  v=tt;
  vt=sqrt(u*u + v*v + w*w);
  lastSpeedSet=setuvw;
}

//******************************************************************************

void FGInitialCondition::SetWBodyFpsIC(float tt) {
  w=tt;
  vt=sqrt( u*u + v*v + w*w );
  lastSpeedSet=setuvw;
}

//******************************************************************************

float FGInitialCondition::GetUBodyFpsIC(void) {
    if(lastSpeedSet == setvg )
      return u;
    else
      return vt*calpha*cbeta - uw;
}

//******************************************************************************

float FGInitialCondition::GetVBodyFpsIC(void) {
    if( lastSpeedSet == setvg )
      return v;
    else {
      return vt*sbeta - vw;
    }  
}

//******************************************************************************

float FGInitialCondition::GetWBodyFpsIC(void) {
    if( lastSpeedSet == setvg )
      return w;
    else 
      return vt*salpha*cbeta -ww;
}

//******************************************************************************

void FGInitialCondition::SetWindNEDFpsIC(float wN, float wE, float wD ) {
  wnorth = wN; weast = wE; wdown = wD;
  lastWindSet = setwned;
  calcWindUVW();
  if(lastSpeedSet == setvg)
    SetVgroundFpsIC(vg);
}

//******************************************************************************

// positive from left
void FGInitialCondition::SetHeadWindKtsIC(float head){ 
    whead=head*ktstofps;
    lastWindSet=setwhc; 
    calcWindUVW();
    if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);

} 

//******************************************************************************

void FGInitialCondition::SetCrossWindKtsIC(float cross){ 
    wcross=cross*ktstofps; 
    lastWindSet=setwhc; 
    calcWindUVW();
    if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);

} 

//******************************************************************************

void FGInitialCondition::SetWindDownKtsIC(float wD) { 
    wdown=wD; 
    calcWindUVW();
    if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);
} 

//******************************************************************************

void FGInitialCondition::SetWindMagKtsIC(float mag) {
  wmag=mag*ktstofps;
  lastWindSet=setwmd;
  calcWindUVW();    
  if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);
}

//******************************************************************************

void FGInitialCondition::SetWindDirDegIC(float dir) {
  wdir=dir*degtorad;
  lastWindSet=setwmd;
  calcWindUVW();    
  if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);
}


//******************************************************************************

void FGInitialCondition::calcWindUVW(void) {
    
    switch(lastWindSet) {
      case setwmd:
        wnorth=wmag*cos(wdir);
        weast=wmag*sin(wdir);
      break;
      case setwhc:
        wnorth=whead*cos(psi) + wcross*cos(psi+M_PI/2);
        weast=whead*sin(psi) + wcross*sin(psi+M_PI/2);
      break;
    }    
    uw=wnorth*ctheta*cpsi +
       weast*ctheta*spsi -
       wdown*stheta;
    vw=wnorth*( sphi*stheta*cpsi - cphi*spsi ) +
        weast*( sphi*stheta*spsi + cphi*cpsi ) +
       wdown*sphi*ctheta;
    ww=wnorth*(cphi*stheta*cpsi + sphi*spsi) +
       weast*(cphi*stheta*spsi - sphi*cpsi) +
       wdown*cphi*ctheta;
            
   
    /* cout << "FGInitialCondition::calcWindUVW: wnorth, weast, wdown "
         << wnorth << ", " << weast << ", " << wdown << endl;
    cout << "FGInitialCondition::calcWindUVW: theta, phi, psi "
          << theta << ", " << phi << ", " << psi << endl;
    cout << "FGInitialCondition::calcWindUVW: uw, vw, ww "
          << uw << ", " << vw << ", " << ww << endl;   */

}

//******************************************************************************

void FGInitialCondition::SetAltitudeFtIC(float tt) {
  altitude=tt;
  fdmex->GetPosition()->Seth(altitude);
  fdmex->GetAtmosphere()->Run();
  //lets try to make sure the user gets what they intended

  switch(lastSpeedSet) {
  case setned:
  case setuvw:
  case setvt:
    SetVtrueKtsIC(vt*fpstokts);
    break;
  case setvc:
    SetVcalibratedKtsIC(vc*fpstokts);
    break;
  case setve:
    SetVequivalentKtsIC(ve*fpstokts);
    break;
  case setmach:
    SetMachIC(mach);
    break;
  case setvg:
    SetVgroundFpsIC(vg);
    break;
  }
}

//******************************************************************************

void FGInitialCondition::SetAltitudeAGLFtIC(float tt) {
  fdmex->GetPosition()->SetDistanceAGL(tt);
  altitude=fdmex->GetPosition()->Geth();
  SetAltitudeFtIC(altitude);
}

//******************************************************************************

void FGInitialCondition::SetSeaLevelRadiusFtIC(double tt) {
  sea_level_radius = tt;
}

//******************************************************************************

void FGInitialCondition::SetTerrainAltitudeFtIC(double tt) {
  terrain_altitude=tt;
}

//******************************************************************************

void FGInitialCondition::calcUVWfromNED(void) {
  u=vnorth*ctheta*cpsi +
     veast*ctheta*spsi -
     vdown*stheta;
  v=vnorth*( sphi*stheta*cpsi - cphi*spsi ) +
     veast*( sphi*stheta*spsi + cphi*cpsi ) +
     vdown*sphi*ctheta;
  w=vnorth*( cphi*stheta*cpsi + sphi*spsi ) +
     veast*( cphi*stheta*spsi - sphi*cpsi ) +
     vdown*cphi*ctheta;
}

//******************************************************************************

void FGInitialCondition::SetVnorthFpsIC(float tt) {
  vnorth=tt;
  calcUVWfromNED();
  vt=sqrt(u*u + v*v + w*w);
  lastSpeedSet=setned;
}

//******************************************************************************

void FGInitialCondition::SetVeastFpsIC(float tt) {
  veast=tt;
  calcUVWfromNED();
  vt=sqrt(u*u + v*v + w*w);
  lastSpeedSet=setned;
}

//******************************************************************************

void FGInitialCondition::SetVdownFpsIC(float tt) {
  vdown=tt;
  calcUVWfromNED();
  vt=sqrt(u*u + v*v + w*w);
  SetClimbRateFpsIC(-1*vdown);
  lastSpeedSet=setned;
}

//******************************************************************************

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

//******************************************************************************

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
      salpha=sin(alpha);
      calpha=cos(alpha);
    }
  }
  calcWindUVW();
  return result;
}

//******************************************************************************

bool FGInitialCondition::getTheta(void) {
  bool result=false;
  float guess=alpha+gamma;
  xlo=xhi=0;
  xmin=-89;xmax=89;
  sfunc=&FGInitialCondition::GammaEqOfTheta;
  if(findInterval(0,guess)){
    if(solve(&theta,0)){
      result=true;
      stheta=sin(theta);
      ctheta=cos(theta);
    }
  }
  calcWindUVW();
  return result;
}

//******************************************************************************

float FGInitialCondition::GammaEqOfTheta(float Theta) {
  float a,b,c,d;
  float sTheta,cTheta;

  //theta=Theta; stheta=sin(theta); ctheta=cos(theta);
  sTheta=sin(Theta); cTheta=cos(Theta);
  calcWindUVW();
  a=wdown + vt*calpha*cbeta + uw;
  b=vt*sphi*sbeta + vw*sphi;
  c=vt*cphi*salpha*cbeta + ww*cphi;
  return vt*sgamma - ( a*sTheta - (b+c)*cTheta);
}

//******************************************************************************

float FGInitialCondition::GammaEqOfAlpha(float Alpha) {
  float a,b,c,d;
  float sAlpha,cAlpha;

  sAlpha=sin(Alpha); cAlpha=cos(Alpha);
  a=wdown + vt*cAlpha*cbeta + uw;
  b=vt*sphi*sbeta + vw*sphi;
  c=vt*cphi*sAlpha*cbeta + ww*cphi;

  return vt*sgamma - ( a*stheta - (b+c)*ctheta );
}

//******************************************************************************

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
  //cout << "calcVcas: vcas= " << vcas*fpstokts << " mach= " << Mach << " pressure: " << pt << endl;
  return vcas;
}

//******************************************************************************

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

//******************************************************************************

bool FGInitialCondition::solve(float *y,float x)
{
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

  //cout << "Success= " << success << " Vcas: " << vcas*fpstokts << " Mach: " << x2 << endl;
  return success;
}

//******************************************************************************

float FGInitialCondition::GetWindDirDegIC(void) {
  if(weast != 0.0) 
    return atan2(weast,wnorth)*radtodeg;
  else if(wnorth > 0) 
    return 0.0;
  else
    return 180.0;
}        

//******************************************************************************

bool FGInitialCondition::Load(string path, string acname, string fname)
{
  string resetDef;
  string token="";

  float temp;

# ifndef macintosh
  resetDef = path + "/" + acname + "/" + fname + ".xml";
# else
  resetDef = path + ";" + acname + ";" + fname + ".xml";
# endif

  cout << resetDef << endl;
  FGConfigFile resetfile(resetDef);
  if (!resetfile.IsOpen()) return false;

  resetfile.GetNextConfigLine();
  token = resetfile.GetValue();
  if (token != "initialize") {
    cerr << "The reset file " << resetDef
         << " does not appear to be a reset file" << endl;
    return false;
  }
  
  resetfile.GetNextConfigLine();
  resetfile >> token;
  while (token != "/initialize" && token != "EOF") {
    if (token == "UBODY" ) { resetfile >> temp; SetUBodyFpsIC(temp); } 
    if (token == "VBODY" ) { resetfile >> temp; SetVBodyFpsIC(temp); } 
    if (token == "WBODY" ) { resetfile >> temp; SetWBodyFpsIC(temp); }  
    if (token == "LATITUDE" ) { resetfile >> temp; SetLatitudeDegIC(temp); }
    if (token == "LONGITUDE" ) { resetfile >> temp; SetLongitudeDegIC(temp); }
    if (token == "PHI" ) { resetfile >> temp; SetRollAngleDegIC(temp); }
    if (token == "THETA" ) { resetfile >> temp; SetPitchAngleDegIC(temp); }
    if (token == "PSI" ) { resetfile >> temp; SetTrueHeadingDegIC(temp); }
    if (token == "ALPHA" ) { resetfile >> temp; SetAlphaDegIC(temp); }
    if (token == "BETA" ) { resetfile >> temp; SetBetaDegIC(temp); }
    if (token == "GAMMA" ) { resetfile >> temp; SetFlightPathAngleDegIC(temp); }
    if (token == "ROC" ) { resetfile >> temp; SetClimbRateFpmIC(temp); }
    if (token == "ALTITUDE" ) { resetfile >> temp; SetAltitudeFtIC(temp); }
    if (token == "WINDDIR" ) { resetfile >> temp; SetWindDirDegIC(temp); }
    if (token == "VWIND" ) { resetfile >> temp; SetWindMagKtsIC(temp); }
    if (token == "HWIND" ) { resetfile >> temp; SetHeadWindKtsIC(temp); }
    if (token == "XWIND" ) { resetfile >> temp; SetCrossWindKtsIC(temp); }
    if (token == "VC" ) { resetfile >> temp; SetVcalibratedKtsIC(temp); }
    if (token == "MACH" ) { resetfile >> temp; SetMachIC(temp); }
    if (token == "VGROUND" ) { resetfile >> temp; SetVgroundKtsIC(temp); }
    resetfile >> token;
  }

  fdmex->RunIC(this);
  
  return true;
}  
