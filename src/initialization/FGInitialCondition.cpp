/*******************************************************************************

 Header:       FGInitialCondition.cpp
 Author:       Tony Peden
 Date started: 7/1/99

 ------------- Copyright (C) 1999  Anthony K. Peden (apeden@earthlink.net) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
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
#include <FGFDMExec.h>
#include <models/FGInertial.h>
#include <models/FGAtmosphere.h>
#include <models/FGAerodynamics.h>
#include <models/FGPropagate.h>
#include <input_output/FGPropertyManager.h>
#include <models/FGPropulsion.h>
#include <input_output/FGXMLParse.h>
#include <math/FGQuaternion.h>
#include <fstream>

namespace JSBSim {

static const char *IdSrc = "$Id: FGInitialCondition.cpp,v 1.31 2009/05/26 05:35:42 jberndt Exp $";
static const char *IdHdr = ID_INITIALCONDITION;

//******************************************************************************

FGInitialCondition::FGInitialCondition(FGFDMExec *FDMExec) : fdmex(FDMExec)
{
  InitializeIC();

  if(FDMExec != NULL ) {
    fdmex->GetPropagate()->SetAltitudeASL(altitudeASL);
    fdmex->GetAtmosphere()->Run();
    PropertyManager=fdmex->GetPropertyManager();
    Constructing = true;
    bind();
    Constructing = false;
  } else {
    cout << "FGInitialCondition: This class requires a pointer to a valid FGFDMExec object" << endl;
  }

  Debug(0);
}

//******************************************************************************

FGInitialCondition::~FGInitialCondition()
{
  Debug(1);
}

//******************************************************************************

void FGInitialCondition::ResetIC(double u0, double v0, double w0,
                                 double p0, double q0, double r0,
                                 double alpha0, double beta0,
                                 double phi0, double theta0, double psi0,
                                 double latRad0, double lonRad0, double altAGLFt0,
                                 double gamma0)
{
  InitializeIC();

  u = u0;  v = v0;  w = w0;
  p = p0;  q = q0;  r = r0;
  alpha = alpha0;  beta = beta0;
  phi = phi0;  theta = theta0;  psi = psi0;
  gamma = gamma0;

  latitude = latRad0;
  longitude = lonRad0;
  SetAltitudeAGLFtIC(altAGLFt0);

  cphi   = cos(phi);   sphi   = sin(phi);   // phi, rad
  ctheta = cos(theta); stheta = sin(theta); // theta, rad
  cpsi   = cos(psi);   spsi   = sin(psi);   // psi, rad

  FGQuaternion Quat( phi, theta, psi );
  Quat.Normalize();

  const FGMatrix33& _Tl2b  = Quat.GetT();     // local to body frame
  const FGMatrix33& _Tb2l  = Quat.GetTInv();  // body to local

  FGColumnVector3 _vUVW_BODY(u,v,w);
  FGColumnVector3 _vUVW_NED = _Tb2l * _vUVW_BODY;
  FGColumnVector3 _vWIND_NED(wnorth,weast,wdown);
//  FGColumnVector3 _vUVWAero = _Tl2b * ( _vUVW_NED + _vWIND_NED );

  uw=_vWIND_NED(1); vw=_vWIND_NED(2); ww=_vWIND_NED(3);

}

//******************************************************************************

void FGInitialCondition::InitializeIC(void)
{
  vt=vc=ve=vg=0;
  mach=0;
  alpha=beta=gamma=0;
  theta=phi=psi=0;
  altitudeASL=hdot=0;
  latitude=longitude=0;
  u=v=w=0;
  p=q=r=0;
  uw=vw=ww=0;
  vnorth=veast=vdown=0;
  wnorth=weast=wdown=0;
  whead=wcross=0;
  wdir=wmag=0;
  lastSpeedSet=setvt;
  lastWindSet=setwned;
  radius_to_vehicle = sea_level_radius = fdmex->GetInertial()->GetRefRadius();
  terrain_elevation = 0;

  targetNlfIC = 1.0;

  salpha=sbeta=stheta=sphi=spsi=sgamma=0;
  calpha=cbeta=ctheta=cphi=cpsi=cgamma=1;
}

//******************************************************************************

void FGInitialCondition::WriteStateFile(int num)
{
  if (Constructing) return;

  string filename = fdmex->GetFullAircraftPath();

  if (filename.empty())
    filename = "initfile.xml";
  else
    filename.append("/initfile.xml");
  
  ofstream outfile(filename.c_str());
  FGPropagate* Propagate = fdmex->GetPropagate();
  
  if (outfile.is_open()) {
    outfile << "<?xml version=\"1.0\"?>" << endl;
    outfile << "<initialize name=\"reset00\">" << endl;
    outfile << "  <ubody unit=\"FT/SEC\"> " << Propagate->GetUVW(eX) << " </ubody> " << endl;
    outfile << "  <vbody unit=\"FT/SEC\"> " << Propagate->GetUVW(eY) << " </vbody> " << endl;
    outfile << "  <wbody unit=\"FT/SEC\"> " << Propagate->GetUVW(eZ) << " </wbody> " << endl;
    outfile << "  <phi unit=\"DEG\"> " << Propagate->GetEuler(ePhi) << " </phi>" << endl;
    outfile << "  <theta unit=\"DEG\"> " << Propagate->GetEuler(eTht) << " </theta>" << endl;
    outfile << "  <psi unit=\"DEG\"> " << Propagate->GetEuler(ePsi) << " </psi>" << endl;
    outfile << "  <longitude unit=\"DEG\"> " << Propagate->GetLongitudeDeg() << " </longitude>" << endl;
    outfile << "  <latitude unit=\"DEG\"> " << Propagate->GetLatitudeDeg() << " </latitude>" << endl;
    outfile << "  <altitude unit=\"FT\"> " << Propagate->GetAltitudeASL() << " </altitude>" << endl;
    outfile << "</initialize>" << endl;
    outfile.close();
  } else {
    cerr << "Could not open and/or write the state to the initial conditions file: " << filename << endl;
  }
}

//******************************************************************************

void FGInitialCondition::SetVcalibratedKtsIC(double tt) {

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

void FGInitialCondition::SetVequivalentKtsIC(double tt) {
  ve=tt*ktstofps;
  lastSpeedSet=setve;
  vt=ve*1/sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
  mach=vt/fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
}

//******************************************************************************

void FGInitialCondition::SetVgroundFpsIC(double tt) {
  double ua,va,wa;
  double vxz;

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

void FGInitialCondition::SetVtrueFpsIC(double tt) {
  vt=tt;
  lastSpeedSet=setvt;
  mach=vt/fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
  ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
}

//******************************************************************************

void FGInitialCondition::SetMachIC(double tt) {
  mach=tt;
  lastSpeedSet=setmach;
  vt=mach*fdmex->GetAtmosphere()->GetSoundSpeed();
  vc=calcVcas(mach);
  ve=vt*sqrt(fdmex->GetAtmosphere()->GetDensityRatio());
  //cout << "Vt: " << vt*fpstokts << " Vc: " << vc*fpstokts << endl;
}

//******************************************************************************

void FGInitialCondition::SetClimbRateFpmIC(double tt) {
  SetClimbRateFpsIC(tt/60.0);
}

//******************************************************************************

void FGInitialCondition::SetClimbRateFpsIC(double tt) {

  if(vt > 0.1) {
    hdot=tt;
    gamma=asin(hdot/vt);
    sgamma=sin(gamma); cgamma=cos(gamma);
  }
}

//******************************************************************************

void FGInitialCondition::SetFlightPathAngleRadIC(double tt) {
  gamma=tt;
  sgamma=sin(gamma); cgamma=cos(gamma);
  getTheta();
  hdot=vt*sgamma;
}

//******************************************************************************

void FGInitialCondition::SetAlphaRadIC(double tt) {
  alpha=tt;
  salpha=sin(alpha); calpha=cos(alpha);
  getTheta();
}

//******************************************************************************

void FGInitialCondition::SetThetaRadIC(double tt) {
  theta=tt;
  stheta=sin(theta); ctheta=cos(theta);
  getAlpha();
}

//******************************************************************************

void FGInitialCondition::SetBetaRadIC(double tt) {
  beta=tt;
  sbeta=sin(beta); cbeta=cos(beta);
  getTheta();

}

//******************************************************************************

void FGInitialCondition::SetPhiRadIC(double tt) {
  phi=tt;
  sphi=sin(phi); cphi=cos(phi);
  getTheta();
}

//******************************************************************************

void FGInitialCondition::SetPsiRadIC(double tt) {
    psi=tt;
    spsi=sin(psi); cpsi=cos(psi);
    calcWindUVW();
}

//******************************************************************************

void FGInitialCondition::SetUBodyFpsIC(double tt) {
  u=tt;
  vt=sqrt(u*u + v*v + w*w);
  lastSpeedSet=setuvw;
}

//******************************************************************************

void FGInitialCondition::SetVBodyFpsIC(double tt) {
  v=tt;
  vt=sqrt(u*u + v*v + w*w);
  lastSpeedSet=setuvw;
}

//******************************************************************************

void FGInitialCondition::SetWBodyFpsIC(double tt) {
  w=tt;
  vt=sqrt( u*u + v*v + w*w );
  lastSpeedSet=setuvw;
}


//******************************************************************************

void FGInitialCondition::SetVNorthFpsIC(double tt) {
  double ua,va,wa;
  double vxz;
  vnorth = tt;
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
  lastSpeedSet=setned;
}

//******************************************************************************

void FGInitialCondition::SetVEastFpsIC(double tt) {
  double ua,va,wa;
  double vxz;
  veast = tt;
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
  lastSpeedSet=setned;
}

//******************************************************************************

void FGInitialCondition::SetVDownFpsIC(double tt) {
  double ua,va,wa;
  double vxz;
  vdown = tt;
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
  SetClimbRateFpsIC(-1*vdown);
  lastSpeedSet=setned;
}

//******************************************************************************

double FGInitialCondition::GetUBodyFpsIC(void) const {
    if (lastSpeedSet == setvg || lastSpeedSet == setned)
      return u;
    else
      return vt*calpha*cbeta - uw;
}

//******************************************************************************

double FGInitialCondition::GetVBodyFpsIC(void) const {
    if (lastSpeedSet == setvg || lastSpeedSet == setned)
      return v;
    else {
      return vt*sbeta - vw;
    }
}

//******************************************************************************

double FGInitialCondition::GetWBodyFpsIC(void) const {
    if (lastSpeedSet == setvg || lastSpeedSet == setned)
      return w;
    else
      return vt*salpha*cbeta -ww;
}

//******************************************************************************

void FGInitialCondition::SetWindNEDFpsIC(double wN, double wE, double wD ) {
  wnorth = wN; weast = wE; wdown = wD;
  lastWindSet = setwned;
  calcWindUVW();
  if(lastSpeedSet == setvg)
    SetVgroundFpsIC(vg);
}

//******************************************************************************

void FGInitialCondition::SetCrossWindKtsIC(double cross){
    wcross=cross*ktstofps;
    lastWindSet=setwhc;
    calcWindUVW();
    if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);

}

//******************************************************************************

// positive from left
void FGInitialCondition::SetHeadWindKtsIC(double head){
    whead=head*ktstofps;
    lastWindSet=setwhc;
    calcWindUVW();
    if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);

}

//******************************************************************************

void FGInitialCondition::SetWindDownKtsIC(double wD) {
    wdown=wD;
    calcWindUVW();
    if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);
}

//******************************************************************************

void FGInitialCondition::SetWindMagKtsIC(double mag) {
  wmag=mag*ktstofps;
  lastWindSet=setwmd;
  calcWindUVW();
  if(lastSpeedSet == setvg)
      SetVgroundFpsIC(vg);
}

//******************************************************************************

void FGInitialCondition::SetWindDirDegIC(double dir) {
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
      case setwned:
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
          << uw << ", " << vw << ", " << ww << endl; */

}

