/*******************************************************************************
 
 Source:       FGFlightControlSystem.cpp
 Author:       Tony Peden
 Date started: 7/1/99
 
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

#include "FGFlightControlSystem.h"
#include "Filters/FGFilter.h"
#include "Filters/FGFLTMech.h"
#include "FGFDMExec.h"

extern fstream fcslog;

FGFlightControlSystem::FGFlightControlSystem(FGFDMExec *tt) : FGModel(tt) {

  Name="FGFlightControlSystem";

  //set up the default binding (which are currently unchangeable at runtime)
  binding[fcs_undefined]=fcs_undefined;
  binding[fcs_elevator]=fcs_column;
  binding[fcs_aileron]=fcs_wheel;
  binding[fcs_rudder]=fcs_pedal;
  binding[fcs_spoilerl]=fcs_wheel;
  binding[fcs_spoilerr]=fcs_wheel;
  binding[fcs_flaps]=fcs_flaph;
  binding[fcs_slats]=fcs_slath;
  binding[fcs_kruegers]=fcs_kruegerh;
  binding[fcs_speedbrakes]=fcs_speedbrakeh;
  binding[fcs_wingsweep]=fcs_wingsweeph;
  binding[fcs_hstab]=fcs_hstabh;

  //make sure the remaining pointers are pointing to nothing
  //rather than who-knows-what
  for(int i=0;i<MAXSURFACES;i++) {
    fcslist[i]=0;
  }

  //set up default control system
  fcslist[0]=new FGFilter(FDMExec);
  fcslist[1]=new FGFilter(FDMExec);
  fcslist[2]=new FGFilter(FDMExec);
  fcslist[0]->Next=fcslist[1]->Next=fcslist[2]->Next=0;

}


FGFlightControlSystem::~FGFlightControlSystem() {
  int i,j;
  FGFilter *next, *current;
  i=0;
  while((fcslist[i] != 0) && (i < MAXSURFACES)) {
    j=0;
    current=fcslist[i];
    while((current != 0) && (j < MAXFILTERS)) {
      next=current->Next;
      delete current;
      current=next;
      j++;
    }
    if((next != 0)  && (j >= MAXFILTERS))
      cout << "MEMORY LEAK! More than " << MAXFILTERS << " found while deleting filter list " << i << endl;
    i++;
  }
}

bool FGFlightControlSystem::Run(void) {

  float temp;
  int i,j;
  FGFilter* currentFilter;


  fcslog << "Beginning Run() loop" << endl;
  i=0;
  while((fcslist[i] != 0) && (i < MAXSURFACES)) {
    fcslog << "Executing Filters for surface: "<< i << endl;
    j=0;
    temp=controls[binding[i]];
    currentFilter=fcslist[i];
    while((currentFilter != 0) && (j < MAXFILTERS)) {
      fcslog << "Filter: " << j;
      fcslog << ", " << binding[i] << ", " << controls[binding[i]] << endl;
      currentFilter->SetInput(temp);
      currentFilter->Run();
      temp=currentFilter->GetOutput();
      currentFilter=currentFilter->Next;
      j++;
    }
    surfaces[i]=temp;
    i++;
  }
  fcslog << "End Run() loop" << endl;
  return true;
}

bool FGFlightControlSystem::LoadControlSystem(ifstream &in) {
  FGFilter *newFilter,*currentFilter;
  string buffer,openbracket,closebracket,sname;

  systemmap["FCS_ELEVATOR"]=fcs_elevator;
  systemmap["FCS_AILERON"]=fcs_aileron;
  systemmap["FCS_RUDDER"]=fcs_rudder;
  systemmap["FCS_SPOILERL"]=fcs_spoilerl;
  systemmap["FCS_SPOILERR"]=fcs_spoilerr;
  systemmap["FCS_HSTABILIZER"]=fcs_hstab;
  systemmap["FCS_FLAPS"]=fcs_flaps;
  systemmap["FCS_SLATS"]=fcs_slats;
  systemmap["FCS_KRUEGERS"]=fcs_kruegers;
  systemmap["FCS_SPEEDBRAKES"]=fcs_speedbrakes;
  systemmap["FCS_WINGSWEEP"]=fcs_wingsweep;

  /*
  controlmap["FCS_COLUMN"]=fcsi_column;
  controlmap["FCS_WHEEL"]=fcsi_wheel;
  controlmap["FCS_PEDAL"]=fcsi_pedal;
  controlmap["FCS_FLAPS"]=fcsi_flaps;
  controlmap["FCS_SLATS"]=fcsi_slats;
  controlmap["FCS_KRUEGERS"]=fcsi_kruegers;
  controlmap["FCS_SPEEDBRAKES"]=fcsi_speedbrakes;
  controlmap["FCS_WINGSWEEP"]=fcsi_wingsweep;
  controlmap["FCS_PITCHTRIM"]=fcsi_pitchtrim;
  controlmap["FCS_ROLLTRIM"]=fcsi_rolltrim;
  controlmap["FCS_YAWTRIM"]=fcsi_yawtrim;
  controlmap["FCS_HSTABILIZER"]=fcsi_hstab;
  */

  in >> openbracket;  // get the "{"
  in >> sname;
  cout << "Reading Control System: " << endl;
  int i;

  while((!in.fail()) && (sname != "}")) {
    i=systemmap[sname.c_str()];
    fcslog << sname << endl;
    cout << "  Surface: " << sname << " {" << endl;
    in >> openbracket;  // the {
    in >> buffer;
    int fctr=0;
    currentFilter=0;
    newFilter=0;
    while((!in.fail()) && (buffer != "}") && (fctr <=MAXFILTERS)) {
      fcslog << buffer << endl;
      if(buffer == "FCS_DEFAULT") {
        newFilter=new FGFilter(FDMExec);
      } else if (buffer == "FCS_MECH") {
        fcslog << "Initializing simple filter" << endl;
        newFilter=new FGFLTMech(FDMExec);
      } else if (buffer == "FGFCS_HYDROMECH") {
        //newFilter=new FGCLHydroMech(FDMExec,control,system);
      }
      else if (buffer == "FGFCS_HYDROMECH_MDSTATE") {
        //newFilter=new FGCLHydroMechMDS(FDMExec,control,system);
      }
      else if( buffer == "TRIM_FILTER") {
        //newFilter=new FGFLTTrim(fdmex,control);
      }
      else {
        cout << "Unrecognized filter " << buffer << endl;
      }
      in >> openbracket;
      cout << "    " << "Filter: " << buffer << " {" << endl;
      if ( (newFilter != 0) && (newFilter->LoadFilter(in))) {
        // we have not run out of memory and the file read went ok

        if((fcslist[i] != 0) && (currentFilter == 0)) {
          //we're at the head of the list and a default object exists
          //delete the default object
          fcslog << "Deleting default object on surface " << i << endl;
          delete fcslist[i];
          //and set up to start a new list
          fcslist[i]=0;
        }
        if(fcslist[i] == 0) {
          //we must be starting a new list
          fcslist[i]=newFilter;
          currentFilter=newFilter;
        } else {
          //in middle of list, add object to it
          currentFilter->Next=newFilter;
          //and advance the pointer
          currentFilter=currentFilter->Next;
        }
        currentFilter->Next=0;
        fctr++;
      } else {
        cout << "Error: Filter: " << buffer << " failed to load correctly, ignored" << endl;
      }
      cout << "    " << "}" << endl;
      in >> closebracket;
      fcslog << "Close filter spec: " << closebracket << endl;
      if( closebracket != "}") {
        cout << "Error: Read unexpected token: " << closebracket << endl;
        cout << "This could be caused by a previous error." << endl;
      }
      in >> buffer;

      fcslog << "Next filter: " << buffer << endl;

    }
    fcslog << "Read " << fctr  << " filters for surface " << i << endl;
    if(fctr <= 0) {
      cout << "No filters loaded for the " << sname << " system" << endl;
      cout << "Falling back to default." << endl;
    } else if(fctr > MAXFILTERS)
      cout << "Filter limit exceeded on surface " << i << endl;


    cout << "  " << "}" << endl;
    in >> sname;
    fcslog << "Next surface: " << sname << endl;
  }
  return true;
}
