// FGMSIS test program


#include "FGMSIS.h"
#include <iostream>
using namespace std;
using namespace JSBSim;


int main(void) {

  int day = 1;             // January first UTC
  double sec = 43200.0;    // noon UTC
  double alt = 1;          // altitude, kilometers
  double lat = 37.61833;   // latitude, KSFO
  double lon = -122.375;   // longitude, KSFO
  double lst = 12.0;       // local solar time, hrs   
 
  MSIS* msis = new MSIS();
  msis->InitModel();
  msis->Calculate(day, sec, alt, lat, lon, lst);

  cout << "Temperature (K):   " << msis->GetTemperature() << "\n";
  cout << "Density (gm/cm^3): " << msis->GetDensity() << "\n";
  
  delete msis;
  return 0;
}
