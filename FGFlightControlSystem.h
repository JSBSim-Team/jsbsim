/*******************************************************************************
 
 Header:       FGFlightControlSystem.h
 Author:       Tony Peden
 Date started: 12/20/99
 
 ------------- Copyright (C) 1999  Tony Peden (apeden@earthlink.net) -------------
 
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
 */

#ifndef FGFLIGHTCONTROLSYSTEM_H
#define FGFLIGHTCONTROLSYSTEM_H

using namespace std;

#include "FGModel.h"
#include "FGFDMExec.h"
#include "Filters/FGFilter.h"
//#include "FGControlLaw.h"
#include <fstream>
#include <map>
#include <string>



#define MAXCONTROLS 16
#define MAXSURFACES 16
#define MAXFILTERS  32


#define FGFCS_UNKNOWN       -1
#define FGFCS_DEFAULT        0
#define FGFCS_MINMAX         1
#define FGFCS_RATEMAX        2
#define FGFCS_RATEMAX_VECTOR 4
#define FGFCS_INCREMENT      5

#define fcs_undefined -1
#define fcs_elevator 0
#define fcs_aileron 1
#define fcs_rudder 2
#define fcs_spoilerl 3
#define fcs_spoilerr 4
#define fcs_flaps 5
#define fcs_slats 6
#define fcs_kruegers 7
#define fcs_speedbrakes 8
#define fcs_wingsweep 9
#define fcs_hstab 10


#define fcs_column 0
#define fcs_wheel 1
#define fcs_pedal 2
#define fcs_flaph 3
#define fcs_slath 4
#define fcs_kruegerh 5
#define fcs_speedbrakeh 6
#define fcs_wingsweeph 7
#define fcs_hstabh 8
#define fcs_pitchtrimh 9
#define fcs_rolltrimh 10
#define fcs_yawtrimh 11

typedef map<string, int> SystemMap;

class FGFilter;
class FGFDMExec;

class FGFlightControlSystem : public FGModel {
public:
  FGFlightControlSystem(FGFDMExec* tt);

  ~FGFlightControlSystem();

  bool Run(void);

  bool LoadControlSystem(ifstream &in);

  //generic
  inline void  SetFCSControl(int which, float tt, int n=0) {
    controls[which]=tt;
  }
  inline float GetFCSControl(const int which, int n=0) {
    return controls[which];
  }

  inline float GetFCSSurface(const int which, int n=0) {
    return surfaces[which];
  }
  inline void  SetFCSSurface(const int which, float tt, int n=0) {
    surfaces[which]=tt;
  }

private:
  float controls[MAXCONTROLS];
  float surfaces[MAXSURFACES];
  int binding[MAXSURFACES];
  FGFilter* fcslist[MAXSURFACES];


  SystemMap systemmap;

};

#endif
