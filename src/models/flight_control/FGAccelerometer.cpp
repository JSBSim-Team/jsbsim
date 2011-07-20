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
#include <iostream>
#include <cstdlib>

#include "models/FGPropagate.h"
#include "models/FGAccelerations.h"
#include "models/FGMassBalance.h"
#include "models/FGInertial.h"

using namespace std;

namespace JSBSim {

static const char *IdSrc = "$Id: FGAccelerometer.cpp,v 1.9 2011/07/17 13:51:23 jberndt Exp $";
static const char *IdHdr = ID_ACCELEROMETER;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

FGAccelerometer::FGAccelerometer(FGFCS* fcs, Element* element)
  : FGSensor(fcs, element),
    FGSensorOrientation(element)
{
  Propagate = fcs->GetExec()->GetPropagate();
  Accelerations = fcs->GetExec()->GetAccelerations();
  MassBalance = fcs->GetExec()->GetMassBalance();
  Inertial = fcs->GetExec()->GetInertial();
  
  Element* location_element = element->FindElement("location");
  if (location_element) vLocation = location_element->FindElementTripletConvertTo("IN");
  else {cerr << "No location given for accelerometer. " << endl; exit(-1);}

  vRadius = MassBalance->StructuralToBody(vLocation);

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
    
  //gravitational forces
  vAccel = Propagate->GetTl2b() * FGColumnVector3(0, 0, Inertial->gravity());

  //aircraft forces
  vAccel += (Accelerations->GetUVWdot()
              + Accelerations->GetPQRdot() * vRadius
              + Propagate->GetPQR() * (Propagate->GetPQR() * vRadius));

  // transform to the specified orientation
  vAccel = mT * vAccel;

  Input = vAccel(axis);

  ProcessSensorSignal();

  return true;
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
