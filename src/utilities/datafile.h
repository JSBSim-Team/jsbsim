/***************************************************************************
                          datafile.h  -  description
                             -------------------
    begin                : Sat Nov 4 2000
    copyright            : (C) 2000 by Jon S. Berndt
    email                : jsb@hal-pc.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DATAFILE_H
#define DATAFILE_H

#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>

using namespace std;

/**This class handles reading a data file and placing user-requested data into arrays for plotting.
  *@author Jon S. Berndt
  */

class DataFile {
public:
  DataFile();
  ~DataFile();
  DataFile(string fname);

  std::vector <string> names;
  string data_str;
  typedef std::vector <float> Row;
  typedef std::vector <Row> DataType;
  DataType Data;

  int GetNumFields(void) {if (Data.size() > 0) return(Data[0].size()); else return(0);}
  int GetNumRecords(void) {return(Data.size());}
  float GetStartTime(void) {if (Data.size() >= 2) return(Data[0][0]); else return(0);}
  float GetEndTime(void) {if (Data.size() >= 2) return(Data[Data.size()-1][0]); else return(0);}
  float GetMax(int column) {return(Max[column]);}
  float GetMin(int column) {return(Min[column]);}
  float GetRange(int field) {return (GetMax(field) - GetMin(field));}
  float GetAutoAxisMax(int item);
  float GetAutoAxisMin(int item);
  void SetStartIdx(int sidx) {StartIdx = sidx;}
  void SetEndIdx(int eidx)   {EndIdx = eidx;}
  int GetStartIdx(void)       {return StartIdx;}
  int GetEndIdx(void)         {return EndIdx;}

private: // Private attributes
  string buff_str;
  ifstream f;
  Row Max;
  Row Min;
  int StartIdx, EndIdx;
};
#endif

