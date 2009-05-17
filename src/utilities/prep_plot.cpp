/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       prep_plot.cpp
 Author:       Jon S. Berndt
 Date started: 11/24/2006
 Purpose:      CSV -> gnuplot prepping tool
 
 ------------- Copyright (C) 2006  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------

prep_plot
---------

Input:

A data file in comma-separated value (CSV) format, with time in the first
column. The data file is expected to be all numeric. That is, NaN will
mess it up, I think.

Multiple files (currently up to 10) can be input to prep_plot provided
the names of the files are all the same except for a digit. The digit
is substituted for on the command line to prep_plot using the "#" character.

For example:

./prep_plot F4NOutput#.csv

will look for all input files named F4NOutput0.csv, F4NOutput1.csv,
F4NOutput2.csv, etc. up to F4NOutput9.csv. This will probably be modified
in the future to accept a larger number of plots.

Output:

The output is to standard out (the console, stdout). The output consists of
commands to gnuplot. This file is used as input to gnuplot. Gnuplot then
produces a postscript format file consisting of all plots generated. This
postscript file can easily be converted to PDF format using the ps2pdf utility
that is part of Ghostscript.

Compiling:

g++ prep_plot.cpp plotXMLVisitor.cpp ../simgear/xml/easyxml.cxx -L ../simgear/xml/ -lExpat -o prep_plot.exe

Usage:
(note that an argument with embedded spaces needs to be surrounded by quotes)

prep_plot <filename.csv> [--title="my title"] [--plot=<plotfile.xml> --comprehensive]

I have used this utility as follows to produce a PDF file:

./prep_plot.exe F4NOutput#.csv --title="F4N Ground Reactions Testing (0.3, 0.2)" > gpfile.txt
gnuplot gpfile.txt
ps2pdf14 F4NOutput0.ps F4NOutput0.pdf

Special Notes:

When a set of 3 subsequent data terms is encountered in the .csv data file that
have as part of their names the "subscripts" "_X", "_Y", and "_Z" (at the end
of the variable name) the variables will be plotted on the same page.
Otherwise, all items are plotted individually. This capability will be extended
eventually to include _P, _Q, and _R and perhaps other aeronautically relevant
parameters.

The names of data items as output by JSBSim (as part of FGOutput.cpp) will
likely be modified to work better with this utility.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include "plotXMLVisitor.h"

#define DEFAULT_FONT "Helvetica,10"
#define TITLE_FONT "Helvetica,14"
#define LABEL_FONT "Helvetica,12"
#define AXIS_FONT "Helvetica,12"
#define TIMESTAMP_FONT "Helvetica,8"
#define TICS_FONT "Helvetica,8"

using namespace std;

string HaveTerm(vector <string>&, string); 
int GetTermIndex(vector <string>&, string);
void EmitComparisonPlot(vector <string>&, int, string);
void EmitSinglePlot(string, int, string);
void MakeArbitraryPlot(
  vector <string>& files,
  vector <string>& names,
  string XAxisName,
  vector <string>& LeftYAxisNames,
  vector <string>& RightYAxisNames,
  string Title);

