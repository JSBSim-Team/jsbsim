/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Source:       FGForce.cpp
 Author:       Tony Peden
 Date started: 6/10/00

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
6/10/00  TP   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

The purpose of this class is to provide storage for computed forces and
encapsulate all the functionality associated with transforming those
forces from their native coord system to the body system.  This includes
computing the moments due to the difference between the point of application
and the cg.

*/

#include "FGFDMExec.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGMatrix.h"
#include "FGDefs.h"
#include "FGForce.h"

static const char *IdSrc = "$Header: /cvsroot/jsbsim/JSBSim/Attic/FGForce.cpp,v 1.11 2001/03/20 16:10:48 jberndt Exp $";
static const char *IdHdr = "ID_FORCE";

extern short debug_lvl;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGForce::FGForce(FGFDMExec *FDMExec) :
    vFn(3),
    vMn(3),
    fdmex(FDMExec),
    vFb(3),
    vM(3),
    vXYZn(3),
    vDXYZ(3),
    mT(3,3),
    vSense(3),
    ttype(tNone)
{
  mT(1,1)=1; //identity matrix
  mT(2,2)=1;
  mT(3,3)=1;
  vSense.InitMatrix(1);
  if (debug_lvl & 2) cout << "Instantiated: FGForce" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGForce::~FGForce()
{
  if (debug_lvl & 2) cout << "Destroyed:    FGForce" << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGColumnVector FGForce::GetBodyForces(void) {

  vFb=Transform()*(vFn.multElementWise(vSense));

  //find the distance from this vector's location to the cg
  //needs to be done like this to convert from structural to body coords
  vDXYZ(1) = -(vXYZn(1) - fdmex->GetAircraft()->GetXYZcg()(1))*INCHTOFT;
  vDXYZ(2) =  (vXYZn(2) - fdmex->GetAircraft()->GetXYZcg()(2))*INCHTOFT;  //cg and rp values are in inches
  vDXYZ(3) = -(vXYZn(3) - fdmex->GetAircraft()->GetXYZcg()(3))*INCHTOFT;

  vM=vMn +vDXYZ*vFb;

  return vFb;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMatrix FGForce::Transform(void) {
  switch(ttype) {
  case tWindBody:
    return fdmex->GetState()->GetTs2b(fdmex->GetTranslation()->Getalpha(),fdmex->GetTranslation()->Getbeta());
  case tLocalBody:
    return fdmex->GetState()->GetTl2b();
  case tCustom:

  case tNone:
    return mT;
  default:
    cout << "Unrecognized tranform requested from FGForce::Transform()" << endl;
    exit(1);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGForce::SetAnglesToBody(float broll, float bpitch, float byaw) {

  if(ttype == tCustom) {
    float cp,sp,cr,sr,cy,sy;

    cp=cos(bpitch); sp=sin(bpitch);
    cr=cos(broll);  sr=sin(broll);
    cy=cos(byaw);   sy=sin(byaw);

    mT(1,1)=cp*cy;
    mT(1,2)=cp*sy;
    mT(1,3)=-1*sp;

    mT(2,1)=sr*sp*cy-cr*sy;
    mT(2,2)=sr*sp*sy+cr*cy;
    mT(2,3)=sr*cp;

    mT(3,1)=cr*sp*cy+sr*sy;
    mT(3,2)=cr*sp*sy-sr*cy;
    mT(3,3)=cr*cp;
  }

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGForce::Debug(void)
{
    //TODO: Add your source code here
}

