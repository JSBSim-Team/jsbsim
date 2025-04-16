/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGMagnetometer.cpp
 Author:       Matthew Chave
 Date started: August 2009

 ------------- Copyright (C) 2009 -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option) any
 later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be
 found on the world wide web at http://www.gnu.org.

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

#include "FGMagnetometer.h"
#include "simgear/magvar/coremag.hxx"
#include "models/FGFCS.h"
#include "models/FGMassBalance.h"

using namespace std;

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/


FGMagnetometer::FGMagnetometer(FGFCS* fcs, Element* element)
  : FGSensor(fcs, element),
    FGSensorOrientation(element, fcs->GetExec()->GetLogger()),
    counter(0), INERTIAL_UPDATE_RATE(1000)
{
  Propagate = fcs->GetExec()->GetPropagate();
  MassBalance = fcs->GetExec()->GetMassBalance();
  Inertial = fcs->GetExec()->GetInertial();

  Element* location_element = element->FindElement("location");
  if (location_element)
    vLocation = location_element->FindElementTripletConvertTo("IN");
  else {
    XMLLogException err(fcs->GetExec()->GetLogger(), element);
    err << "No location given for magnetometer.\n";
    throw err;
  }

  vRadius = MassBalance->StructuralToBody(vLocation);

  //assuming date wont significantly change over a flight to affect mag field
  //would be better to get the date from the sim if its simulated...
  time_t rawtime;
  time( &rawtime );
  struct tm ptm;
  #if defined(_MSC_VER) || defined(__MINGW32__)
  gmtime_s(&ptm, &rawtime);
  #else
  gmtime_r(&rawtime, &ptm);
  #endif

  int year = ptm.tm_year;
  if(year>100)
  {
    year-= 100;
  }
  //the months here are zero based TODO find out if the function expects 1s based
  date = (yymmdd_to_julian_days(ptm.tm_year, ptm.tm_mon, ptm.tm_mday)); //Julian 1950-2049 yy,mm,dd
  updateInertialMag();

  Debug(0);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGMagnetometer::~FGMagnetometer()
{
  Debug(1);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMagnetometer::ResetPastStates(void)
{
  FGSensor::ResetPastStates();
  counter = 0;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGMagnetometer::updateInertialMag(void)
{
  if (counter++ % INERTIAL_UPDATE_RATE == 0)//dont need to update every iteration
  {
    usedLat = (Propagate->GetGeodLatitudeRad());//radians, N and E lat and long are positive, S and W negative
    usedLon = (Propagate->GetLongitude());//radians
    usedAlt = (Propagate->GetGeodeticAltitude()*fttom*0.001);//km

    //this should be done whenever the position changes significantly (in nTesla)
    calc_magvar( usedLat, usedLon, usedAlt, date, field );
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGMagnetometer::Run(void )
{
  // There is no input assumed. This is a dedicated magnetic field sensor.

  vRadius = MassBalance->StructuralToBody(vLocation);

  updateInertialMag();

  // Inertial magnetic field rotated to the body frame
  vMag = Propagate->GetTl2b() * FGColumnVector3(field[3], field[4], field[5]);

  // Allow for sensor orientation
  vMag = mT * vMag;

  Input = vMag(axis);

  ProcessSensorSignal();

  SetOutput();

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
//    1: This value explicitly requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGMagnetometer::Debug(int from)
{
  string ax[4] = {"none", "X", "Y", "Z"};

  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) { // Constructor
      log << "        Axis: " << ax[axis] << "\n";
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    FGLogging log(fcs->GetExec()->GetLogger(), LogLevel::DEBUG);
    if (from == 0) log << "Instantiated: FGMagnetometer\n";
    if (from == 1) log << "Destroyed:    FGMagnetometer\n";
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
    }
  }
}
}