int main(int argc, char **argv)
{
  string in_string, var_name, input_arg, plotspecfile="", supplied_title="";
  vector <string> names;
  int ctr=1, next_comma=0, len=0, start=0, file_ctr=0;
  vector <string> files;
  ifstream infile2;
  char num[4];
  bool comprehensive=false;

  if (argc == 1 || argc > 5) {
    cout << endl << "Usage: " << endl << endl;
    cout << "  prep_plot <datafile.csv> [--plot=<plot_directives.csv> --comp[rehensive]] [--title=<title>]"
         << endl << endl;
    exit(-1);
  }

  string filename(argv[1]), new_filename, Title;

  if (filename.find("#") != string::npos) { // if plotting multiple files
    while (file_ctr<10) {
      new_filename=filename;
      sprintf(num,"%d",file_ctr);
      new_filename.replace(new_filename.find("#"),1,num);
      infile2.open(new_filename.c_str());
      if (!infile2.is_open()) break;
      infile2.close();
      files.push_back(new_filename);
      file_ctr++;
    }
  } else {
    files.push_back(filename);
  }
  
  ifstream infile(files[0].c_str());
  if (!infile.is_open()) {
    cerr << "Could not open file: " << files[0] << endl;
    exit(-1);
  }
  getline(infile, in_string, '\n');
  
  // Read command line args
  
  for (int i=2; i<argc; i++) {
    input_arg = string(argv[i]);
    if (input_arg.substr(0,6) == "--plot") {
      plotspecfile=input_arg.erase(0,7);
    } else if (input_arg.substr(0,6) == "--comp") {
      comprehensive=true;
    } else if (input_arg.substr(0,7) == "--title") {
      supplied_title=input_arg.erase(0,8);
    } else {
      cerr << endl << "Unknown argument " << input_arg << endl;
      exit(-1);
    }
  }

  plotXMLVisitor myVisitor;
  if (!plotspecfile.empty()) {
    ifstream plotDirectivesFile(plotspecfile.c_str());
    if (!plotDirectivesFile) {
      cerr << "Could not open autoplot file " << plotspecfile << endl << endl;
      exit(-1);
    }
    readXML (plotDirectivesFile, myVisitor);
  }

  cout << "set terminal postscript enhanced color \""DEFAULT_FONT"\"" << endl;
  if (!supplied_title.empty()) {
    cout << "set title \"" << supplied_title << "\" font \""TITLE_FONT"\"" << endl;
  }
  cout << "set output '" << files[0].substr(0,files[0].size()-4) << ".ps'" << endl;
  
  cout << "set size 1.0,1.0" << endl;
  cout << "set origin 0.0,0.0" << endl;
  cout << "set lmargin  6" << endl;
  cout << "set rmargin  4" << endl;
  cout << "set tmargin  1" << endl;
  cout << "set bmargin  1" << endl;
  
  cout << "set datafile separator \",\"" << endl;
  cout << "set grid xtics ytics" << endl;
  cout << "set xtics font \""TICS_FONT"\"" << endl;
  cout << "set ytics font \""TICS_FONT"\"" << endl;
  cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \""TIMESTAMP_FONT"\"" << endl;

  while(next_comma != string::npos) {
    next_comma=in_string.find_first_of(",",start);
    if (next_comma == string::npos) {
      var_name=in_string.substr(start);
    } else {
      len = next_comma-start;
      var_name=in_string.substr(start,len);
    }
    var_name.erase(0,var_name.find_first_not_of(" "));
    names.push_back(var_name);
    start = next_comma+1;
    ctr++;
  }
  
  unsigned int num_names=names.size();
  
  if (comprehensive) {
  
    for (unsigned int i=1;i<num_names;i++) {
    
      if (    i <= num_names-3
           && names[i].find("_X") != string::npos
           && names[i+1].find("_Y") != string::npos
           && names[i+2].find("_Z") != string::npos )

      { // XYZ value

        if (!supplied_title.empty()) {
          cout << "set title \"" << supplied_title
               << "\\n" << names[i] << " vs. Time\" font \""TITLE_FONT"\"" << endl;
        }

        cout << "set size 1.0,1.0" << endl;
        cout << "set origin 0.0,0.0" << endl;
        cout << "set multiplot" << endl;
        cout << "set size 1.0,0.35" << endl;
        if (files.size()==1) { // Single file
          cout << "set origin 0.0,0.65" << endl;
          cout << "set xlabel \"\"" << endl;
          cout << "set ylabel \"" << names[i] << "\" font \""LABEL_FONT"\"" << endl;
          cout << "set format x \"\"" << endl;
          cout << "set timestamp \"\" 0,0 \""TIMESTAMP_FONT"\"" << endl;
          EmitSinglePlot(files[0], i+1, names[i]);

          cout << "set origin 0.0,0.325" << endl;
          cout << "set title \"\"" << endl;
          cout << "set ylabel \"" << names[i+1] << "\" font \""LABEL_FONT"\"" << endl;
          EmitSinglePlot(files[0], i+2, names[i+1]);

          cout << "set origin 0.0,0.0" << endl;
          cout << "set xlabel \"Time (sec)\" font \""LABEL_FONT"\"" << endl;
          cout << "set ylabel \"" << names[i+2] << "\" font \""LABEL_FONT"\"" << endl;
          cout << "set format x" << endl;
          cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \""TIMESTAMP_FONT"\"" << endl;
          EmitSinglePlot(files[0], i+3, names[i+2]);

        } else { // Multiple files, multiple plots per page

          // Plot 1 (top) X
          cout << "set origin 0.0,0.65" << endl;
          cout << "set xlabel \"\"" << endl;
          cout << "set ylabel \"" << names[i] << "\" font \""LABEL_FONT"\"" << endl;
          cout << "set format x \"\"" << endl;
          cout << "set timestamp \"\" 0,0 \""TIMESTAMP_FONT"\"" << endl;
          EmitComparisonPlot(files, i+1, names[i]);

          // Plot 2 (middle) Y
          cout << "set origin 0.0,0.325" << endl;
          cout << "set title \"\"" << endl;
          cout << "set ylabel \"" << names[i+1] << "\" font \""LABEL_FONT"\"" << endl;
          EmitComparisonPlot(files, i+2, names[i+1]);

          // Plot 3 (bottom) Z
          cout << "set origin 0.0,0.00" << endl;
          cout << "set xlabel \"Time (sec)\" font \""LABEL_FONT"\"" << endl;
          cout << "set ylabel \"" << names[i+2] << "\" font \""LABEL_FONT"\"" << endl;
          cout << "set format x" << endl;
          cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \""TIMESTAMP_FONT"\"" << endl;
          EmitComparisonPlot(files, i+3, names[i+2]);
        }
        i += 2;
        cout << "unset multiplot" << endl;
        cout << "set size 1.0,1.0" << endl;
        cout << "set origin 0.0,0.0" << endl;

      } else { // Straight single value to plot

        if (!supplied_title.empty()) { // title added
          cout << "set title \"" << supplied_title 
               << "\\n" << names[i] << " vs. Time\" font \""TITLE_FONT"\"" << endl;
        }
        cout << "set xlabel \"Time (sec)\" font \""LABEL_FONT"\"" << endl;
        cout << "set ylabel \"" << names[i] << "\" font \""LABEL_FONT"\"" << endl;

        if (files.size()==1) { // Single file
          EmitSinglePlot(files[0], i+1, names[i]);
        } else { // Multiple files
          EmitComparisonPlot(files, i+1, names[i]);
        }
      }
    }
  } // end if comprehensive

  // special plots

  vector <string> LeftYAxisNames;
  vector <string> RightYAxisNames;
  string title;

  for (int i=0; i<myVisitor.vPlots.size();i++) {
    struct Plots& myPlot = myVisitor.vPlots[i];
    LeftYAxisNames.clear();
    for (int y=0;y<myPlot.Y_Variables.size();y++) {
      LeftYAxisNames.push_back(myPlot.Y_Variables[y]);
    }
    RightYAxisNames.clear();
    if (!supplied_title.empty()) Title = supplied_title + string("\\n");
    else Title.clear();
    Title += myPlot.Title;
    MakeArbitraryPlot(files, names, myPlot.X_Variable, LeftYAxisNames, RightYAxisNames, Title);
  }

/*
  LeftYAxisNames.clear();
  LeftYAxisNames.push_back("Latitude (Deg)");
  RightYAxisNames.clear();
  if (!supplied_title.empty()) Title = supplied_title + string("\\n");
  else Title.clear();
  Title += "Ground Track";
  MakeArbitraryPlot(files, names, "Longitude (Deg)", LeftYAxisNames, RightYAxisNames, Title);

  LeftYAxisNames.clear();
  LeftYAxisNames.push_back("Q");
  RightYAxisNames.clear();
  RightYAxisNames.push_back("M");
  if (!supplied_title.empty()) Title = supplied_title + string("\\n");
  else Title.clear();
  Title += "Pitch Response";
  MakeArbitraryPlot(files, names, "Time", LeftYAxisNames, RightYAxisNames, Title);

  LeftYAxisNames.clear();
  LeftYAxisNames.push_back("P");
  LeftYAxisNames.push_back("Q");
  LeftYAxisNames.push_back("R");
  RightYAxisNames.clear();
  if (!supplied_title.empty()) Title = supplied_title + string("\\n");
  else Title.clear();
  Title += "Body Rates";
  MakeArbitraryPlot(files, names, "Time", LeftYAxisNames, RightYAxisNames, Title);
*/
}

