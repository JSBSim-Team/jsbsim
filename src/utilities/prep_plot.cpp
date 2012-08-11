/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       prep_plot.cpp
 Author:       Jon S. Berndt
 Date started: 11/24/2006
 Purpose:      CSV -> gnuplot prepping tool
 
 ------------- Copyright (C) 2006  Jon S. Berndt (jon@jsbsim.org) -------------

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

./prep_plot F4NOutput#.csv --comp

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

g++ prep_plot.cpp plotXMLVisitor.cpp ../simgear/xml/easyxml.cxx -I ../ -L ../simgear/xml/ -lExpat -o prep_plot.exe

These compiler options may produce a faster executable if your machines supports it:
-O9 -march=nocona 

Usage:
(note that an argument with embedded spaces needs to be surrounded by quotes)

prep_plot <filename.csv> [--title="my title"] [--plot=<plotfile.xml>] [--comp[rehensive]] [--start=start_time] [--end=end_time]

I have used this utility as follows to produce a PDF file:

./prep_plot.exe F4NOutput#.csv --title="F4N Ground Reactions Testing (0.3, 0.2)" > gpfile.txt
gnuplot gpfile.txt
ps2pdf F4NOutput0.ps F4NOutput0.pdf

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
#include <sstream>
#include "input_output/string_utilities.h"
#include "plotXMLVisitor.h"

#define DEFAULT_FONT "Arial,10"
#define TITLE_FONT "Arial,12"
#define LABEL_FONT "Arial,10"
#define AXIS_FONT "Arial,10"
#define TIMESTAMP_FONT "Arial,8"
#define TICS_FONT "Arial,8"

using namespace std;

string plot_range;

string HaveTerm(vector <string>&, string); 
int GetTermIndex(vector <string>&, string);
void EmitComparisonPlot(vector <string>&, int, string);
void EmitSinglePlot(string, int, string);
bool MakeArbitraryPlot(
  vector <string>& files,
  vector <string>& names,
  struct Plots& myPlot,
  string Title,
  stringstream& plot);

