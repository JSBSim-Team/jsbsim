/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Sat Nov  4 21:16:33 CST 2000
    copyright            : (C) 2000 by Jon S. Berndt
    email                : jsb@hal-pc.org
    
    compile this under windows like this:
    
    g++ -o simplot datafile.cpp main.cpp -I/usr/local/dislin -L/usr/local/dislin -ldisgcc -lgdi32 -lcomdlg32
    
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream.h>
#include <cstdio>
#include <cstdlib>
#include "dislin.h"
#include "datafile.h"

int main(int argc, char *argv[])
{
  string commands_str, digits = "0123456789", displaytype="CONS", next;
  string savefile="", pngFName_str="JSBSim", time_str, float_digits=".0123456789";
  vector <int> commands_vec;
  size_t start = 0, end = 0, namelen;
  char pngFName[200]="JSBSim";
  float sf, ef;
  char scratch;

  if (argc != 2 ) {
    cout << endl << "Usage: SimPlot <data_file_name.csv>" << endl << endl;
    exit(-1);
  }

  DataFile df(argv[1]);

  cout << endl << endl << "Here are the available parameters which may be plotted: " << endl;
  cout << endl;
  
  namelen = 0;
  for (unsigned int i=0; i<df.names.size();i++) {
    namelen = namelen > df.names[i].size() ? namelen : df.names[i].size();
  }

nextplot:
  commands_str = "";
  time_str = "";
  start=0;
  end=0;
  commands_vec.clear();

  bool col1 = true;
  for (unsigned int i=0; i<df.names.size();i++) {
    if (col1) {
      cout << i << ") " << df.names[i];
      for (unsigned int j=0; j<= namelen - df.names[i].size() + 2; j++) cout << " ";
      if (i<10) cout << " ";
      col1=!col1;
    } else {
      cout << i << ") " << df.names[i] << endl;
      col1=!col1;
    }
  }

  int numvars   = df.GetNumFields();
  int numpoints = df.GetNumRecords();
  float starttime = df.GetStartTime();
  float endtime   = df.GetEndTime();

  cout << endl;
  cout << endl << "Data file contains " << numvars << " independent variables." << endl;
  cout <<         "Number of data points: " << numpoints << endl;
  cout <<         "Time goes from " << starttime << " to " << endtime << " seconds." << endl;

entertime:

  cout << endl << "Enter new time range [#.#-#.# or -]: ";
  cin >> time_str;

  sf = starttime;
  ef = endtime;

  if (time_str[0] != '-') {
    sscanf(time_str.c_str(),"%f %c %f",&sf, &scratch, &ef);
    
    bool redo = true;
    if (ef <= sf) {
      cout << "The end time must be greater than the start time" << endl;
    } else if (ef <= 0.0) {
      cout << "The end time must be greater than zero" << endl;
    } else if (sf < 0.000) {
      cout << "The start time must not be less than zero" << endl;
    } else if (sf < starttime) {
      cout << "The start time must not be less than " << starttime << endl;
    } else if (ef > endtime) {
      cout << "The end time must not be greater than " << endtime << endl;
    } else {
      for (int pt=0; pt<df.GetNumRecords(); pt++) {
        if (df.Data[pt][0] <= sf) df.SetStartIdx(pt);
        if (df.Data[pt][0] <= ef) {
          df.SetEndIdx(pt);
        } else {
          break;
        }
      }
      redo = false;
    }
    if (redo) goto entertime;
  } else {
    df.SetStartIdx(0);
    df.SetEndIdx(df.GetNumRecords()-1);
  }

  cout << endl << "Enter a comma-separated list of the items to be plotted (or 'q' to quit): ";
  cin >> commands_str;

  if (commands_str[0] == 'q') exit(0);
  
  while (1) {
    start = commands_str.find_first_of(digits,start);
    if (start >= string::npos) break;
    end   = commands_str.find_first_not_of(digits,start);
    commands_vec.push_back(atoi(commands_str.substr(start, end-start).c_str()));
    start=end;
  }

  cout << "Initializing plot page ..." << endl;
  
savepng:

// Set page format

  metafl((char*)displaytype.c_str());
  setpag("da4l");

// Initialization

  disini();

// Set Plot Parameters

  pagera();
  serif();
  shdcha();
  chncrv("color");

// Set plot axis system

  axspos(450,1800);
  axslen(2200,1200);

// Set plot titles

  int thisplot;
  int numtraces = commands_vec.size();

  name("Time (in seconds)","x");
  
  string longaxistext;
  for (thisplot=0; thisplot < numtraces; thisplot++) {
    longaxistext = longaxistext + " " + df.names[commands_vec[thisplot]];
  }
  name((char*)(longaxistext.c_str()),"y");

  labdig(3,"xy");
  ticks(10,"xy");

  titlin("JSBSim plot",1);
  string subtitle = "";
  for (thisplot=0; thisplot < numtraces; thisplot++) {
    if (thisplot == 0) subtitle = df.names[commands_vec[thisplot]];
    else               subtitle = subtitle + ", " + df.names[commands_vec[thisplot]];
  }

  subtitle = subtitle + string(" vs. Time");
  titlin((char*)(subtitle.c_str()),3);

// Plot data

  float *timarray = new float[df.GetEndIdx()-df.GetStartIdx()+1]; // new jsb 11/9

  for (int pt=df.GetStartIdx(), pti=0; pt<=df.GetEndIdx(); pt++, pti++) {
    timarray[pti] = df.Data[pt][0];
  }

  float axismax = df.GetAutoAxisMax(commands_vec[0]);
  float axismin = df.GetAutoAxisMin(commands_vec[0]);

  for (thisplot=1; thisplot < numtraces; thisplot++) {
    axismax = axismax > df.GetAutoAxisMax(commands_vec[thisplot]) ? axismax : df.GetAutoAxisMax(commands_vec[thisplot]);
    axismin = axismin < df.GetAutoAxisMin(commands_vec[thisplot]) ? axismin : df.GetAutoAxisMin(commands_vec[thisplot]);
  }
  
  float spread = axismax - axismin;

  if      (spread < 1.0)   labdig(3,"y");
  else if (spread < 10.0)  labdig(2,"y");
  else if (spread < 100.0) labdig(1,"y");
  else                      labdig(0,"y");

  if (spread > 1000.0) {
    labdig(2,"y");
    labels("fexp","y");
  } else {
    labels("float","y");
  }

  spread = df.Data[df.GetEndIdx()][0] - df.Data[df.GetStartIdx()][0];
  
  if      (spread < 1.0)   labdig(3,"x");
  else if (spread < 10.0)  labdig(2,"x");
  else if (spread < 100.0) labdig(1,"x");
  else                      labdig(0,"x");

  float fac = ((int)(spread + 0.5))/10.00;
  
  if (spread > 1000.0) labels("fexp","x");
  else                 labels("float","x");

  graf( df.Data[df.GetStartIdx()][0], // starttime
        df.Data[df.GetEndIdx()][0],   // endtime
        df.Data[df.GetStartIdx()][0], // starttime
        fac,
        axismin,
        axismax,
        axismin,
        (axismax - axismin)/10.0);

  title();
  color("blue");
  grid(1,1);

  for (thisplot=0; thisplot < numtraces; thisplot++) {
    float *datarray = new float[df.GetEndIdx()-df.GetStartIdx()+1];
    for (int pt=df.GetStartIdx(), pti=0; pt<=df.GetEndIdx(); pt++, pti++) {
      datarray[pti] = df.Data[pt][commands_vec[thisplot]];
    }
    color("red");
    curve(timarray,datarray,df.GetEndIdx()-df.GetStartIdx()+1);
    delete[] datarray;
  }

  char legendtext[numtraces*(namelen)];
  
  color("blue");
  legini(legendtext, numtraces, (namelen));
  legtit("Legend");
  for (thisplot=0; thisplot < numtraces; thisplot++) {
    leglin(legendtext, (char*)df.names[commands_vec[thisplot]].c_str(), thisplot+1);
  }
  legend(legendtext, 3);

// Terminate

  disfin();

// Request to save

  if (displaytype == "CONS") {
    cout << endl << "Save graph as a .png file [y|N]: ";
    cin >> savefile;
    if (savefile[0] == 'y') {
      displaytype = "PNG";
      pngFName_str = dwgtxt("Enter filename:",pngFName);
      if (pngFName_str.find_first_of("png",0) == string::npos) {
        pngFName_str = pngFName_str + ".png";
      }
      setfil((char*)pngFName_str.c_str());
      goto savepng;
    }
  } else {
    displaytype = "CONS";
  }

  cout << endl << "Create another plot? [Y|n]: ";
  cin >> next;
  
  if (next[0] == 'n') {
    return EXIT_SUCCESS;
  } else {
    goto nextplot;
  }    
}

