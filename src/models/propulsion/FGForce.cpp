/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Source:       FGForce.cpp
 Author:       Tony Peden
 Date started: 6/10/00

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
6/10/00  TP   Created


FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

The purpose of this class is to provide storage for computed forces and
encapsulate all the functionality associated with transforming those
forces from their native coord system to the body system.  This includes
computing the moments due to the difference between the point of application
and the cg.

*/

#include <iostream>
#include <cstdlib>

#include "FGForce.h"
#include "FGFDMExec.h"
#include "models/FGPropagate.h"
#include "models/FGMassBalance.h"
#include "models/FGAuxiliary.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGForce.cpp,v 1.17 2011/10/31 14:54:41 bcoconni Exp $";
static const char *IdHdr = ID_FORCE;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGForce::FGForce(FGFDMExec *FDMExec) :
                 fdmex(FDMExec),
                 ttype(tNone)
{
  vFn.InitMatrix();
  vMn.InitMatrix();
  vH.InitMatrix();
  vOrient.InitMatrix();
  vXYZn.InitMatrix();
  vActingXYZn.InitMatrix();

  vFb.InitMatrix();
  vM.InitMatrix();
  vDXYZ.InitMatrix();

  mT.InitMatrix(1., 0., 0.,
                0., 1., 0.,
                0., 0., 1.);

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGForce::~FGForce()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGColumnVector3& FGForce::GetBodyForces(void)
{
  vFb = Transform()*vFn;

  // Find the distance from this vector's acting location to the cg; this
  // needs to be done like this to convert from structural to body coords.
  // CG and RP values are in inches

  vDXYZ = fdmex->GetMassBalance()->StructuralToBody(vActingXYZn);

  vM = vMn + vDXYZ*vFb;

  return vFb;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

const FGMatrix33& FGForce::Transform(void) const
{
  switch(ttype) {
  case tWindBody:
    return fdmex->GetAuxiliary()->GetTw2b();
  case tLocalBody:
    return fdmex->GetPropagate()->GetTl2b();
  case tCustom:
  case tNone:
    return mT;
  default:
    cout << "Unrecognized tranform requested from FGForce::Transform()" << endl;
    exit(1);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGForce::UpdateCustomTransformMatrix(void)
{
  double cp,sp,cr,sr,cy,sy;
  double srsp, crcy, crsy;

  cp=cos(vOrient(ePitch)); sp=sin(vOrient(ePitch));
  cr=cos(vOrient(eRoll));  sr=sin(vOrient(eRoll));
  cy=cos(vOrient(eYaw));   sy=sin(vOrient(eYaw));

  srsp = sr*sp;
  crcy = cr*cy;
  crsy = cr*sy;

  mT(1,1) =  cp*cy;
  mT(2,1) =  cp*sy;
  mT(3,1) = -sp;

  mT(1,2) = srsp*cy - crsy;
  mT(2,2) = srsp*sy + crcy;
  mT(3,2) = sr*cp;

  mT(1,3) = crcy*sp + sr*sy;
  mT(2,3) = crsy*sp - sr*cy;
  mT(3,3) = cr*cp;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGForce::SetAnglesToBody(double broll, double bpitch, double byaw)
{
  if (ttype == tCustom) {
    vOrient(ePitch) = bpitch;
    vOrient(eRoll) = broll;
    vOrient(eYaw) = byaw;

    UpdateCustomTransformMatrix();
  }
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

void FGForce::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor

    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGForce" << endl;
    if (from == 1) cout << "Destroyed:    FGForce" << endl;
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