//******************************************************************************

void FGInitialCondition::SetAltitudeASLFtIC(double tt)
{
  altitudeASL=tt;
  fdmex->GetPropagate()->SetAltitudeASL(altitudeASL);
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

void FGInitialCondition::SetAltitudeAGLFtIC(double tt)
{
  SetAltitudeASLFtIC(terrain_elevation + tt);
}

//******************************************************************************

void FGInitialCondition::SetSeaLevelRadiusFtIC(double tt)
{
  sea_level_radius = tt;
}

//******************************************************************************

void FGInitialCondition::SetTerrainElevationFtIC(double tt)
{
  terrain_elevation=tt;
}

//******************************************************************************

void FGInitialCondition::calcUVWfromNED(void)
{
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

bool FGInitialCondition::getMachFromVcas(double *Mach,double vcas) {

  bool result=false;
  double guess=1.5;
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
  double guess=theta-gamma;

  if(vt < 0.01) return 0;

  xlo=xhi=0;
  xmin=fdmex->GetAerodynamics()->GetAlphaCLMin();
  xmax=fdmex->GetAerodynamics()->GetAlphaCLMax();
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
  double guess=alpha+gamma;

  if(vt < 0.01) return 0;

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

double FGInitialCondition::GammaEqOfTheta(double Theta) {
  double a,b,c;
  double sTheta,cTheta;

  //theta=Theta; stheta=sin(theta); ctheta=cos(theta);
  sTheta=sin(Theta); cTheta=cos(Theta);
  calcWindUVW();
  a=wdown + vt*calpha*cbeta + uw;
  b=vt*sphi*sbeta + vw*sphi;
  c=vt*cphi*salpha*cbeta + ww*cphi;
  return vt*sgamma - ( a*sTheta - (b+c)*cTheta);
}

//******************************************************************************

double FGInitialCondition::GammaEqOfAlpha(double Alpha) {
  double a,b,c;
  double sAlpha,cAlpha;
  sAlpha=sin(Alpha); cAlpha=cos(Alpha);
  a=wdown + vt*cAlpha*cbeta + uw;
  b=vt*sphi*sbeta + vw*sphi;
  c=vt*cphi*sAlpha*cbeta + ww*cphi;

  return vt*sgamma - ( a*stheta - (b+c)*ctheta );
}

//******************************************************************************

double FGInitialCondition::calcVcas(double Mach) {

  double p=fdmex->GetAtmosphere()->GetPressure();
  double psl=fdmex->GetAtmosphere()->GetPressureSL();
  double rhosl=fdmex->GetAtmosphere()->GetDensitySL();
  double pt,A,B,D,vcas;
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

bool FGInitialCondition::findInterval(double x,double guess) {
  //void find_interval(inter_params &ip,eqfunc f,double y,double constant, int &flag){

  int i=0;
  bool found=false;
  double flo,fhi,fguess;
  double lo,hi,step;
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

bool FGInitialCondition::solve(double *y,double x)
{
  double x1,x2,x3,f1,f2,f3,d,d0;
  double eps=1E-5;
  double const relax =0.9;
  int i;
  bool success=false;

  //initializations
  d=1;
  x2 = 0;
  x1=xlo;x3=xhi;
  f1=(this->*sfunc)(x1)-x;
  f3=(this->*sfunc)(x3)-x;
  d0=fabs(x3-x1);

  //iterations
  i=0;
  while ((fabs(d) > eps) && (i < 100)) {
    d=(x3-x1)/d0;
    x2 = x1-d*d0*f1/(f3-f1);

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

double FGInitialCondition::GetWindDirDegIC(void) const {
  if(weast != 0.0)
    return atan2(weast,wnorth)*radtodeg;
  else if(wnorth > 0)
    return 0.0;
  else
    return 180.0;
}

//******************************************************************************

bool FGInitialCondition::Load(string rstfile, bool useStoredPath)
{
  int n;

  string sep = "/";
  if( useStoredPath ) {
    init_file_name = fdmex->GetFullAircraftPath() + sep + rstfile + ".xml";
  } else {
    init_file_name = rstfile;
  }

  document = LoadXMLDocument(init_file_name);

  // Make sure that the document is valid
  if (!document) {
    cerr << "File: " << init_file_name << " could not be read." << endl;
    exit(-1);
  }

  if (document->GetName() != string("initialize")) {
    cerr << "File: " << init_file_name << " is not a reset file." << endl;
    exit(-1);
  }

  if (document->FindElement("latitude"))
    SetLatitudeDegIC(document->FindElementValueAsNumberConvertTo("latitude", "DEG"));
  if (document->FindElement("longitude"))
    SetLongitudeDegIC(document->FindElementValueAsNumberConvertTo("longitude", "DEG"));
  if (document->FindElement("elevation"))
    SetTerrainElevationFtIC(document->FindElementValueAsNumberConvertTo("elevation", "FT"));
  if (document->FindElement("altitude")) // This is feet above ground level
    SetAltitudeAGLFtIC(document->FindElementValueAsNumberConvertTo("altitude", "FT"));
  if (document->FindElement("ubody"))
    SetUBodyFpsIC(document->FindElementValueAsNumberConvertTo("ubody", "FT/SEC"));
  if (document->FindElement("vbody"))
    SetVBodyFpsIC(document->FindElementValueAsNumberConvertTo("vbody", "FT/SEC"));
  if (document->FindElement("wbody"))
    SetWBodyFpsIC(document->FindElementValueAsNumberConvertTo("wbody", "FT/SEC"));
  if (document->FindElement("vnorth"))
    SetVNorthFpsIC(document->FindElementValueAsNumberConvertTo("vnorth", "FT/SEC"));
  if (document->FindElement("veast"))
    SetVEastFpsIC(document->FindElementValueAsNumberConvertTo("veast", "FT/SEC"));
  if (document->FindElement("vdown"))
    SetVDownFpsIC(document->FindElementValueAsNumberConvertTo("vdown", "FT/SEC"));
  if (document->FindElement("winddir"))
    SetWindDirDegIC(document->FindElementValueAsNumberConvertTo("winddir", "DEG"));
  if (document->FindElement("vwind"))
    SetWindMagKtsIC(document->FindElementValueAsNumberConvertTo("vwind", "KTS"));
  if (document->FindElement("hwind"))
    SetHeadWindKtsIC(document->FindElementValueAsNumberConvertTo("hwind", "KTS"));
  if (document->FindElement("xwind"))
    SetCrossWindKtsIC(document->FindElementValueAsNumberConvertTo("xwind", "KTS"));
  if (document->FindElement("vc"))
    SetVcalibratedKtsIC(document->FindElementValueAsNumberConvertTo("vc", "KTS"));
  if (document->FindElement("vt"))
    SetVtrueKtsIC(document->FindElementValueAsNumberConvertTo("vt", "KTS"));
  if (document->FindElement("mach"))
    SetMachIC(document->FindElementValueAsNumber("mach"));
  if (document->FindElement("phi"))
    SetPhiDegIC(document->FindElementValueAsNumberConvertTo("phi", "DEG"));
  if (document->FindElement("theta"))
    SetThetaDegIC(document->FindElementValueAsNumberConvertTo("theta", "DEG"));
  if (document->FindElement("psi"))
    SetPsiDegIC(document->FindElementValueAsNumberConvertTo("psi", "DEG"));
  if (document->FindElement("alpha"))
    SetAlphaDegIC(document->FindElementValueAsNumberConvertTo("alpha", "DEG"));
  if (document->FindElement("beta"))
    SetBetaDegIC(document->FindElementValueAsNumberConvertTo("beta", "DEG"));
  if (document->FindElement("gamma"))
    SetFlightPathAngleDegIC(document->FindElementValueAsNumberConvertTo("gamma", "DEG"));
  if (document->FindElement("roc"))
    SetClimbRateFpsIC(document->FindElementValueAsNumberConvertTo("roc", "FT/SEC"));
  if (document->FindElement("vground"))
    SetVgroundKtsIC(document->FindElementValueAsNumberConvertTo("vground", "KTS"));
  if (document->FindElement("targetNlf"))
  {
    SetTargetNlfIC(document->FindElementValueAsNumber("targetNlf"));
  }

  // Check to see if any engines are specified to be initialized in a running state
  FGPropulsion* propulsion = fdmex->GetPropulsion();
  Element* running_elements = document->FindElement("running");
  while (running_elements) {
    n = int(running_elements->GetDataAsNumber());
    propulsion->InitRunning(n);
    running_elements = document->FindNextElement("running");
  }

  fdmex->RunIC();

  return true;
}

//******************************************************************************

void FGInitialCondition::bind(void){
  PropertyManager->Tie("ic/vc-kts", this,
                       &FGInitialCondition::GetVcalibratedKtsIC,
                       &FGInitialCondition::SetVcalibratedKtsIC,
                       true);
  PropertyManager->Tie("ic/ve-kts", this,
                       &FGInitialCondition::GetVequivalentKtsIC,
                       &FGInitialCondition::SetVequivalentKtsIC,
                       true);
  PropertyManager->Tie("ic/vg-kts", this,
                       &FGInitialCondition::GetVgroundKtsIC,
                       &FGInitialCondition::SetVgroundKtsIC,
                       true);
  PropertyManager->Tie("ic/vt-kts", this,
                       &FGInitialCondition::GetVtrueKtsIC,
                       &FGInitialCondition::SetVtrueKtsIC,
                       true);
  PropertyManager->Tie("ic/mach", this,
                       &FGInitialCondition::GetMachIC,
                       &FGInitialCondition::SetMachIC,
                       true);
  PropertyManager->Tie("ic/roc-fpm", this,
                       &FGInitialCondition::GetClimbRateFpmIC,
                       &FGInitialCondition::SetClimbRateFpmIC,
                       true);
  PropertyManager->Tie("ic/gamma-deg", this,
                       &FGInitialCondition::GetFlightPathAngleDegIC,
                       &FGInitialCondition::SetFlightPathAngleDegIC,
                       true);
  PropertyManager->Tie("ic/alpha-deg", this,
                       &FGInitialCondition::GetAlphaDegIC,
                       &FGInitialCondition::SetAlphaDegIC,
                       true);
  PropertyManager->Tie("ic/beta-deg", this,
                       &FGInitialCondition::GetBetaDegIC,
                       &FGInitialCondition::SetBetaDegIC,
                       true);
  PropertyManager->Tie("ic/theta-deg", this,
                       &FGInitialCondition::GetThetaDegIC,
                       &FGInitialCondition::SetThetaDegIC,
                       true);
  PropertyManager->Tie("ic/phi-deg", this,
                       &FGInitialCondition::GetPhiDegIC,
                       &FGInitialCondition::SetPhiDegIC,
                       true);
  PropertyManager->Tie("ic/psi-true-deg", this,
                       &FGInitialCondition::GetPsiDegIC );
  PropertyManager->Tie("ic/lat-gc-deg", this,
                       &FGInitialCondition::GetLatitudeDegIC,
                       &FGInitialCondition::SetLatitudeDegIC,
                       true);
  PropertyManager->Tie("ic/long-gc-deg", this,
                       &FGInitialCondition::GetLongitudeDegIC,
                       &FGInitialCondition::SetLongitudeDegIC,
                       true);
  PropertyManager->Tie("ic/h-sl-ft", this,
                       &FGInitialCondition::GetAltitudeASLFtIC,
                       &FGInitialCondition::SetAltitudeASLFtIC,
                       true);
  PropertyManager->Tie("ic/h-agl-ft", this,
                       &FGInitialCondition::GetAltitudeAGLFtIC,
                       &FGInitialCondition::SetAltitudeAGLFtIC,
                       true);
  PropertyManager->Tie("ic/sea-level-radius-ft", this,
                       &FGInitialCondition::GetSeaLevelRadiusFtIC,
                       &FGInitialCondition::SetSeaLevelRadiusFtIC,
                       true);
  PropertyManager->Tie("ic/terrain-elevation-ft", this,
                       &FGInitialCondition::GetTerrainElevationFtIC,
                       &FGInitialCondition::SetTerrainElevationFtIC,
                       true);
  PropertyManager->Tie("ic/vg-fps", this,
                       &FGInitialCondition::GetVgroundFpsIC,
                       &FGInitialCondition::SetVgroundFpsIC,
                       true);
  PropertyManager->Tie("ic/vt-fps", this,
                       &FGInitialCondition::GetVtrueFpsIC,
                       &FGInitialCondition::SetVtrueFpsIC,
                       true);
  PropertyManager->Tie("ic/vw-bx-fps", this,
                       &FGInitialCondition::GetWindUFpsIC);
  PropertyManager->Tie("ic/vw-by-fps", this,
                       &FGInitialCondition::GetWindVFpsIC);
  PropertyManager->Tie("ic/vw-bz-fps", this,
                       &FGInitialCondition::GetWindWFpsIC);
  PropertyManager->Tie("ic/vw-north-fps", this,
                       &FGInitialCondition::GetWindNFpsIC);
  PropertyManager->Tie("ic/vw-east-fps", this,
                       &FGInitialCondition::GetWindEFpsIC);
  PropertyManager->Tie("ic/vw-down-fps", this,
                       &FGInitialCondition::GetWindDFpsIC);
  PropertyManager->Tie("ic/vw-mag-fps", this,
                       &FGInitialCondition::GetWindFpsIC);
  PropertyManager->Tie("ic/vw-dir-deg", this,
                       &FGInitialCondition::GetWindDirDegIC,
                       &FGInitialCondition::SetWindDirDegIC,
                       true);

  PropertyManager->Tie("ic/roc-fps", this,
                       &FGInitialCondition::GetClimbRateFpsIC,
                       &FGInitialCondition::SetClimbRateFpsIC,
                       true);
  PropertyManager->Tie("ic/u-fps", this,
                       &FGInitialCondition::GetUBodyFpsIC,
                       &FGInitialCondition::SetUBodyFpsIC,
                       true);
  PropertyManager->Tie("ic/v-fps", this,
                       &FGInitialCondition::GetVBodyFpsIC,
                       &FGInitialCondition::SetVBodyFpsIC,
                       true);
  PropertyManager->Tie("ic/w-fps", this,
                       &FGInitialCondition::GetWBodyFpsIC,
                       &FGInitialCondition::SetWBodyFpsIC,
                       true);
  PropertyManager->Tie("ic/vn-fps", this,
                       &FGInitialCondition::GetVNorthFpsIC,
                       &FGInitialCondition::SetVNorthFpsIC,
                       true);
  PropertyManager->Tie("ic/ve-fps", this,
                       &FGInitialCondition::GetVEastFpsIC,
                       &FGInitialCondition::SetVEastFpsIC,
                       true);
  PropertyManager->Tie("ic/vd-fps", this,
                       &FGInitialCondition::GetVDownFpsIC,
                       &FGInitialCondition::SetVDownFpsIC,
                       true);
  PropertyManager->Tie("ic/gamma-rad", this,
                       &FGInitialCondition::GetFlightPathAngleRadIC,
                       &FGInitialCondition::SetFlightPathAngleRadIC,
                       true);
  PropertyManager->Tie("ic/alpha-rad", this,
                       &FGInitialCondition::GetAlphaRadIC,
                       &FGInitialCondition::SetAlphaRadIC,
                       true);
  PropertyManager->Tie("ic/theta-rad", this,
                       &FGInitialCondition::GetThetaRadIC,
                       &FGInitialCondition::SetThetaRadIC,
                       true);
  PropertyManager->Tie("ic/beta-rad", this,
                       &FGInitialCondition::GetBetaRadIC,
                       &FGInitialCondition::SetBetaRadIC,
                       true);
  PropertyManager->Tie("ic/phi-rad", this,
                       &FGInitialCondition::GetPhiRadIC,
                       &FGInitialCondition::SetPhiRadIC,
                       true);
  PropertyManager->Tie("ic/psi-true-rad", this,
                       &FGInitialCondition::GetPsiRadIC);
  PropertyManager->Tie("ic/lat-gc-rad", this,
                       &FGInitialCondition::GetLatitudeRadIC,
                       &FGInitialCondition::SetLatitudeRadIC,
                       true);
  PropertyManager->Tie("ic/long-gc-rad", this,
                       &FGInitialCondition::GetLongitudeRadIC,
                       &FGInitialCondition::SetLongitudeRadIC,
                       true);
  PropertyManager->Tie("ic/p-rad_sec", this,
                       &FGInitialCondition::GetPRadpsIC,
                       &FGInitialCondition::SetPRadpsIC,
                       true);
  PropertyManager->Tie("ic/q-rad_sec", this,
                       &FGInitialCondition::GetQRadpsIC,
                       &FGInitialCondition::SetQRadpsIC,
                       true);
  PropertyManager->Tie("ic/r-rad_sec", this,
                       &FGInitialCondition::GetRRadpsIC,
                       &FGInitialCondition::SetRRadpsIC,
                       true);

  typedef int (FGInitialCondition::*iPMF)(void) const;
  PropertyManager->Tie("simulation/write-state-file",
                       this,
                       (iPMF)0,
                       &FGInitialCondition::WriteStateFile);

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGInitialCondition::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGInitialCondition" << endl;
    if (from == 1) cout << "Destroyed:    FGInitialCondition" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}