// ############################################################################

string HaveTerm(vector <string>& names, string parameter)
{
  for (unsigned int i=0; i<names.size(); i++) {
    if (names[i] == parameter) return names[i];
  }
  return string("");
}

// ############################################################################

int GetTermIndex(vector <string>& names, string parameter)
{
  for (unsigned int i=0; i<names.size(); i++) {
    if (names[i] == parameter) return i+1;
  }
  return -1;
}

// ############################################################################

void MakeArbitraryPlot(
  vector <string>& files,
  vector <string>& names,
  string XAxisName,
  vector <string>& LeftYAxisNames,
  vector <string>& RightYAxisNames,
  string Title)
{
  bool have_all_terms=true;
  int i;
  int numLeftYAxisNames = LeftYAxisNames.size();
  int numRightYAxisNames = RightYAxisNames.size();
  
  have_all_terms = have_all_terms && !HaveTerm(names, XAxisName).empty();
  for (i=0; i<numLeftYAxisNames; i++) have_all_terms = have_all_terms && !HaveTerm(names, LeftYAxisNames[i]).empty();
  for (i=0; i<numRightYAxisNames; i++) have_all_terms = have_all_terms && !HaveTerm(names, RightYAxisNames[i]).empty();

  if (have_all_terms) {
    if (!Title.empty()) {
      cout << "set title \"" << Title << "\" font \""TITLE_FONT"\"" << endl;
    }
    cout << "set xlabel \"" << XAxisName << "\" font \""LABEL_FONT"\"" << endl;

    cout << "set ylabel \"";
    for (i=0; i<numLeftYAxisNames-1; i++) {
      cout << LeftYAxisNames[i] << ", ";
    }
    cout << LeftYAxisNames[numLeftYAxisNames-1] << "\" font \""LABEL_FONT"\"" << endl;

    if (numRightYAxisNames > 0) {
      cout << "set y2label \"";
      for (i=0; i<numRightYAxisNames-1; i++) {
        cout << RightYAxisNames[i] << ", ";
      }
      cout << RightYAxisNames[numRightYAxisNames-1] << "\" font \""LABEL_FONT"\"" << endl;
    }

    cout << "set timestamp \"%d/%m/%y %H:%M\" 0,0 \""TIMESTAMP_FONT"\"" << endl;

    if (files.size() == 1) { // Single file
    
      if (numRightYAxisNames > 0) {
        cout << "set rmargin 9" << endl;
        cout << "set y2tics font \""TICS_FONT"\"" << endl;
      }

      cout << "plot \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
           << ":" << GetTermIndex(names, LeftYAxisNames[0]) << " with lines title \""
           << LeftYAxisNames[0] << "\"";
      if (numLeftYAxisNames > 1) {
        cout << ", \\" << endl;
        for (i=1; i<numLeftYAxisNames-1; i++) {
          cout << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, LeftYAxisNames[i]) << " with lines title \"" 
               << LeftYAxisNames[i] << "\", \\" << endl;
        }
        cout << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)<< ":" 
             << GetTermIndex(names, LeftYAxisNames[numLeftYAxisNames-1]) << " with lines title \"" 
             << LeftYAxisNames[numLeftYAxisNames-1] << "\"";
      }
      if (numRightYAxisNames > 0) {
        cout << ", \\" << endl;
        for (i=0; i<numRightYAxisNames-1; i++) {
          cout << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, RightYAxisNames[i]) << " with lines axes x1y2 title \""
               << RightYAxisNames[i] << "\", \\" << endl;
        }
        cout << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
             << ":" << GetTermIndex(names, RightYAxisNames[numRightYAxisNames-1]) << " with lines axes x1y2 title \""
             << RightYAxisNames[numRightYAxisNames-1] << "\"";
      }
      cout << endl;
      if (numRightYAxisNames > 0) {
        cout << "set rmargin 4" << endl;
        cout << "unset y2tics" << endl;
        cout << "set y2label" << endl;
      }

    } else { // Multiple file comparison plot

      if (numRightYAxisNames > 0) {
        cout << "set rmargin 9" << endl;
        cout << "set y2tics font \""TICS_FONT"\"" << endl;
      }

      for (int f=0;f<files.size();f++) {
      
        if (f==0) cout << "plot ";
        else      {
          cout << ", \\" << endl;
          cout << "     ";
        }

        cout << "\"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
             << ":" << GetTermIndex(names, LeftYAxisNames[0]) << " with lines title \""
             << LeftYAxisNames[0] << ": " << f << "\"";
        if (numLeftYAxisNames > 1) {
          cout << ", \\" << endl;
          for (i=1; i<numLeftYAxisNames-1; i++) {
            cout << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
                 << ":" << GetTermIndex(names, LeftYAxisNames[i]) << " with lines title \"" 
                 << LeftYAxisNames[i] << ": " << f << "\", \\" << endl;
          }
          cout << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)<< ":" 
               << GetTermIndex(names, LeftYAxisNames[numLeftYAxisNames-1]) << " with lines title \"" 
               << LeftYAxisNames[numLeftYAxisNames-1] << ": " << f << "\"";
        }
        if (numRightYAxisNames > 0) {
          cout << ", \\" << endl;
          for (i=0; i<numRightYAxisNames-2; i++) {
            cout << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
                 << ":" << GetTermIndex(names, RightYAxisNames[i]) << " with lines axes x1y2 title \""
                 << RightYAxisNames[i] << ": " << f << "\", \\" << endl;
          }
          cout << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, RightYAxisNames[numRightYAxisNames-1]) << " with lines axes x1y2 title \""
               << RightYAxisNames[numRightYAxisNames-1] << ": " << f << "\"";
        }
      }
      cout << endl;
      if (numRightYAxisNames > 0) {
        cout << "set rmargin 4" << endl;
        cout << "unset y2tics" << endl;
        cout << "set y2label" << endl;
      }
    }
  }
}

// ############################################################################

void EmitSinglePlot(string filename, int index, string linetitle )
{
  cout << "plot \"" << filename << "\" using 1:" << index << " with lines title \"" << linetitle << "\"" << endl;
}

// ############################################################################

void EmitComparisonPlot(vector <string>& filenames, int index, string linetitle)
{
  cout << "plot \"" << filenames[0] << "\" using 1:" << index << " with lines title \"" << linetitle << ": 1" << "\", \\" << endl;
  for (unsigned int f=1;f<filenames.size()-1;f++){
    cout << "\"" << filenames[f] << "\" using 1:" << index << " with lines title \"" << linetitle << ": " << f+1 << "\", \\" << endl;
  }
  cout << "\"" << filenames[filenames.size()-1] << "\" using 1:" << index << " with lines title \"" << linetitle << ": " << filenames.size() << "\"" << endl;
}
