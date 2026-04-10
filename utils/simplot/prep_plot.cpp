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
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "input_output/string_utilities.h"
#include "plotXMLVisitor.h"

using namespace std;

string plot_range;

string DEFAULT_FONT, TITLE_FONT, LABEL_FONT, AXIS_FONT, TIMESTAMP_FONT, TICS_FONT;

typedef vector <string> string_array;
typedef vector <string_array> multi_string_array;
multi_string_array NamesArray;
bool multiplot=false;

string HaveTerm(const vector <string>&, const string); 
int GetTermIndex(const vector <string>&, const string);
void EmitComparisonPlot(const vector <string>&, const int, const string);
void EmitSinglePlot(const string, const int, const string);
bool MakeArbitraryPlot(
  const vector <string>& files,
  const vector <string>& names,
  const struct Plots& myPlot,
  string Title,
  stringstream& plot);
void PrintNames(const vector <string>&);
string itostr(int number)
{
  stringstream ss;  // create a stringstream
  ss << number;     // add number to the stream
  return ss.str();  // return a string with the contents of the stream
}

int main(int argc, char **argv)
{
  string in_string, var_name, input_arg, supplied_title="";
  string outfile="";
  string_array plotspecfiles;
  string_array files;
  int ctr=1, next_comma=0, len=0, start=0, file_ctr=0;
  ifstream infile2;
  char num[8];
  bool comprehensive=false;
  bool pdf=false;
  bool png=false;
  bool nokey=false;
  bool plotspecs=false;
  string set_thickness="";
  int font_sz_delta=0;

  string font = "Helvetica,";

  unsigned int DEFAULT_FONT_SZ = 12;
  unsigned int TITLE_FONT_SZ = 14;
  unsigned int LABEL_FONT_SZ = 12;
  unsigned int AXIS_FONT_SZ = 12;
  unsigned int TIMESTAMP_FONT_SZ = 10;
  unsigned int TICS_FONT_SZ = 10;
  
  string start_time="", end_time="";

  if (argc == 1 || string(argv[1]) == "--help") {
    cout << endl << "Usage: " << endl << endl;
    cout << "  prep_plot <datafile.csv> [--plot=<plot_directives.xml>] [--comp[rehensive]]"
         << " [--out=<output file name>]"
         << " [--start=<time>] [--end=<time>] [--title=<title>] [--pdf | --png]"
         << " [--thick | --thicker | --thickest] [--smallest | --small | --large | --largest]"
         << endl << endl;
    cout << "If only the input data file name is given, all of the parameters available in that plot file" << endl;
    cout << "are given." << endl << endl;
    exit(-1);
  }

  string filename(argv[1]), new_filename, Title;

  if (filename.find("#") != string::npos) { // if plotting multiple files
    multiplot = true;
    nokey = true;
    while (1) {
      new_filename=filename;
      snprintf(num,sizeof(num),"%d",file_ctr);
      new_filename.replace(new_filename.find("#"),1,num);
      infile2.open(new_filename.c_str());
      if (!infile2.is_open()) {
        break;
      } else {
        getline(infile2, in_string, '\n');
        NamesArray.push_back(split(in_string, ','));
        infile2.close();
        files.push_back(new_filename);
        file_ctr++;
      }
    }
  } else {
    nokey = false;
    files.push_back(filename);
  }

  ifstream infile(files[0].c_str());
  if (!infile.is_open()) {
    cerr << "Could not open file: " << files[0] << endl;
    exit(-1);
  }
  getline(infile, in_string, '\n');
  string_array names = split(in_string, ',');
  unsigned int num_names=names.size();
  
  // Read command line args
  
  for (int i=2; i<argc; i++) {
    input_arg = string(argv[i]);
    if (input_arg.substr(0,6) == "--plot") {
      plotspecs=true;
      plotspecfiles.push_back(input_arg.erase(0,7));
    } else if (input_arg.substr(0,5) == "--out") {
      outfile = input_arg.erase(0,6);
    } else if (input_arg.substr(0,5) == "--pdf") {
      pdf=true;
    } else if (input_arg.substr(0,5) == "--png") {
      png=true;
    } else if (input_arg.substr(0,6) == "--comp") {
      comprehensive=true;
    } else if (input_arg.substr(0,7) == "--title") {
      supplied_title=input_arg.erase(0,8);
    } else if (input_arg.substr(0,7) == "--start") {
      start_time=input_arg.erase(0,8);
    } else if (input_arg.substr(0,5) == "--end") {
      end_time=input_arg.erase(0,6);
    } else if (input_arg.substr(0,10) == "--thickest") {
      set_thickness = "set termoption lw 5";
    } else if (input_arg.substr(0,9) == "--thicker") {
      set_thickness = "set termoption lw 3";
    } else if (input_arg.substr(0,7) == "--thick") {
      set_thickness = "set termoption lw 2";
    } else if (input_arg.substr(0,10) == "--smallest") {
      DEFAULT_FONT_SZ = 8;
      TITLE_FONT_SZ = 8;
      LABEL_FONT_SZ = 8;
      AXIS_FONT_SZ = 8;
      TIMESTAMP_FONT_SZ = 8;
      TICS_FONT_SZ = 8;
      font_sz_delta = 0;
    } else if (input_arg.substr(0,7) == "--small") {
      font_sz_delta = -2;
    } else if (input_arg.substr(0,9) == "--largest") {
      DEFAULT_FONT_SZ = 14;
      TITLE_FONT_SZ = 14;
      LABEL_FONT_SZ = 14;
      AXIS_FONT_SZ = 14;
      TIMESTAMP_FONT_SZ = 14;
      TICS_FONT_SZ = 14;
      font_sz_delta = 0;
    } else if (input_arg.substr(0,7) == "--large") {
      font_sz_delta = 2;
    } else {
      cerr << endl << "Unknown argument " << input_arg << endl;
      exit(-1);
    }
  }

  DEFAULT_FONT_SZ += font_sz_delta;
  TITLE_FONT_SZ += font_sz_delta;
  LABEL_FONT_SZ += font_sz_delta;
  AXIS_FONT_SZ += font_sz_delta;
  TIMESTAMP_FONT_SZ += font_sz_delta;
  TICS_FONT_SZ += font_sz_delta;

  DEFAULT_FONT = font + itostr(DEFAULT_FONT_SZ);
  TITLE_FONT = font + itostr(TITLE_FONT_SZ);
  LABEL_FONT = font + itostr(LABEL_FONT_SZ);
  AXIS_FONT = font + itostr(AXIS_FONT_SZ);
  TIMESTAMP_FONT = font + itostr(TIMESTAMP_FONT_SZ);
  TICS_FONT = font + itostr(TICS_FONT_SZ);

  if (!plotspecs && ! comprehensive) { // Just print out names to be plotted and exit.
    cout << "Known variable names in data file:" << endl;
    PrintNames(names);
    exit(0);
  }

  if (outfile.size() == 0) {
    outfile = files[0].substr(0,files[0].size()-4);
  } else {
    if (outfile.find(".pdf") != string::npos) {
      outfile = outfile.substr(0,outfile.size()-4);
    } else if (outfile.find(".png") != string::npos) {
      outfile = outfile.substr(0,outfile.size()-4);
    } else if (outfile.find(".ps") != string::npos) {
      outfile = outfile.substr(0,outfile.size()-3);
    }
  }

  plot_range="";
  if (start_time.size() > 0 || end_time.size() > 0)
    plot_range = "["+start_time+":"+end_time+"]";

  if (pdf) {
    cout << "set terminal pdf enhanced color rounded size 12,9 font \"" << DEFAULT_FONT << "\"" << endl;
    cout << "set output '" << outfile << ".pdf'" << endl;
    cout << "set lmargin  13" << endl;
    cout << "set rmargin  4" << endl;
    cout << "set tmargin  4" << endl;
    cout << "set bmargin  4" << endl;
    if (nokey) cout << "set nokey" << endl;
  } else if (png) {
    cout << "set terminal png enhanced truecolor size 1280,1024 rounded font \"" << DEFAULT_FONT << "\"" << endl;
    cout << "set output '" << outfile << ".png'" << endl;
    cout << "set size 1.0,1.0" << endl;
    cout << "set origin 0.0,0.0" << endl;
    cout << "set lmargin  6" << endl;
    cout << "set rmargin  4" << endl;
    cout << "set tmargin  4" << endl;
    cout << "set bmargin  4" << endl;
    if (nokey) cout << "set nokey" << endl;
  } else {
    cout << "set terminal postscript enhanced color font \"" << DEFAULT_FONT << "\"" << endl;
    cout << "set output '" << outfile << ".ps'" << endl;
    if (nokey) cout << "set nokey" << endl;
  }

  if (!supplied_title.empty()) {
    cout << "set title \"" << supplied_title << "\" font \"" << TITLE_FONT << "\"" << endl;
  }
  
  cout << "set datafile separator \",\"" << endl;
  cout << "set grid xtics ytics" << endl;
  cout << "set xtics font \"" << TICS_FONT << "\"" << endl;
  cout << "set ytics font \"" << TICS_FONT << "\"" << endl;
  cout << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \"" << TIMESTAMP_FONT << "\"" << endl;

  if (set_thickness.length() > 0)
    cout << set_thickness << endl;

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
        cout << "set ylabel \"" << names[i+2] << "\" font \"" << LABEL_FONT << "\"" << endl;
        if (!multiplot) EmitSinglePlot(files[0], i+3, names[i+2]);
        else EmitComparisonPlot(files, i+3, names[i+2]);

        // Plot 2 in middle
        cout << "set tmargin  2" << endl;
        cout << "set bmargin  2" << endl;
        cout << "set title \"\"" << endl;
        cout << "set xlabel \"\"" << endl;
        cout << "set ylabel \"" << names[i+1] << "\" font \"" << LABEL_FONT << "\"" << endl;
        if (!multiplot) EmitSinglePlot(files[0], i+2, names[i+1]);
        else EmitComparisonPlot(files, i+2, names[i+1]);

        // Plot 3 at bottom
        cout << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \"" << TIMESTAMP_FONT << "\"" << endl;
        cout << "set tmargin  0" << endl;
        cout << "set bmargin  4" << endl;
        cout << "set title \"\"" << endl;
        cout << "set format x \"%.1f\"" << endl;
        cout << "set xlabel \"Time (sec)\" font \"" << LABEL_FONT << "\"" << endl;
        cout << "set ylabel \"" << names[i] << "\" font \"" << LABEL_FONT << "\"" << endl;
        if (!multiplot) EmitSinglePlot(files[0], i+1, names[i]);
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
               << "\\n" << names[i] << " vs. Time\" font \"" << TITLE_FONT << "\"" << endl;
        } else {
          cout << "set title \"" << names[i] << " vs. Time\" font \"" << TITLE_FONT << "\"" << endl;
        }
        cout << "set xlabel \"Time (sec)\" font \"" << LABEL_FONT << "\"" << endl;
        cout << "set ylabel \"" << names[i] << "\" font \"" << LABEL_FONT << "\"" << endl;

        if (!multiplot) {                             // Single file
          EmitSinglePlot(files[0], i+1, names[i]);
        } else { // Multiple files
          EmitComparisonPlot(files, i+1, names[i]);
        }
      }
    }
  } // end if comprehensive

  // special single plots from plot spec files

  vector <string> LeftYAxisNames;
  vector <string> RightYAxisNames;
  string title;
  stringstream newPlot;
  bool result = false;
  plotXMLVisitor myVisitor;

  // Execute this for each plot spec file e.g. --plot=data_plot/position.xml --plot=data_plot/velocities.xml ... */
  for (int fl=0; fl<plotspecfiles.size(); fl++) {

    newPlot.str("");

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
      newPlot << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \"" << TIMESTAMP_FONT << "\"" << endl;
      result = MakeArbitraryPlot(files, names, myPlot, Title, newPlot);
      if (result) cout << newPlot.str();
    }

    // special multiple plots (using <page> element)

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

      newPlot << "set timestamp \"%d/%m/%y %H:%M\" offset 0,1 font \"" << TIMESTAMP_FONT << "\"" << endl;
      newPlot << "set multiplot title \"" + supplied_title + "\"" << endl;

      for (int plot=0; plot<numPlots; plot++)
      {
        struct Plots& myPlot = myVisitor.vPages[page].vPlots[plot];
        float position = (float)plot*(size + 2.*margin);

        newPlot << "set size 1.0," << size << endl;
        newPlot << "set origin 0.0," << position << endl;

        Title = "";
        // Add Title logic here, if any needed?

        newPlot << "##" << endl << "##" << endl;
        newPlot << "print \"Processing parameter plot: " << myPlot.Title << "\"" << endl;
        cout << "##" << endl << "##" << endl;

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
//
// Determines if the supplied name "parameter" is to be found in any of the long
// long names in the names[] array. Parameter may be a shorthand version of the
// fully qualified name in the names[] array.

string HaveTerm(const vector <string>& names, const string parameter)
{
  for (unsigned int i=0; i<names.size(); i++) {
    if (names[i].find(parameter) != string::npos) {
      int start = names[i].find(parameter);
      if (start + parameter.length() == names[i].size()) return names[i];
    }
  }
  //cerr << "Could not find parameter: _" << parameter << "_" << endl;

  return string("");
}

// ############################################################################

int GetTermIndex(const vector <string>& names, const string parameter)
{
  for (unsigned int i=0; i<names.size(); i++) {
    if (names[i].find(parameter) != string::npos) {
      int start = names[i].find(parameter);
      if (start + parameter.length() == names[i].size()) return i+1;
    }
  }
  return -1;
}

// ############################################################################

bool MakeArbitraryPlot(
  const vector <string>& files,
  const vector <string>& names,
  const struct Plots& myPlot,
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
  string plotType = "lines";

  if (myPlot.plotType == points) plotType = "points";

  // This line assumes time is in column 1
  if (GetTermIndex(names, XAxisName) == 1) time_range = plot_range;

  have_all_terms = have_all_terms && !HaveTerm(names, XAxisName).empty();
  for (i=0; i<numLeftYAxisNames; i++) have_all_terms = have_all_terms && !HaveTerm(names, LeftYAxisNames[i]).empty();
  for (i=0; i<numRightYAxisNames; i++) have_all_terms = have_all_terms && !HaveTerm(names, RightYAxisNames[i]).empty();

  if (have_all_terms) {
    // Title
    Title += myPlot.Title;
    if (!Title.empty())
      newPlot << "set title \"" << Title << "\" font \"" << LABEL_FONT << "\"" << endl;
    else
      newPlot << "unset title" << endl;
    
    // X axis caption and ranges
    if (!myPlot.Axis_Caption[eX].empty()) {
      newPlot << "set xlabel \"" << myPlot.Axis_Caption[eX] << "\" font \"" << LABEL_FONT << "\"" << endl;
    } else {
      newPlot << "unset xlabel" << endl;
    }

    // Left Y axis caption and ranges
    if (!myPlot.Axis_Caption[eY].empty()) {
      newPlot << "set ylabel \"" << myPlot.Axis_Caption[eY] << "\" font \"" << LABEL_FONT << "\"" << endl;
    } else {
      newPlot << "unset ylabel" << endl;
    }

    // Right Y axis caption and ranges
    if (!myPlot.Axis_Caption[eY2].empty()) {
      newPlot << "set y2label \"" << myPlot.Axis_Caption[eY2] << "\" font \"" << LABEL_FONT << "\"" << endl;
    } else {
      newPlot << "unset y2label" << endl;
    }

    newPlot << "set xrange [" << myPlot.Min[0] << ":" << myPlot.Max[0] << "]" << endl;
    newPlot << "set yrange [" << myPlot.Min[1] << ":" << myPlot.Max[1] << "]" << endl;
    if (myPlot.Y2_Variables.size() > 0) {
      newPlot << "set y2range [" << myPlot.Min[2] << ":" << myPlot.Max[2] << "]" << endl;
    }

    if (myPlot.plotType == points) newPlot << "set pointsize 0.25" << endl;

                                          // ##########################
    if ( !multiplot ) {                   // ## Single file plotting ##
                                          // ##########################
      if (numRightYAxisNames > 0) {
        newPlot << "set rmargin 9" << endl;
        newPlot << "set y2tics font \"" << TICS_FONT << "\"" << endl;
      }

      newPlot << "plot " << time_range << " \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
           << ":" << GetTermIndex(names, LeftYAxisNames[0]) << " with " << plotType << " title \""
           << LeftYAxisNames[0] << "\"";
      if (numLeftYAxisNames > 1) {
        newPlot << ", \\" << endl;
        for (i=1; i<numLeftYAxisNames-1; i++) {
          newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, LeftYAxisNames[i]) << " with " << plotType << " title \"" 
               << LeftYAxisNames[i] << "\", \\" << endl;
        }
        newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)<< ":" 
             << GetTermIndex(names, LeftYAxisNames[numLeftYAxisNames-1]) << " with " << plotType << " title \"" 
             << LeftYAxisNames[numLeftYAxisNames-1] << "\"";
      }
      if (numRightYAxisNames > 0) {
        newPlot << ", \\" << endl;
        for (i=0; i<numRightYAxisNames-1; i++) {
          newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
               << ":" << GetTermIndex(names, RightYAxisNames[i]) << " with " << plotType << " axes x1y2 title \""
               << RightYAxisNames[i] << "\", \\" << endl;
        }
        newPlot << "     \"" << files[0] << "\" using " << GetTermIndex(names, XAxisName)
             << ":" << GetTermIndex(names, RightYAxisNames[numRightYAxisNames-1]) << " with " << plotType << " axes x1y2 title \""
             << RightYAxisNames[numRightYAxisNames-1] << "\"";
      }
      newPlot << endl;
      if (numRightYAxisNames > 0) {
        newPlot << "set rmargin 4" << endl;
        newPlot << "unset y2tics" << endl;
        newPlot << "set y2label" << endl;
      }
                                     // #######################################
    } else {                         // ## Multiple file comparison plotting ##
                                     // #######################################
      if (numRightYAxisNames > 0) {
        newPlot << "set rmargin 9" << endl;
        newPlot << "set y2tics font \"" << TICS_FONT << "\"" << endl;
      }

      for (int f=0;f<files.size();f++) {

        bool HasAllTerms = true;

        if (HaveTerm(NamesArray[f], XAxisName).empty()) HasAllTerms = false;
        for (unsigned int c=0; c<numLeftYAxisNames; c++) if (HaveTerm(NamesArray[f], LeftYAxisNames[c]).empty()) HasAllTerms = false;
        for (unsigned int c=0; c<numRightYAxisNames; c++) if (HaveTerm(NamesArray[f], RightYAxisNames[c]).empty()) HasAllTerms = false;

        if (!HasAllTerms) {
          newPlot.clear();
          return HasAllTerms;
        }

        if (f==0) newPlot << "plot " << time_range << " ";
        else      {
          newPlot << ", \\" << endl;
          newPlot << "     ";
        }

        newPlot << "\"" << files[f] << "\" using " << GetTermIndex(NamesArray[f], XAxisName)
             << ":" << GetTermIndex(NamesArray[f], LeftYAxisNames[0]) << " with " << plotType << " title \""
             << LeftYAxisNames[0] << ": " << f << "\"";
        if (numLeftYAxisNames > 1) {
          newPlot << ", \\" << endl;
          for (i=1; i<numLeftYAxisNames-1; i++) {
            newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(NamesArray[f], XAxisName)
                 << ":" << GetTermIndex(NamesArray[f], LeftYAxisNames[i]) << " with " << plotType << " title \"" 
                 << LeftYAxisNames[i] << ": " << f << "\", \\" << endl;
          }
          newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(NamesArray[f], XAxisName)<< ":" 
               << GetTermIndex(NamesArray[f], LeftYAxisNames[numLeftYAxisNames-1]) << " with " << plotType << " title \"" 
               << LeftYAxisNames[numLeftYAxisNames-1] << ": " << f << "\"";
        }
        if (numRightYAxisNames > 0) {
          newPlot << ", \\" << endl;
          for (i=0; i<numRightYAxisNames-2; i++) {
            newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(NamesArray[f], XAxisName)
                 << ":" << GetTermIndex(NamesArray[f], RightYAxisNames[i]) << " with " << plotType << " axes x1y2 title \""
                 << RightYAxisNames[i] << ": " << f << "\", \\" << endl;
          }
          newPlot << "     \"" << files[f] << "\" using " << GetTermIndex(NamesArray[f], XAxisName)
               << ":" << GetTermIndex(NamesArray[f], RightYAxisNames[numRightYAxisNames-1]) << " with " << plotType << " axes x1y2 title \""
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

void EmitSinglePlot(const string filename, const int index, const string linetitle )
{
  cout << "print \"Processing parameter plot: " << linetitle << "\"" << endl;
  cout << "plot " << plot_range << " \"" << filename << "\" using 1:" << index << " with lines title \"" << linetitle << "\"" << endl;
}

// ############################################################################

void EmitComparisonPlot(const vector <string>& filenames, const int index, const string linetitle)
{
  string varname = NamesArray[0][index-1];
  bool fail = false;

  for (unsigned int f=1;f<filenames.size();f++) {
    if (HaveTerm(NamesArray[f],varname).size() == 0) {
      cerr << "## Variable: " << varname << " does not exist in all files being plotted." << endl;
      fail = true;
    }
  }

  if (!fail) {
  cout << "##" << endl << "##" << endl;
  cout << "print \"Processing parameter plot: " << linetitle << "\"" << endl;
  cout << "##" << endl << "##" << endl;
    cout << "plot " << plot_range <<  " \"" << filenames[0] << "\" using 1:" << GetTermIndex(NamesArray[0],varname) << " with lines title \"" << linetitle << ": 1" << "\", \\" << endl;
  for (unsigned int f=1;f<filenames.size()-1;f++){
      cout << "\"" << filenames[f] << "\" using 1:" << GetTermIndex(NamesArray[f],varname) << " with lines title \"" << linetitle << ": " << f+1 << "\", \\" << endl;
  }
    cout << "\"" << filenames[filenames.size()-1] << "\" using 1:" << GetTermIndex(NamesArray[filenames.size()-1],varname) << " with lines title \"" << linetitle << ": " << filenames.size() << "\"" << endl;
  }

}

// ############################################################################

void PrintNames(const vector <string>& names)
{
  for (int i=0; i<names.size(); i++) {
    cout << "  " << i+1 << ":  " << names[i] << endl;
  }
}