int main(int argc, char **argv)
{
  string in_string, var_name, input_arg, supplied_title="";
  vector <string> plotspecfiles;
  vector <string> names;
  int ctr=1, next_comma=0, len=0, start=0, file_ctr=0;
  vector <string> files;
  ifstream infile2;
  char num[4];
  bool comprehensive=false;
  bool pdf=false;
  bool png=false;

  string start_time="", end_time="";

  if (argc == 1 || string(argv[1]) == "--help") {
    cout << endl << "Usage: " << endl << endl;
    cout << "  prep_plot <datafile.csv> [--plot=<plot_directives.xml>] [--comp[rehensive]] [--start=<time>] [--end=<time.] [--title=<title>] [--pdf|--png]"
         << endl << endl;
    exit(-1);
  }

  string filename(argv[1]), new_filename, Title;

  if (filename.find("#") != string::npos) { // if plotting multiple files
    while (1) {
      new_filename=filename;
      sprintf(num,"%d",file_ctr);
      new_filename.replace(new_filename.find("#"),1,num);
      infile2.open(new_filename.c_str());
      if (!infile2.is_open()) {
        break;
      } else {
        infile2.close();
        files.push_back(new_filename);
        file_ctr++;
      }
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
  names = split(in_string, ',');
  unsigned int num_names=names.size();
  
  // Read command line args
  
  for (int i=2; i<argc; i++) {
    input_arg = string(argv[i]);
    if (input_arg.substr(0,6) == "--plot") {
      plotspecfiles.push_back(input_arg.erase(0,7));
    } else if (input_arg.substr(0,6) == "--pdf") {
      pdf=true;
    } else if (input_arg.substr(0,6) == "--png") {
      png=true;
    } else if (input_arg.substr(0,6) == "--comp") {
      comprehensive=true;
    } else if (input_arg.substr(0,7) == "--title") {
      supplied_title=input_arg.erase(0,8);
    } else if (input_arg.substr(0,7) == "--start") {
      start_time=input_arg.erase(0,8);
    } else if (input_arg.substr(0,5) == "--end") {
      end_time=input_arg.erase(0,6);
    } else {
      cerr << endl << "Unknown argument " << input_arg << endl;
      exit(-1);
    }
  }

  plot_range="";
  if (start_time.size() > 0 || end_time.size() > 0)
    plot_range = "["+start_time+":"+end_time+"]";

  if (pdf) {
    cout << "set terminal pdf enhanced color rounded size 12,9 font \""DEFAULT_FONT"\"" << endl;
    cout << "set output '" << files[0].substr(0,files[0].size()-4) << ".pdf'" << endl;
    cout << "set lmargin  13" << endl;
    cout << "set rmargin  4" << endl;
    cout << "set tmargin  4" << endl;
    cout << "set bmargin  4" << endl;
  } else if (png) {
    cout << "set terminal png enhanced truecolor size 1280,1024 rounded font \""DEFAULT_FONT"\"" << endl;
    cout << "set output '" << files[0].substr(0,files[0].size()-4) << ".png'" << endl;
    cout << "set size 1.0,1.0" << endl;
    cout << "set origin 0.0,0.0" << endl;
    cout << "set lmargin  6" << endl;
    cout << "set rmargin  4" << endl;
    cout << "set tmargin  4" << endl;
    cout << "set bmargin  4" << endl;
  } else {
    cout << "set terminal postscript enhanced color font \""DEFAULT_FONT"\"" << endl;
    cout << "set output '" << files[0].substr(0,files[0].size()-4) << ".ps'" << endl;
  }

  if (!supplied_title.empty()) {
    cout << "set title \"" << supplied_title << "\" font \""TITLE_FONT"\"" << endl;
  }
  
  cout << "set datafile separator \",\"" << endl;
  cout << "set grid xtics ytics" << endl;
  cout << "set xtics font \""TICS_FONT"\"" << endl;
  cout << "set ytics font \""TICS_FONT"\"" << endl;
  cout << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \""TIMESTAMP_FONT"\"" << endl;

  if (comprehensive) {
  
    for (unsigned int i=1;i<num_names;i++) {
    
      if ( i <= num_names-3 &&
           (
             (
             names[i].find("_X") != string::npos
             && names[i+1].find("_Y") != string::npos
             && names[i+2].find("_Z") != string::npos
             )
             ||
             (
             names[i].find("X_") != string::npos
             && names[i+1].find("Y_") != string::npos
             && names[i+2].find("Z_") != string::npos
             )
             ||
             (
             names[i].find("P ") != string::npos
             && names[i+1].find("Q ") != string::npos
             && names[i+2].find("R ") != string::npos
             )
           )
         )

      { // XYZ value

        cout << "set multiplot layout 3,1 title \"" + supplied_title + "\"" << endl;
        cout << "set format x \"\"" << endl;
        cout << "unset timestamp" << endl;

        // Plot 1 at top
        cout << "set tmargin  4" << endl;
        cout << "set bmargin  0" << endl;
        cout << "set title \"\"" << endl;
        cout << "set xlabel \"\"" << endl;
        cout << "set ylabel \"" << names[i+2] << "\" font \""LABEL_FONT"\"" << endl;
        if (files.size()==1) EmitSinglePlot(files[0], i+3, names[i+2]);
        else EmitComparisonPlot(files, i+3, names[i+2]);

        // Plot 2 in middle
        cout << "set tmargin  2" << endl;
        cout << "set bmargin  2" << endl;
        cout << "set title \"\"" << endl;
        cout << "set xlabel \"\"" << endl;
        cout << "set ylabel \"" << names[i+1] << "\" font \""LABEL_FONT"\"" << endl;
        if (files.size()==1) EmitSinglePlot(files[0], i+2, names[i+1]);
        else EmitComparisonPlot(files, i+2, names[i+1]);

        // Plot 3 at bottom
        cout << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \""TIMESTAMP_FONT"\"" << endl;
        cout << "set tmargin  0" << endl;
        cout << "set bmargin  4" << endl;
        cout << "set title \"\"" << endl;
        cout << "set format x \"%.1f\"" << endl;
        cout << "set xlabel \"Time (sec)\" font \""LABEL_FONT"\"" << endl;
        cout << "set ylabel \"" << names[i] << "\" font \""LABEL_FONT"\"" << endl;
        if (files.size()==1) EmitSinglePlot(files[0], i+1, names[i]);
        else EmitComparisonPlot(files, i+1, names[i]);

        i += 2;
        cout << "unset multiplot" << endl;

        cout << "set size 1.0,1.0" << endl;
        cout << "set origin 0.0,0.0" << endl;

        cout << "set tmargin  4" << endl;
        cout << "set bmargin  4" << endl;

    } else { // Straight single value to plot

        if (!supplied_title.empty()) { // title added
          cout << "set title \"" << supplied_title 
               << "\\n" << names[i] << " vs. Time\" font \""TITLE_FONT"\"" << endl;
        } else {
          cout << "set title \"" << names[i] << " vs. Time\" font \""TITLE_FONT"\"" << endl;
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

  // special single plots

  vector <string> LeftYAxisNames;
  vector <string> RightYAxisNames;
  string title;
  stringstream newPlot;
  bool result = false;
  plotXMLVisitor myVisitor;

  // Execute this for each plot spec file e.g. --plot=data_plot/position.xml --plot=data_plot/velocities.xml ... */
  for (int fl=0; fl<plotspecfiles.size(); fl++) {

    ifstream plotDirectivesFile(plotspecfiles[fl].c_str());
    if (!plotDirectivesFile) {
      cerr << "Could not open autoplot file " << plotspecfiles[fl] << endl << endl;
      continue; // if a data plot spec file doesn't exist, skip it.
    }

    plotXMLVisitor *aVisitor = new plotXMLVisitor();
    myVisitor = *aVisitor;
    readXML (plotDirectivesFile, myVisitor);

    for (int i=0; i<myVisitor.vPlots.size();i++) {
      newPlot.str("");
      struct Plots& myPlot = myVisitor.vPlots[i];
      Title = "";
      if (!supplied_title.empty()) Title = supplied_title + string("\\n");
      newPlot << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \""TIMESTAMP_FONT"\"" << endl;
      result = MakeArbitraryPlot(files, names, myPlot, Title, newPlot);
      if (result) cout << newPlot.str();
    }

    // special multiple plots

    for (int page=0; page<myVisitor.vPages.size(); page++) {
      int numPlots = myVisitor.vPages[page].vPlots.size();
      newPlot.str("");
      
      // Calculate margins smartly
      float marginXLabel = 0.0;
      for (int plot=1; plot<numPlots; plot++)
      {
        if (myVisitor.vPages[page].vPlots[plot].Axis_Caption[eX].size() > 0) {
          marginXLabel = 8.0;
          break;
        }
      }

      float marginTitle = 0.0;
      for (int plot=0; plot<numPlots-1; plot++)
      {
        if (myVisitor.vPages[page].vPlots[plot].Title.size() > 0) {
          marginTitle = 9.0;
          break;
        }
      }

      float margin = (3. + marginTitle + marginXLabel)/540.;
      float plot_margin = (2.*(numPlots-1.))*margin;
      float size = (1.0 - plot_margin)/(float)numPlots;
/*
      if (!pdf && !png) {
        cout << "set size 1.0,1.0" << endl;
        cout << "set origin 0.0,0.0" << endl;
      } else {
//          cout << "set size 0.9,0.9" << endl;
//          cout << "set origin 0.05,0.05" << endl;
      }
*/
//        newPlot << "set size 1.0,1.0" << endl;
//        newPlot << "set origin 0.0,0.0" << endl;
      newPlot << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \""TIMESTAMP_FONT"\"" << endl;
      newPlot << "set multiplot" << endl;

      for (int plot=0; plot<numPlots; plot++)
      {
        struct Plots& myPlot = myVisitor.vPages[page].vPlots[plot];
        float position = (float)plot*(size + 2.*margin);
        newPlot << "set size 1.0," << size << endl;
        newPlot << "set origin 0.0," << position << endl;

        Title = "";
        if (!supplied_title.empty()) Title = supplied_title + string("\\n");

        result = MakeArbitraryPlot(files, names, myPlot, Title, newPlot);
        if (!result) break;
        newPlot << "unset timestamp" << endl;
      }
    
      newPlot << "unset multiplot" << endl;
      newPlot << "set size 1.0,1.0" << endl;
      newPlot << "set origin 0.0,0.0" << endl;

      if (result) cout << newPlot.str();
    }
  }
  // cout << endl << "System call here" << endl;
}

// ############################################################################

string HaveTerm(vector <string>& names, string parameter)
{
  for (unsigned int i=0; i<names.size(); i++) {
    if (names[i] == parameter.substr(0,names[i].size())) return names[i];
  }
  cerr << "Could not find parameter: _" << parameter << "_" << endl;
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

bool MakeArbitraryPlot(
  vector <string>& files,
  vector <string>& names,
  struct Plots& myPlot,
  string Title,
  stringstream& newPlot)
{
  bool have_all_terms=true;
  int i;
  vector <string> LeftYAxisNames = myPlot.Y_Variables;
  vector <string> RightYAxisNames = myPlot.Y2_Variables;
  string XAxisName = myPlot.X_Variable;
  int numLeftYAxisNames = LeftYAxisNames.size();
  int numRightYAxisNames = RightYAxisNames.size();
  string time_range="";

  // This line assumes time is in column 1
  if (GetTermIndex(names, XAxisName) == 1) time_range = plot_range;

  have_all_terms = have_all_terms && !HaveTerm(names, XAxisName).empty();
  for (i=0; i<numLeftYAxisNames; i++) have_all_terms = have_all_terms && !HaveTerm(names, LeftYAxisNames[i]).empty();
  for (i=0; i<numRightYAxisNames; i++) have_all_terms = have_all_terms && !HaveTerm(names, RightYAxisNames[i]).empty();

  if (have_all_terms) {
    // Title
    Title += myPlot.Title;
    if (!Title.empty())
      newPlot << "set title \"" << Title << "\" font \""TITLE_FONT"\"" << endl;
    else
      newPlot << "unset title" << endl;
    
    // X axis caption
    if (!myPlot.Axis_Caption[eX].empty())
      newPlot << "set xlabel \"" << myPlot.Axis_Caption[eX] << "\" font \""LABEL_FONT"\"" << endl;
    else
      newPlot << "unset xlabel" << endl;

    // Left Y axis caption
    if (!myPlot.Axis_Caption[eY].empty())
      newPlot << "set ylabel \"" << myPlot.Axis_Caption[eY] << "\" font \""LABEL_FONT"\"" << endl;
    else
      newPlot << "unset ylabel" << endl;

    // Right Y axis caption
    if (!myPlot.Axis_Caption[eY2].empty())
      newPlot << "set y2label \"" << myPlot.Axis_Caption[eY2] << "\" font \""LABEL_FONT"\"" << endl;
    else
      newPlot << "unset y2label" << endl;

    if (files.size() == 1) { // Single file
    
      if (numRightYAxisNames > 0) {
        newPlot << "set rmargin 9" << endl;
        newPlot << "set y2tics font \""TICS_FONT"\"" << endl;
      }

      newPlot << "plot " << time_range << " \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
           << ":" << GetTermIndex(names, LeftYAxisNames[0]) << " with lines title \""
           << LeftYAxisNames[0] << "\"";
      if (numLeftYAxisNames > 1) {
        newPlot << ", \\" << endl;
        for (i=1; i<numLeftYAxisNames-1; i++) {
          newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, LeftYAxisNames[i]) << " with lines title \"" 
               << LeftYAxisNames[i] << "\", \\" << endl;
        }
        newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)<< ":" 
             << GetTermIndex(names, LeftYAxisNames[numLeftYAxisNames-1]) << " with lines title \"" 
             << LeftYAxisNames[numLeftYAxisNames-1] << "\"";
      }
      if (numRightYAxisNames > 0) {
        newPlot << ", \\" << endl;
        for (i=0; i<numRightYAxisNames-1; i++) {
          newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, RightYAxisNames[i]) << " with lines axes x1y2 title \""
               << RightYAxisNames[i] << "\", \\" << endl;
        }
        newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
             << ":" << GetTermIndex(names, RightYAxisNames[numRightYAxisNames-1]) << " with lines axes x1y2 title \""
             << RightYAxisNames[numRightYAxisNames-1] << "\"";
      }
      newPlot << endl;
      if (numRightYAxisNames > 0) {
        newPlot << "set rmargin 4" << endl;
        newPlot << "unset y2tics" << endl;
        newPlot << "set y2label" << endl;
      }

    } else { // Multiple file comparison plot

      if (numRightYAxisNames > 0) {
        newPlot << "set rmargin 9" << endl;
        newPlot << "set y2tics font \""TICS_FONT"\"" << endl;
      }

      for (int f=0;f<files.size();f++) {
      
        if (f==0) cout << "plot " << time_range << " ";
        else      {
          newPlot << ", \\" << endl;
          newPlot << "     ";
        }

        newPlot << "\"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
             << ":" << GetTermIndex(names, LeftYAxisNames[0]) << " with lines title \""
             << LeftYAxisNames[0] << ": " << f << "\"";
        if (numLeftYAxisNames > 1) {
          newPlot << ", \\" << endl;
          for (i=1; i<numLeftYAxisNames-1; i++) {
            newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
                 << ":" << GetTermIndex(names, LeftYAxisNames[i]) << " with lines title \"" 
                 << LeftYAxisNames[i] << ": " << f << "\", \\" << endl;
          }
          newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)<< ":" 
               << GetTermIndex(names, LeftYAxisNames[numLeftYAxisNames-1]) << " with lines title \"" 
               << LeftYAxisNames[numLeftYAxisNames-1] << ": " << f << "\"";
        }
        if (numRightYAxisNames > 0) {
          newPlot << ", \\" << endl;
          for (i=0; i<numRightYAxisNames-2; i++) {
            newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
                 << ":" << GetTermIndex(names, RightYAxisNames[i]) << " with lines axes x1y2 title \""
                 << RightYAxisNames[i] << ": " << f << "\", \\" << endl;
          }
          newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, RightYAxisNames[numRightYAxisNames-1]) << " with lines axes x1y2 title \""
               << RightYAxisNames[numRightYAxisNames-1] << ": " << f << "\"";
        }
      }
      newPlot << endl;
      if (numRightYAxisNames > 0) {
        newPlot << "set rmargin 4" << endl;
        newPlot << "unset y2tics" << endl;
        newPlot << "set y2label" << endl;
      }
    }
  }
  return have_all_terms;
}

// ############################################################################

void EmitSinglePlot(string filename, int index, string linetitle )
{
  cout << "plot " << plot_range << " \"" << filename << "\" using 1:" << index << " with lines title \"" << linetitle << "\"" << endl;
}

// ############################################################################

void EmitComparisonPlot(vector <string>& filenames, int index, string linetitle)
{
  cout << "plot " << plot_range <<  " \"" << filenames[0] << "\" using 1:" << index << " with lines title \"" << linetitle << ": 1" << "\", \\" << endl;
  for (unsigned int f=1;f<filenames.size()-1;f++){
    cout << "\"" << filenames[f] << "\" using 1:" << index << " with lines title \"" << linetitle << ": " << f+1 << "\", \\" << endl;
  }
  cout << "\"" << filenames[filenames.size()-1] << "\" using 1:" << index << " with lines title \"" << linetitle << ": " << filenames.size() << "\"" << endl;
}
