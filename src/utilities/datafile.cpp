/***************************************************************************
                          datafile.cpp  -  description
                             -------------------
    begin                : Sat Nov 4 2000
    copyright            : (C) 2000 by Jon S. Berndt
    email                : jon@jsbsim.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "datafile.h"

DataFile::DataFile() {

}


DataFile::~DataFile() {
  f.close();
}


/** This overloaded constructor opens the requested file. */

DataFile::DataFile(string fname) {
  int count=0;
  unsigned short start, end;
  string var;

  f.open(fname.c_str());
  f.setf(ios::skipws);
  if ( !f ) {
    cout << "fileopen failed for file " << fname << endl << endl;
    exit(-1);
  } else {
    cout << "File " << fname << " successfully opened." << endl;
  }

  getline(f, data_str);
  end = 0;

  while (1) {
    start = end;
    while (data_str[start] == ' ') start++;
    end = data_str.find(",", start);
    count++;
    if (end >= 65535) {
      end = data_str.size();
      var = data_str.substr(start, end-start);
      names.push_back(var);
      break;
    } else {
      var = data_str.substr(start, end-start);
      names.push_back(var);
      end++;
    }
  }

  cout << "Done parsing names. Reading data ..." << endl;

  int row=0, column=0;
  char scratch[10];

  while (1) {
    column = 0;
    Data.push_back(*(new Row()));
    while (1) {
      Data[row].push_back(0.0);
      f >> Data[row][column];
      if (f.eof()) {
        Data.pop_back();
        break;
      }
      if (++column == count) break;
      else f >> scratch;
    }
    row++;
    if (f.eof()) break;
  }

  for (int i=0;i<GetNumFields();i++) {
    Max.push_back(0.0);
    Min.push_back(0.0);
  }

  for (int fld=0; fld<GetNumFields(); fld++) {
    Max[fld] = Data[0][fld];
    Min[fld] = Data[0][fld];
    for (int rec=1;rec<GetNumRecords();rec++) {
      if (Data[rec][fld] > Max[fld]) Max[fld] = Data[rec][fld];
      else if (Data[rec][fld] < Min[fld]) Min[fld] = Data[rec][fld];
    }
  }

  StartIdx = 0;
  EndIdx = GetNumRecords()-1;

  cout << endl << "Done Reading data ..." << endl;

}


float DataFile::GetAutoAxisMax(int item) {
  double Mx, order, magnitude;
  float max = Max[item];
  float min = Min[item];

  if (max == 0.0 && min == 0.0) return(1.0);

  if (StartIdx != 0 || EndIdx != (GetNumRecords()-1)) {
    Mx = Data[StartIdx][item];
    for (int rec=StartIdx+1; rec<=EndIdx; rec++) {
      if (Data[rec][item] > Mx) Mx = Data[rec][item];
    }
  }

  order = (int)(log10(fabs(max)));
  magnitude = pow((double)10.0, (double)order);

  if (max > 0.0) Mx = ((int)(max / magnitude) + 1) * magnitude;
  else           Mx = ((int)(max / magnitude)) * magnitude;

  return Mx;
}


float DataFile::GetAutoAxisMin(int item) {
  float Mn, order, magnitude;
  float min = Min[item];
  float max = Max[item];

  if (max == 0.0 && min == 0.0) return(0.0);

  if (StartIdx != 0 || (EndIdx != GetNumRecords()-1)) {
    min = Data[StartIdx][item];
    for (int rec=StartIdx+1; rec<=EndIdx; rec++) {
      if (Data[rec][item] < min) min = Data[rec][item];
    }
  }

  order = (int)(log10(fabs(min)));
  magnitude = pow((double)10.0, (double)order);

  if (min > 0.0) Mn = ((int)(min / magnitude)) * magnitude;
  else           Mn = ((int)(min / magnitude) - 1) * magnitude;

  return Mn;
}


