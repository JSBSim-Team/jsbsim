/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGAccelerometer.cpp
 Author:       Jon Berndt
 Date started: 9 July 2005

 ------------- Copyright (C) 2005 -------------

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

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGAccelerometer.h"

namespace JSBSim {

static const char *IdSrc = "$Id: FGAccelerometer.cpp,v 1.1 2009/05/08 11:58:10 jberndt Exp $";
static const char *IdHdr = ID_ACCELEROMETER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGAccelerometer::FGAccelerometer(FGFCS* fcs, Element* element) : FGSensor(fcs, element)
{
  Propagate = fcs->GetExec()->GetPropagate();
  MassBalance = fcs->GetExec()->GetMassBalance();
  
  Element* location_element = element->FindElement("location");
  if (location_element) vLocation = location_element->FindElementTripletConvertTo("IN");
  else {cerr << "No location given for accelerometer. " << endl; exit(-1);}

  vRadius = MassBalance->StructuralToBody(vLocation);

  Element* orient_element = element->FindElement("orientation");
  if (orient_element) vOrient = orient_element->FindElementTripletConvertTo("RAD");
  else {cerr << "No orientation given for accelerometer. " << endl;}

  Element* axis_element = element->FindElement("axis");
  if (axis_element) {
    string sAxis = element->FindElementValue("axis");
    if (sAxis == "X" || sAxis == "x") {
      axis = 1;
    } else if (sAxis == "Y" || sAxis == "y") {
      axis = 2;
    } else if (sAxis == "Z" || sAxis == "z") {
      axis = 3;
    } else {
      cerr << "  Incorrect/no axis specified for accelerometer; assuming X axis" << endl;
      axis = 1;
    }
  }

  CalculateTransformMatrix();

  Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGAccelerometer::~FGAccelerometer()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGAccelerometer::Run(void )
{
  // There is no input assumed. This is a dedicated acceleration sensor.
  
  vRadius = MassBalance->StructuralToBody(vLocation);

  vAccel = mT * (Propagate->GetUVWdot()
                 + Propagate->GetPQRdot() * vRadius
                 + Propagate->GetPQR() * (Propagate->GetPQR() * vRadius));
  
  Input = vAccel(axis);

  Output = Input; // perfect accelerometer

  // Degrade signal as specified

  if (fail_stuck) {
    Output = PreviousOutput;
    return true;
  }

  if (lag != 0.0)            Lag();       // models accelerometer lag
  if (noise_variance != 0.0) Noise();     // models noise
  if (drift_rate != 0.0)     Drift();     // models drift over time
  if (bias != 0.0)           Bias();      // models a finite bias

  if (fail_low)  Output = -HUGE_VAL;
  if (fail_high) Output =  HUGE_VAL;

  if (bits != 0)             Quantize();  // models quantization degradation
//  if (delay != 0.0)          Delay();     // models system signal transport latencies

  Clip(); // Is it right to clip an accelerometer?
  return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGAccelerometer::CalculateTransformMatrix(void)
{
  double cp,sp,cr,sr,cy,sy;

  cp=cos(vOrient(ePitch)); sp=sin(vOrient(ePitch));
  cr=cos(vOrient(eRoll));  sr=sin(vOrient(eRoll));
  cy=cos(vOrient(eYaw));   sy=sin(vOrient(eYaw));

  mT(1,1) =  cp*cy;
  mT(1,2) =  cp*sy;
  mT(1,3) = -sp;

  mT(2,1) = sr*sp*cy - cr*sy;
  mT(2,2) = sr*sp*sy + cr*cy;
  mT(2,3) = sr*cp;

  mT(3,1) = cr*sp*cy + sr*sy;
  mT(3,2) = cr*sp*sy - sr*cy;
  mT(3,3) = cr*cp;
  
  // This transform is different than for FGForce, where we want a native nozzle
  // force in body frame. Here we calculate the body frame accel and want it in
  // the transformed accelerometer frame. So, the next line is commented out.
  // mT = mT.Inverse();
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

void FGAccelerometer::Debug(int from)
{
  string ax[4] = {"none", "X", "Y", "Z"};

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      cout << "        Axis: " << ax[axis] << endl;
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGAccelerometer" << endl;
    if (from == 1) cout << "Destroyed:    FGAccelerometer" << endl;
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
